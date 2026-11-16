/*
 * Copyright (c) 2026 boxart-nx contributors
 * Distributed under the terms of the GNU General Public License v2
 */

#pragma once

#include <switch.h>
#include <SDL.h>
#include <cstdint>
#include <string>
#include <vector>
#include <memory>

namespace BoxArt {

struct ImportFileItem {
    std::string filename;
    std::string full_path;
    SDL_Texture* thumbnail_texture = nullptr;
};

struct GameItem {
    uint64_t title_id = 0;
    std::string title_id_hex;
    std::string name;
    std::string author;
    std::string version;

    bool has_custom_cover = false;

    // Original icon raw bytes from NsApplicationControlData (128 KB max)
    std::vector<uint8_t> original_icon_jpeg;

    // Current displayed icon texture (lazy-loaded when item is visible)
    SDL_Texture* current_icon_texture = nullptr;
    bool icon_loaded = false;

    // Matched import file if found in import directory
    std::string matched_import_path;

    void FreeTextures();
};

class Inventory {
public:
    Inventory() = default;
    ~Inventory();

    // Reloads all installed applications from NS (fast metadata scan, no upfront JPEG decoding)
    bool ReloadGames(SDL_Renderer* renderer, std::string& out_error);

    // Scans sdmc:/config/boxart-nx/import/ for PNG, JPG, WebP images (filenames only)
    bool ScanImportDirectory(SDL_Renderer* renderer);

    // Lazy loads icon texture for a specific game when visible
    void EnsureIconLoaded(SDL_Renderer* renderer, GameItem& item);

    // Force reload cover status and texture for a specific game (e.g. after apply/restore)
    void RefreshGameTextures(SDL_Renderer* renderer, GameItem& item);

    std::vector<GameItem>& GetGames() { return m_games; }
    std::vector<ImportFileItem>& GetImportFiles() { return m_import_files; }

    void Clear();

private:
    std::vector<GameItem> m_games;
    std::vector<ImportFileItem> m_import_files;
};

// Texture creation helpers
SDL_Texture* CreateTextureFromMemory(SDL_Renderer* renderer, const uint8_t* data, size_t size);
SDL_Texture* CreateTextureFromFile(SDL_Renderer* renderer, const std::string& path);

} // namespace BoxArt
