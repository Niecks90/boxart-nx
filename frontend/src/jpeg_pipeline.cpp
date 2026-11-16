/*
 * Copyright (c) 2026 boxart-nx contributors
 * Distributed under the terms of the GNU General Public License v2
 */

#include "jpeg_pipeline.hpp"

#define STBI_NO_STDIO
#include <stb/stb_image.h>

#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include <stb/stb_image_resize2.h>

#include <webp/decode.h>
#include <turbojpeg.h>

#include <cstdio>
#include <cstring>
#include <algorithm>
#include <sys/stat.h>
#include <unistd.h>

namespace BoxArt {

static void EnsureDirectory(const std::string& path) {
    mkdir(path.c_str(), 0777);
}

static void EnsureConfigTree() {
    EnsureDirectory("sdmc:/config");
    EnsureDirectory("sdmc:/config/boxart-nx");
    EnsureDirectory("sdmc:/config/boxart-nx/covers");
    EnsureDirectory("sdmc:/config/boxart-nx/import");
}

std::string JpegPipeline::GetCoverPath(uint64_t title_id) {
    char buf[128];
    std::snprintf(buf, sizeof(buf), "sdmc:/config/boxart-nx/covers/%016llx.jpg", (unsigned long long)title_id);
    return std::string(buf);
}

bool JpegPipeline::HasCustomCover(uint64_t title_id) {
    std::string path = GetCoverPath(title_id);
    return (access(path.c_str(), F_OK) == 0);
}

bool JpegPipeline::RevertCustomCover(uint64_t title_id, std::string& out_error) {
    std::string path = GetCoverPath(title_id);
    if (access(path.c_str(), F_OK) != 0) {
        return true; // Already not present
    }
    if (unlink(path.c_str()) != 0) {
        out_error = "Failed to delete: " + path;
        return false;
    }
    return true;
}

bool JpegPipeline::EncodeNormalizedJpeg(const uint8_t* rgba_pixels, int width, int height, std::vector<uint8_t>& out_jpeg, std::string& out_error) {
    if (!rgba_pixels || width <= 0 || height <= 0) {
        out_error = "Invalid input dimensions";
        return false;
    }

    // 1. Center crop to square
    int crop_size = std::min(width, height);
    int crop_x = (width - crop_size) / 2;
    int crop_y = (height - crop_size) / 2;

    std::vector<uint8_t> cropped(crop_size * crop_size * 4);
    for (int y = 0; y < crop_size; ++y) {
        const uint8_t* src_row = rgba_pixels + ((crop_y + y) * width + crop_x) * 4;
        uint8_t* dst_row = cropped.data() + (y * crop_size) * 4;
        std::memcpy(dst_row, src_row, crop_size * 4);
    }

    // 2. Resize to 256x256 using stbir
    std::vector<uint8_t> resized(TARGET_WIDTH * TARGET_HEIGHT * 4);
    unsigned char* res = stbir_resize_uint8_linear(
        cropped.data(), crop_size, crop_size, crop_size * 4,
        resized.data(), TARGET_WIDTH, TARGET_HEIGHT, TARGET_WIDTH * 4,
        STBIR_RGBA
    );
    if (!res) {
        out_error = "stbir_resize failed";
        return false;
    }

    // 3. Compress with libjpeg-turbo (Baseline 4:2:0, size <= 128KB loop)
    tjhandle tj = tjInitCompress();
    if (!tj) {
        out_error = "tjInitCompress failed";
        return false;
    }

    int quality = 90;
    bool success = false;
    unsigned char* jpeg_buf = nullptr;
    unsigned long jpeg_size = 0;

    while (quality >= 10) {
        jpeg_buf = nullptr;
        jpeg_size = 0;

        int rc = tjCompress2(
            tj,
            resized.data(),
            TARGET_WIDTH,
            TARGET_WIDTH * 4,
            TARGET_HEIGHT,
            TJPF_RGBA,
            &jpeg_buf,
            &jpeg_size,
            TJSAMP_420,
            quality,
            TJFLAG_ACCURATEDCT
        );

        if (rc == 0 && jpeg_size > 0 && jpeg_size <= MAX_COVER_SIZE) {
            out_jpeg.resize(jpeg_size);
            std::memcpy(out_jpeg.data(), jpeg_buf, jpeg_size);
            tjFree(jpeg_buf);
            jpeg_buf = nullptr;
            success = true;
            break;
        }

        if (jpeg_buf) {
            tjFree(jpeg_buf);
            jpeg_buf = nullptr;
        }

        quality -= 5;
    }

    tjDestroy(tj);

    if (!success) {
        out_error = "Could not compress cover under 128KB even at quality 10";
        return false;
    }

    return true;
}

bool JpegPipeline::ProcessAndSaveFromMemory(uint64_t title_id, const uint8_t* data, size_t size, std::string& out_error) {
    if (!data || size == 0) {
        out_error = "Empty input buffer";
        return false;
    }

    uint8_t* raw_rgba = nullptr;
    int w = 0, h = 0;
    bool is_webp = false;

    // Check if WebP format
    if (WebPGetInfo(data, size, &w, &h)) {
        raw_rgba = WebPDecodeRGBA(data, size, &w, &h);
        is_webp = true;
    }

    if (!raw_rgba) {
        int channels = 0;
        raw_rgba = stbi_load_from_memory(data, static_cast<int>(size), &w, &h, &channels, 4);
    }

    if (!raw_rgba) {
        out_error = "Failed to decode image data (unsupported or corrupted format)";
        return false;
    }

    std::vector<uint8_t> out_jpeg;
    bool ok = EncodeNormalizedJpeg(raw_rgba, w, h, out_jpeg, out_error);

    if (is_webp) {
        WebPFree(raw_rgba);
    } else {
        stbi_image_free(raw_rgba);
    }

    if (!ok) {
        return false;
    }

    EnsureConfigTree();
    std::string out_path = GetCoverPath(title_id);
    FILE* f = std::fopen(out_path.c_str(), "wb");
    if (!f) {
        out_error = "Failed to open destination file: " + out_path;
        return false;
    }

    size_t written = std::fwrite(out_jpeg.data(), 1, out_jpeg.size(), f);
    std::fclose(f);

    if (written != out_jpeg.size()) {
        out_error = "Incomplete file write to " + out_path;
        unlink(out_path.c_str());
        return false;
    }

    return true;
}

bool JpegPipeline::ProcessAndSaveFromFile(uint64_t title_id, const std::string& input_path, std::string& out_error) {
    FILE* f = std::fopen(input_path.c_str(), "rb");
    if (!f) {
        out_error = "Failed to open input file: " + input_path;
        return false;
    }

    std::fseek(f, 0, SEEK_END);
    long size = std::ftell(f);
    std::fseek(f, 0, SEEK_SET);

    if (size <= 0 || size > 50 * 1024 * 1024) { // 50MB max sanity check
        std::fclose(f);
        out_error = "Invalid file size for: " + input_path;
        return false;
    }

    std::vector<uint8_t> buffer(size);
    size_t read_bytes = std::fread(buffer.data(), 1, size, f);
    std::fclose(f);

    if (read_bytes != static_cast<size_t>(size)) {
        out_error = "Failed to read file: " + input_path;
        return false;
    }

    return ProcessAndSaveFromMemory(title_id, buffer.data(), buffer.size(), out_error);
}

} // namespace BoxArt
