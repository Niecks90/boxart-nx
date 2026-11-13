/*
 * Copyright (c) 2026 boxart-nx contributors
 * Distributed under the terms of the GNU General Public License v2
 */

#pragma once

#include <cstdint>
#include <cstddef>
#include <string>
#include <vector>

namespace BoxArt {

class JpegPipeline {
public:
    static constexpr size_t MAX_COVER_SIZE = 0x20000; // 131,072 bytes (128 KB)
    static constexpr int TARGET_WIDTH = 256;
    static constexpr int TARGET_HEIGHT = 256;

    // Normalizes an image from a file path (PNG, JPG, BMP, WebP) and writes to sdmc:/config/boxart-nx/covers/<TitleID>.jpg
    static bool ProcessAndSaveFromFile(uint64_t title_id, const std::string& input_path, std::string& out_error);

    // Normalizes an image from a memory buffer (e.g. downloaded from SteamGridDB) and writes to sdmc:/config/boxart-nx/covers/<TitleID>.jpg
    static bool ProcessAndSaveFromMemory(uint64_t title_id, const uint8_t* data, size_t size, std::string& out_error);

    // Encodes a raw RGBA buffer into baseline 4:2:0 JPEG <= 128KB
    static bool EncodeNormalizedJpeg(const uint8_t* rgba_pixels, int width, int height, std::vector<uint8_t>& out_jpeg, std::string& out_error);

    // Helper to get destination cover path: sdmc:/config/boxart-nx/covers/<016llx>.jpg
    static std::string GetCoverPath(uint64_t title_id);

    // Checks if a custom cover file exists for this title ID
    static bool HasCustomCover(uint64_t title_id);

    // Deletes the custom cover file to revert to original
    static bool RevertCustomCover(uint64_t title_id, std::string& out_error);
};

} // namespace BoxArt
