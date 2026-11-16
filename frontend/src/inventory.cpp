/*
 * Copyright (c) 2026 boxart-nx contributors
 * Distributed under the terms of the GNU General Public License v2
 */

#include "inventory.hpp"
#include "jpeg_pipeline.hpp"

#define STBI_NO_STDIO
#include <stb/stb_image.h>
#include <webp/decode.h>

#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <algorithm>
#include <cstdio>
#include <cstring>

namespace BoxArt {

void GameItem::FreeTextures() {
    if (current_icon_texture) {
        SDL_DestroyTexture(current_icon_texture);
        current_icon_texture = nullptr;
    }
    icon_loaded = false;
}

Inventory::~Inventory() {
    Clear();
}

void Inventory::Clear() {
    for (auto& game : m_games) {
        game.FreeTextures();
    }
    m_games.clear();

    for (auto& item : m_import_files) {
        if (item.thumbnail_texture) {
            SDL_DestroyTexture(item.thumbnail_texture);
            item.thumbnail_texture = nullptr;
        }
    }
    m_import_files.clear();
}

SDL_Texture* CreateTextureFromRgba(SDL_Renderer* renderer, const uint8_t* rgba, int width, int height) {
    if (!renderer || !rgba || width <= 0 || height <= 0) {
        return nullptr;
    }

    SDL_Texture* tex = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STATIC, width, height);
    if (!tex) {
        return nullptr;
    }

    SDL_UpdateTexture(tex, nullptr, rgba, width * 4);
    SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);
    return tex;
}

SDL_Texture* CreateTextureFromMemory(SDL_Renderer* renderer, const uint8_t* data, size_t size) {
    if (!renderer || !data || size == 0) {
        return nullptr;
    }

    int w = 0, h = 0;
    uint8_t* rgba = nullptr;
    bool is_webp = false;

    if (WebPGetInfo(data, size, &w, &h)) {
        rgba = WebPDecodeRGBA(data, size, &w, &h);
        is_webp = true;
    }

    if (!rgba) {
        int comp = 0;
        rgba = stbi_load_from_memory(data, static_cast<int>(size), &w, &h, &comp, 4);
    }

    if (!rgba) {
        return nullptr;
    }

    SDL_Texture* tex = CreateTextureFromRgba(renderer, rgba, w, h);

    if (is_webp) {
        WebPFree(rgba);
    } else {
        stbi_image_free(rgba);
    }

    return tex;
}

SDL_Texture* CreateTextureFromFile(SDL_Renderer* renderer, const std::string& path) {
    FILE* f = std::fopen(path.c_str(), "rb");
    if (!f) return nullptr;

    std::fseek(f, 0, SEEK_END);
    long size = std::ftell(f);
    std::fseek(f, 0, SEEK_SET);

    if (size <= 0 || size > 30 * 1024 * 1024) {
        std::fclose(f);
        return nullptr;
    }

    std::vector<uint8_t> buffer(size);
    size_t read_bytes = std::fread(buffer.data(), 1, size, f);
    std::fclose(f);

    if (read_bytes != static_cast<size_t>(size)) {
        return nullptr;
    }

    return CreateTextureFromMemory(renderer, buffer.data(), buffer.size());
}

static std::string ToLower(const std::string& str) {
    std::string lower = str;
    std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char c) { return std::tolower(c); });
    return lower;
}

void Inventory::EnsureIconLoaded(SDL_Renderer* renderer, GameItem& item) {
    if (item.icon_loaded && item.current_icon_texture != nullptr) {
        return;
    }

    if (item.current_icon_texture) {
        SDL_DestroyTexture(item.current_icon_texture);
        item.current_icon_texture = nullptr;
    }

    item.has_custom_cover = JpegPipeline::HasCustomCover(item.title_id);
    if (item.has_custom_cover) {
        std::string path = JpegPipeline::GetCoverPath(item.title_id);
        item.current_icon_texture = CreateTextureFromFile(renderer, path);
    }

    // Fallback to original icon texture from NAND data
    if (!item.current_icon_texture && !item.original_icon_jpeg.empty()) {
        item.current_icon_texture = CreateTextureFromMemory(renderer, item.original_icon_jpeg.data(), item.original_icon_jpeg.size());
    }

    item.icon_loaded = true;
}

void Inventory::RefreshGameTextures(SDL_Renderer* renderer, GameItem& item) {
    item.FreeTextures();
    EnsureIconLoaded(renderer, item);
}

