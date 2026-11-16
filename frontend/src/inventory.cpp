/*
 * Copyright (c) 2026 boxart-nx contributors
 * Distributed under the terms of the GNU General Public License v2
 */

#include "inventory.hpp"
#include "jpeg_pipeline.hpp"

#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <algorithm>
#include <cstdio>
#include <cstring>

namespace BoxArt {

Inventory::~Inventory() {
    Clear();
}

void Inventory::Clear() {
    m_games.clear();
    m_import_files.clear();
}

static std::string ToLower(const std::string& str) {
    std::string lower = str;
    std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char c) { return std::tolower(c); });
    return lower;
}

bool Inventory::ScanImportDirectory() {
    m_import_files.clear();

    const char* dir_path = "sdmc:/config/boxart-nx/import";
    DIR* d = opendir(dir_path);
    if (!d) {
        mkdir("sdmc:/config", 0777);
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

bool Inventory::ReloadGames(std::function<void(int current, int total)> progress_cb) {
    Clear();

    // First collect all valid application records
    std::vector<NsApplicationRecord> all_records;
    s32 entry_offset = 0;
    while (true) {
        NsApplicationRecord records[32];
        s32 out_count = 0;
        Result rc = nsListApplicationRecord(records, 32, entry_offset, &out_count);
        if (R_FAILED(rc) || out_count <= 0) {
            break;
        }

        for (s32 i = 0; i < out_count; ++i) {
            if (records[i].application_id != 0) {
                all_records.push_back(records[i]);
            }
        }

        entry_offset += out_count;
    }

    const int total = static_cast<int>(all_records.size());
    if (progress_cb) {
        progress_cb(0, total);
    }

    auto control_data = std::make_unique<NsApplicationControlData>();

    for (int i = 0; i < total; ++i) {
        uint64_t tid = all_records[i].application_id;

        GameItem game;
        game.title_id = tid;

        char hex_buf[32];
        std::snprintf(hex_buf, sizeof(hex_buf), "%016llx", (unsigned long long)tid);
        game.title_id_hex = hex_buf;

        u64 actual_size = 0;
        std::memset(control_data.get(), 0, sizeof(NsApplicationControlData));
        Result rc = nsGetApplicationControlData(NsApplicationControlSource_Storage, tid, control_data.get(), sizeof(NsApplicationControlData), &actual_size);

        if (R_SUCCEEDED(rc)) {
            NacpLanguageEntry* lang_entry = nullptr;
            rc = nsGetApplicationDesiredLanguage(&control_data->nacp, &lang_entry);
            if (R_SUCCEEDED(rc) && lang_entry && lang_entry->name[0] != '\0') {
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

        game.has_custom_cover = JpegPipeline::HasCustomCover(game.title_id);
        m_games.push_back(std::move(game));

        if (progress_cb) {
            progress_cb(i + 1, total);
        }
    }

    // Sort games alphabetically by name
    std::sort(m_games.begin(), m_games.end(), [](const GameItem& a, const GameItem& b) {
        return a.name < b.name;
    });

    // Scan import directory for matching filenames
    ScanImportDirectory();

    return true;
}

} // namespace BoxArt
