/*
 * Copyright (c) 2026 boxart-nx contributors
 * Distributed under the terms of the GNU General Public License v2
 */

#include <switch.h>
#include <borealis.hpp>
#include "main_activity.hpp"

int main(int argc, char* argv[]) {
    // Initialize NS service for application metadata access
    Result rc = nsInitialize();
    if (R_FAILED(rc)) {
        brls::Logger::error("Failed to initialize ns service: {:#x}", rc);
        return 1;
    }

    // Set Borealis logging
    brls::Logger::setLogLevel(brls::LogLevel::LOG_INFO);

    // Initialize Borealis framework
    if (!brls::Application::init()) {
        brls::Logger::error("Unable to initialize Borealis");
        nsExit();
        return 1;
    }

    // Push Main Horizon OS Activity
    brls::Application::pushActivity(new BoxArt::MainActivity());

    // Main event loop
    while (brls::Application::mainLoop())
        ;

    // Clean exit
    nsExit();
    return 0;
}