bool Inventory::ScanImportDirectory(SDL_Renderer* /*renderer*/) {
    for (auto& item : m_import_files) {
        if (item.thumbnail_texture) {
            SDL_DestroyTexture(item.thumbnail_texture);
            item.thumbnail_texture = nullptr;
        }
    }
    m_import_files.clear();

    const char* dir_path = "sdmc:/config/boxart-nx/import";
    DIR* d = opendir(dir_path);
    if (!d) {
        mkdir("sdmc:/config/boxart-nx", 0777);
        mkdir("sdmc:/config/boxart-nx/import", 0777);
        d = opendir(dir_path);
        if (!d) return false;
    }

    struct dirent* entry = nullptr;
    while ((entry = readdir(d)) != nullptr) {
        if (entry->d_name[0] == '.') continue;

        std::string fname = entry->d_name;
        std::string lower = ToLower(fname);

        if (lower.ends_with(".png") || lower.ends_with(".jpg") || lower.ends_with(".jpeg") || lower.ends_with(".webp")) {
            ImportFileItem item;
            item.filename = fname;
            item.full_path = std::string(dir_path) + "/" + fname;
            item.thumbnail_texture = nullptr; // Lazy-loaded when browse modal opens
            m_import_files.push_back(std::move(item));
        }
    }
    closedir(d);

    // Update matched import paths on all games
    for (auto& game : m_games) {
        game.matched_import_path.clear();
        for (const auto& imp : m_import_files) {
            std::string imp_lower = ToLower(imp.filename);
            if (imp_lower.starts_with(game.title_id_hex)) {
                game.matched_import_path = imp.full_path;
                break;
            }
        }
    }

    return true;
}

bool Inventory::ReloadGames(SDL_Renderer* renderer, std::string& out_error) {
    (void)out_error;
    Clear();

    // Allocate NsApplicationControlData dynamically on heap to avoid huge stack allocation (144 KB)
    auto control_data = std::make_unique<NsApplicationControlData>();

    s32 entry_offset = 0;
    while (true) {
        NsApplicationRecord records[32];
        s32 out_count = 0;
        Result rc = nsListApplicationRecord(records, 32, entry_offset, &out_count);
        if (R_FAILED(rc) || out_count <= 0) {
            break;
        }

        for (s32 i = 0; i < out_count; ++i) {
            uint64_t tid = records[i].application_id;
            if (tid == 0) continue;

            GameItem game;
            game.title_id = tid;

            char hex_buf[32];
            std::snprintf(hex_buf, sizeof(hex_buf), "%016llx", (unsigned long long)tid);
            game.title_id_hex = hex_buf;

            u64 actual_size = 0;
            std::memset(control_data.get(), 0, sizeof(NsApplicationControlData));
            rc = nsGetApplicationControlData(NsApplicationControlSource_Storage, tid, control_data.get(), sizeof(NsApplicationControlData), &actual_size);

            if (R_SUCCEEDED(rc)) {
                NacpLanguageEntry* lang_entry = nullptr;
                rc = nsGetApplicationDesiredLanguage(&control_data->nacp, &lang_entry);
                if (R_SUCCEEDED(rc) && lang_entry) {
                    game.name = lang_entry->name;
                    game.author = lang_entry->author;
                } else {
                    game.name = "Unknown Title (" + game.title_id_hex + ")";
                    game.author = "Unknown Author";
                }

                if (control_data->nacp.display_version[0] != '\0') {
                    game.version = control_data->nacp.display_version;
                } else {
                    game.version = "1.0.0";
                }

                // Copy original icon raw JPEG bytes (zero GPU decode at startup)
                if (actual_size > sizeof(NacpStruct)) {
                    size_t icon_size = static_cast<size_t>(actual_size - sizeof(NacpStruct));
                    if (icon_size <= sizeof(control_data->icon)) {
                        game.original_icon_jpeg.resize(icon_size);
                        std::memcpy(game.original_icon_jpeg.data(), control_data->icon, icon_size);
                    }
                }
            } else {
                game.name = "Title " + game.title_id_hex;
                game.author = "Unknown";
                game.version = "1.0.0";
            }

            // Check if custom cover file is already on SD card
            game.has_custom_cover = JpegPipeline::HasCustomCover(game.title_id);
            game.icon_loaded = false;
            game.current_icon_texture = nullptr;

            m_games.push_back(std::move(game));
        }

        entry_offset += out_count;
    }

    // Sort games alphabetically by name
    std::sort(m_games.begin(), m_games.end(), [](const GameItem& a, const GameItem& b) {
        return a.name < b.name;
    });

    // Scan import directory for matching filenames
    ScanImportDirectory(renderer);

    return true;
}

} // namespace BoxArt
