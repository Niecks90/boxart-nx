/*
 * Copyright (c) 2026 boxart-nx contributors
 * Distributed under the terms of the GNU General Public License v2
 */

#pragma once

#include "inventory.hpp"
#include "steamgriddb.hpp"
#include <SDL.h>
#include <string>
#include <vector>

namespace BoxArt {

enum class ModalType {
    None,
    ImportChooser,
    SteamGridDBSearch,
    SteamGridDBGrids,
    ApiKeyInput,
    MessagePopup
};

class Gui {
public:
    Gui();
    ~Gui();

    bool Initialize();
    void Run();
    void Shutdown();

private:
    void RenderMainUI();
    void RenderGameList(float width, float height);
    void RenderGameDetails(float width, float height);
    void RenderImportChooserModal();
    void RenderSteamGridDBSearchModal();
    void RenderSteamGridDBGridsModal();
    void RenderApiKeyModal();
    void RenderMessageModal();

    void ApplyCoverFromFile(GameItem& game, const std::string& path);
    void ApplyCoverFromMemory(GameItem& game, const uint8_t* data, size_t size);
    void RestoreOriginalCover(GameItem& game);

    void ShowMessage(const std::string& title, const std::string& message);

    SDL_Window* m_window = nullptr;
    SDL_Renderer* m_renderer = nullptr;
    SDL_GameController* m_controller = nullptr;

    Inventory m_inventory;
    SteamGridDBClient m_steam_client;

    int m_selected_game_index = 0;
    char m_search_filter[64] = {0};

    // Modal states
    ModalType m_active_modal = ModalType::None;
    std::string m_modal_title;
    std::string m_modal_message;

    // SteamGridDB search states
    char m_steam_search_query[128] = {0};
    std::vector<SteamGameMatch> m_steam_matches;
    int m_selected_steam_match = -1;
    std::vector<SteamGridResult> m_steam_grids;
    int m_selected_steam_grid = -1;
    bool m_steam_loading = false;
    std::string m_steam_status;

    // Status notification
    std::string m_status_banner;
    uint32_t m_status_banner_time = 0;
};

} // namespace BoxArt
