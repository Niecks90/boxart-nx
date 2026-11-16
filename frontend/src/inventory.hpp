/*
 * Copyright (c) 2026 boxart-nx contributors
 * Distributed under the terms of the GNU General Public License v2
 */

#pragma once

#include <switch.h>
#include <cstdint>
#include <string>
#include <vector>
#include <memory>
#include <functional>

namespace BoxArt {

struct ImportFileItem {
    std::string filename;
    std::string full_path;
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

    // Matched import file if found in import directory
    std::string matched_import_path;
};

class Inventory {
public:
    Inventory() = default;
    ~Inventory();

    // Reloads all installed applications from NS (fast metadata scan, no upfront JPEG decoding)
    bool ReloadGames(std::function<void(int current, int total)> progress_cb = nullptr);

    // Scans sdmc:/config/boxart-nx/import/ for PNG, JPG, WebP images (filenames only)
    bool ScanImportDirectory();

    std::vector<GameItem>& GetGames() { return m_games; }
    const std::vector<GameItem>& GetGames() const { return m_games; }

    std::vector<ImportFileItem>& GetImportFiles() { return m_import_files; }
    const std::vector<ImportFileItem>& GetImportFiles() const { return m_import_files; }

    void Clear();

private:
    std::vector<GameItem> m_games;
    std::vector<ImportFileItem> m_import_files;
};

} // namespace BoxArt
