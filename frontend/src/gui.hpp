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

#if defined(__SWITCH__)
// Nintendo Switch controller mapping:
// Physical A (East)  = SDL_CONTROLLER_BUTTON_B
// Physical B (South) = SDL_CONTROLLER_BUTTON_A
// Physical X (North) = SDL_CONTROLLER_BUTTON_Y
// Physical Y (West)  = SDL_CONTROLLER_BUTTON_X
constexpr Uint8 BTN_A = SDL_CONTROLLER_BUTTON_B;
constexpr Uint8 BTN_B = SDL_CONTROLLER_BUTTON_A;
constexpr Uint8 BTN_X = SDL_CONTROLLER_BUTTON_Y;
constexpr Uint8 BTN_Y = SDL_CONTROLLER_BUTTON_X;
#else
constexpr Uint8 BTN_A = SDL_CONTROLLER_BUTTON_A;
constexpr Uint8 BTN_B = SDL_CONTROLLER_BUTTON_B;
constexpr Uint8 BTN_X = SDL_CONTROLLER_BUTTON_X;
constexpr Uint8 BTN_Y = SDL_CONTROLLER_BUTTON_Y;
#endif

enum class ModalType {
    None,
    ManageCover,
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
    void RenderSplash(const char* message, float progress);
    void RenderMainUI();
    void RenderHeader();
    void RenderCarousel();
    void RenderGameInfo(const GameItem& game);
    void RenderBottomBar();

    void RenderManageCoverModal();
    void RenderImportChooserModal();
    void RenderSteamGridDBSearchModal();
    void RenderSteamGridDBGridsModal();
    void RenderApiKeyModal();
    void RenderMessageModal();

    void NavigateCarousel(int delta);
    void OpenSteamGridDBSearch();

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
    bool m_stick_centered = true;
    uint32_t m_stick_move_time = 0;

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

    bool m_is_shutdown = false;
};

} // namespace BoxArt
