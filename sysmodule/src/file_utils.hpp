/*
 * Copyright (c) 2026 boxart-nx contributors
 * Distributed under the terms of the GNU General Public License v2
 */

#pragma once

#include "libams.hpp"

#define BOXART_CONFIG_DIR "sdmc:/config/boxart-nx"
#define BOXART_COVERS_DIR "sdmc:/config/boxart-nx/covers"
#define BOXART_LOG_FILE   "sdmc:/config/boxart-nx/mitm.log"

class FileUtils {
public:
    static ams::Result Initialize();
    static bool IsInitialized();
    static void Exit();
    static void LogLine(const char* format, ...) __attribute__((format(printf, 1, 2)));
};
