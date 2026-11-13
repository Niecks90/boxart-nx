/*
 * Copyright (c) 2026 boxart-nx contributors
 * Distributed under the terms of the GNU General Public License v2
 */

#include <switch.h>
#include "gui.hpp"
#include <cstdio>

int main(int argc, char* argv[]) {
    // Initialize libnx services
    bool socket_initialized = false;
    Result rc = socketInitializeDefault();
    if (R_SUCCEEDED(rc)) {
        socket_initialized = true;
    }

    rc = nsInitialize();
    if (R_FAILED(rc)) {
        if (socket_initialized) socketExit();
        return 1;
    }

    // Run GUI application
    {
        BoxArt::Gui gui;
        if (gui.Initialize()) {
            gui.Run();
        }
        gui.Shutdown();
    }

    // Exit libnx services
    nsExit();
    if (socket_initialized) {
        socketExit();
    }

    return 0;
}
