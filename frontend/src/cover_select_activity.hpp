/*
 * Copyright (c) 2026 boxart-nx contributors
 * Distributed under the terms of the GNU General Public License v2
 */

#pragma once

#include <borealis.hpp>
#include "inventory.hpp"
#include "steamgriddb.hpp"

namespace BoxArt {

class CoverSelectActivity : public brls::Activity {
public:
    CoverSelectActivity(const GameItem& game, std::function<void()> on_applied = nullptr);
    ~CoverSelectActivity() override = default;

private:
    brls::View* CreateSteamGridTab();
    brls::View* CreateSdImportTab();

    void SearchSteamGridDB();
    void ApplyCoverFromMemory(const std::string& url);
    void ApplyCoverFromFile(const std::string& path);

    GameItem m_game;
    std::function<void()> m_on_applied;
    SteamGridDBClient m_steam_client;

    std::string m_search_query;
    brls::Box* m_steam_results_container = nullptr;
    brls::Label* m_search_label = nullptr;

    // SD import tab state
    std::string m_selected_import_path;
    brls::Image* m_sd_preview_image = nullptr;
    brls::Label* m_sd_preview_label = nullptr;
};

} // namespace BoxArt
