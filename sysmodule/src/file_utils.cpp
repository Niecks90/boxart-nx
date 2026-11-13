/*
 * Copyright (c) 2026 boxart-nx contributors
 * Distributed under the terms of the GNU General Public License v2
 */

#include "file_utils.hpp"
#include <cstdarg>
#include <atomic>

static ams::os::Mutex g_log_mutex{false};
static std::atomic_bool g_initialized = false;
static s64 g_log_offset = 0;

bool FileUtils::IsInitialized() {
    return g_initialized.load();
}

ams::Result FileUtils::Initialize() {
    // Ensure configuration directories exist
    R_TRY(ams::fs::EnsureDirectory("sdmc:/config"));
    R_TRY(ams::fs::EnsureDirectory(BOXART_CONFIG_DIR));
    R_TRY(ams::fs::EnsureDirectory(BOXART_COVERS_DIR));

    bool has_log = false;
    R_TRY(ams::fs::HasFile(&has_log, BOXART_LOG_FILE));
    if (!has_log) {
        R_TRY(ams::fs::CreateFile(BOXART_LOG_FILE, 0));
    }

    ams::fs::FileHandle file;
    R_TRY(ams::fs::OpenFile(&file, BOXART_LOG_FILE, ams::fs::OpenMode_Write | ams::fs::OpenMode_AllowAppend));
    R_TRY(ams::fs::GetFileSize(&g_log_offset, file));
    ams::fs::CloseFile(file);

    g_initialized = true;

    LogLine("[boxart-nx] Sysmodule initialized. SD logging started.");
    return ams::ResultSuccess();
}

void FileUtils::Exit() {
    g_initialized = false;
}

void FileUtils::LogLine(const char* format, ...) {
    if (!g_initialized.load()) {
        return;
    }

    std::va_list args;
    va_start(args, format);

    std::scoped_lock lock(g_log_mutex);
    ams::fs::FileHandle file;
    if (R_SUCCEEDED(ams::fs::OpenFile(&file, BOXART_LOG_FILE, ams::fs::OpenMode_Write | ams::fs::OpenMode_AllowAppend))) {
        char buffer[512];
        int len = ams::util::TVSNPrintf(buffer, sizeof(buffer), format, args);
        if (len > 0) {
            ams::fs::WriteFile(file, g_log_offset, buffer, len, ams::fs::WriteOption::Flush);
            g_log_offset += len;
            ams::fs::WriteFile(file, g_log_offset, "\n", 1, ams::fs::WriteOption::Flush);
            g_log_offset += 1;
        }
        ams::fs::CloseFile(file);
    }

    va_end(args);
}
