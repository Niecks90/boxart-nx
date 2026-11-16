/*
 * Copyright (c) 2026 boxart-nx contributors
 * Distributed under the terms of the GNU General Public License v2
 */

#include "gui.hpp"
#include "jpeg_pipeline.hpp"

#include <imgui.h>
#include "imgui_impl_sdl2.h"
#include "imgui_impl_sdlrenderer2.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace BoxArt {

static void SetupHorizonStyle() {
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding    = 10.0f;
    style.ChildRounding     = 8.0f;
    style.FrameRounding     = 8.0f;
    style.PopupRounding     = 10.0f;
    style.ScrollbarRounding = 6.0f;
    style.GrabRounding      = 6.0f;
    style.WindowBorderSize  = 1.0f;
    style.FrameBorderSize   = 0.0f;
    style.ItemSpacing       = ImVec2(10.0f, 8.0f);
    style.ItemInnerSpacing  = ImVec2(8.0f, 6.0f);
    style.WindowPadding     = ImVec2(16.0f, 16.0f);

    ImVec4* colors = style.Colors;
    colors[ImGuiCol_WindowBg]             = ImVec4(0.09f, 0.10f, 0.12f, 1.00f);
    colors[ImGuiCol_ChildBg]              = ImVec4(0.12f, 0.13f, 0.16f, 1.00f);
    colors[ImGuiCol_PopupBg]              = ImVec4(0.11f, 0.12f, 0.15f, 0.98f);
    colors[ImGuiCol_Border]               = ImVec4(0.22f, 0.25f, 0.30f, 0.70f);
    colors[ImGuiCol_FrameBg]              = ImVec4(0.16f, 0.18f, 0.22f, 1.00f);
    colors[ImGuiCol_FrameBgHovered]       = ImVec4(0.22f, 0.26f, 0.32f, 1.00f);
    colors[ImGuiCol_FrameBgActive]        = ImVec4(0.26f, 0.32f, 0.40f, 1.00f);
    colors[ImGuiCol_TitleBg]              = ImVec4(0.08f, 0.09f, 0.11f, 1.00f);
    colors[ImGuiCol_TitleBgActive]        = ImVec4(0.0f, 0.45f, 0.70f, 1.00f);
    colors[ImGuiCol_Button]               = ImVec4(0.15f, 0.28f, 0.42f, 1.00f);
    colors[ImGuiCol_ButtonHovered]        = ImVec4(0.0f, 0.55f, 0.85f, 1.00f);
    colors[ImGuiCol_ButtonActive]         = ImVec4(0.0f, 0.65f, 0.95f, 1.00f);
    colors[ImGuiCol_Header]               = ImVec4(0.18f, 0.35f, 0.55f, 0.65f);
    colors[ImGuiCol_HeaderHovered]        = ImVec4(0.22f, 0.45f, 0.70f, 0.85f);
    colors[ImGuiCol_HeaderActive]         = ImVec4(0.0f, 0.60f, 0.90f, 1.00f);
    colors[ImGuiCol_ScrollbarBg]          = ImVec4(0.07f, 0.08f, 0.10f, 0.60f);
    colors[ImGuiCol_ScrollbarGrab]        = ImVec4(0.24f, 0.28f, 0.34f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.32f, 0.38f, 0.46f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive]  = ImVec4(0.40f, 0.48f, 0.58f, 1.00f);
    colors[ImGuiCol_Text]                 = ImVec4(0.92f, 0.94f, 0.96f, 1.00f);
    colors[ImGuiCol_TextDisabled]         = ImVec4(0.50f, 0.56f, 0.62f, 1.00f);
    colors[ImGuiCol_NavHighlight]         = ImVec4(0.0f, 0.75f, 1.00f, 1.00f);
}

Gui::Gui() {
}

Gui::~Gui() {
    Shutdown();
}

void Gui::RenderSplash(const char* message, float progress) {
    ImGui_ImplSDLRenderer2_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();

    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImVec2(1280, 720));
    ImGui::Begin("##SplashWindow", nullptr,
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoBringToFrontOnFocus
    );

    ImDrawList* draw = ImGui::GetWindowDrawList();
    draw->AddRectFilled(ImVec2(0, 0), ImVec2(1280, 720), IM_COL32(18, 20, 24, 255));

    // Logo & title
    const char* title = "boxart-nx";
    ImVec2 title_sz = ImGui::CalcTextSize(title);
    float cx = 640.0f;
    float cy = 320.0f;

    draw->AddText(ImGui::GetFont(), ImGui::GetFontSize() * 2.2f, ImVec2(cx - title_sz.x * 1.1f, cy - 70), IM_COL32(0, 200, 255, 255), title);

    const char* sub = "Nintendo Switch Home Menu Icon Manager";
    ImVec2 sub_sz = ImGui::CalcTextSize(sub);
    draw->AddText(ImGui::GetFont(), ImGui::GetFontSize() * 1.05f, ImVec2(cx - sub_sz.x * 0.52f, cy - 10), IM_COL32(170, 180, 195, 255), sub);

    // Status message
    ImVec2 msg_sz = ImGui::CalcTextSize(message);
    draw->AddText(ImGui::GetFont(), ImGui::GetFontSize(), ImVec2(cx - msg_sz.x * 0.5f, cy + 40), IM_COL32(220, 225, 235, 255), message);

    // Progress bar
    float bar_w = 420.0f;
    float bar_h = 6.0f;
    float bar_x = cx - bar_w * 0.5f;
    float bar_y = cy + 75.0f;
    draw->AddRectFilled(ImVec2(bar_x, bar_y), ImVec2(bar_x + bar_w, bar_y + bar_h), IM_COL32(40, 45, 55, 255), 3.0f);
    if (progress > 0.0f) {
        float fill_w = std::clamp(progress, 0.0f, 1.0f) * bar_w;
        draw->AddRectFilled(ImVec2(bar_x, bar_y), ImVec2(bar_x + fill_w, bar_y + bar_h), IM_COL32(0, 200, 255, 255), 3.0f);
    }

    ImGui::End();

    ImGui::Render();
    SDL_SetRenderDrawColor(m_renderer, 18, 20, 24, 255);
    SDL_RenderClear(m_renderer);
    ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), m_renderer);
    SDL_RenderPresent(m_renderer);
}

bool Gui::Initialize() {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER) < 0) {
        return false;
    }

    m_window = SDL_CreateWindow(
        "boxart-nx",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        1280,
        720,
        SDL_WINDOW_SHOWN
    );
    if (!m_window) return false;

    m_renderer = SDL_CreateRenderer(
        m_window,
        -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC
    );
    if (!m_renderer) return false;

    // Immediately present a blank dark frame on screen so it never sits on the OS black buffer
    SDL_SetRenderDrawColor(m_renderer, 18, 20, 24, 255);
    SDL_RenderClear(m_renderer);
    SDL_RenderPresent(m_renderer);

    // Open first game controller if available
    for (int i = 0; i < SDL_NumJoysticks(); ++i) {
        if (SDL_IsGameController(i)) {
            m_controller = SDL_GameControllerOpen(i);
            if (m_controller) break;
        }
    }

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
    io.IniFilename = nullptr; // Disable imgui.ini writing to SD

    SetupHorizonStyle();

    // Setup Platform/Renderer backends
    ImGui_ImplSDL2_InitForSDLRenderer(m_window, m_renderer);
    ImGui_ImplSDLRenderer2_Init(m_renderer);

    // Show initial splash frame immediately on frame 1
    RenderSplash("Scanning installed games...", 0.25f);

    // Perform fast metadata scan (zero JPEG decoding on boot)
    std::string err;
    m_inventory.ReloadGames(m_renderer, err);

    // Finalize splash before transitioning to main carousel
    RenderSplash("Loading completed!", 1.0f);

    return true;
}

void Gui::Shutdown() {
    if (m_is_shutdown) return;
    m_is_shutdown = true;

    for (auto& grid : m_steam_grids) {
        grid.FreeTexture();
    }
    m_steam_grids.clear();

    m_inventory.Clear();

    ImGui_ImplSDLRenderer2_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    if (m_controller) {
        SDL_GameControllerClose(m_controller);
        m_controller = nullptr;
    }

    if (m_renderer) {
        SDL_DestroyRenderer(m_renderer);
        m_renderer = nullptr;
    }

    if (m_window) {
        SDL_DestroyWindow(m_window);
        m_window = nullptr;
    }

    SDL_Quit();
}

void Gui::NavigateCarousel(int delta) {
    auto& games = m_inventory.GetGames();
    if (games.empty()) return;

    int new_index = m_selected_game_index + delta;
    if (new_index < 0) new_index = 0;
    if (new_index >= (int)games.size()) new_index = (int)games.size() - 1;
    m_selected_game_index = new_index;
}

void Gui::OpenSteamGridDBSearch() {
    auto& games = m_inventory.GetGames();
    if (m_selected_game_index < 0 || m_selected_game_index >= (int)games.size()) return;

    std::snprintf(m_steam_search_query, sizeof(m_steam_search_query), "%s", games[m_selected_game_index].name.c_str());
    m_steam_matches.clear();
    for (auto& g : m_steam_grids) g.FreeTexture();
    m_steam_grids.clear();
    m_selected_steam_match = -1;
    m_selected_steam_grid = -1;
    m_steam_status.clear();

    if (!m_steam_client.HasApiKey()) {
        m_active_modal = ModalType::ApiKeyInput;
    } else {
        m_active_modal = ModalType::SteamGridDBSearch;
        std::string err;
        if (m_steam_client.SearchGames(m_steam_search_query, m_steam_matches, err)) {
            m_steam_status = "Found " + std::to_string(m_steam_matches.size()) + " matches.";
        } else {
            m_steam_status = "Search error: " + err;
        }
    }
}

void Gui::ShowMessage(const std::string& title, const std::string& message) {
    m_modal_title = title;
    m_modal_message = message;
    m_active_modal = ModalType::MessagePopup;
}

void Gui::ApplyCoverFromFile(GameItem& game, const std::string& path) {
    std::string err;
    if (JpegPipeline::ProcessAndSaveFromFile(game.title_id, path, err)) {
        m_inventory.RefreshGameTextures(m_renderer, game);
        m_status_banner = "Custom cover applied for " + game.name + "!";
        m_status_banner_time = SDL_GetTicks();
    } else {
        ShowMessage("Error applying cover", err);
    }
}

void Gui::ApplyCoverFromMemory(GameItem& game, const uint8_t* data, size_t size) {
    std::string err;
    if (JpegPipeline::ProcessAndSaveFromMemory(game.title_id, data, size, err)) {
        m_inventory.RefreshGameTextures(m_renderer, game);
        m_status_banner = "SteamGridDB cover applied for " + game.name + "!";
        m_status_banner_time = SDL_GetTicks();
    } else {
        ShowMessage("Error applying cover", err);
    }
}

void Gui::RestoreOriginalCover(GameItem& game) {
    std::string err;
    if (JpegPipeline::RevertCustomCover(game.title_id, err)) {
        m_inventory.RefreshGameTextures(m_renderer, game);
        m_status_banner = "Restored official NAND icon for " + game.name + "!";
        m_status_banner_time = SDL_GetTicks();
    } else {
        ShowMessage("Error reverting cover", err);
    }
}

void Gui::Run() {
    bool running = true;

    while (appletMainLoop() && running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL2_ProcessEvent(&event);

            if (event.type == SDL_QUIT) {
                running = false;
            } else if (event.type == SDL_CONTROLLERBUTTONDOWN) {
                Uint8 btn = event.cbutton.button;
                if (btn == SDL_CONTROLLER_BUTTON_START) {
                    // Plus button (+) -> Exit safely
                    running = false;
                } else if (btn == SDL_CONTROLLER_BUTTON_BACK) {
                    // Minus button (-) -> Settings / API Key modal
                    m_active_modal = ModalType::ApiKeyInput;
                } else if (btn == BTN_B) {
                    // Physical B -> Cancel / Go Back
                    if (m_active_modal == ModalType::SteamGridDBGrids) {
                        m_active_modal = ModalType::SteamGridDBSearch;
                    } else if (m_active_modal != ModalType::None) {
                        m_active_modal = ModalType::None;
                    }
                } else if (m_active_modal == ModalType::None) {
                    // Main carousel controls
                    if (btn == BTN_A) {
                        if (!m_inventory.GetGames().empty()) {
                            m_active_modal = ModalType::ManageCover;
                        }
                    } else if (btn == BTN_X) {
                        OpenSteamGridDBSearch();
                    } else if (btn == BTN_Y) {
                        auto& games = m_inventory.GetGames();
                        if (m_selected_game_index >= 0 && m_selected_game_index < (int)games.size()) {
                            RestoreOriginalCover(games[m_selected_game_index]);
                        }
                    } else if (btn == SDL_CONTROLLER_BUTTON_DPAD_LEFT) {
                        NavigateCarousel(-1);
                    } else if (btn == SDL_CONTROLLER_BUTTON_DPAD_RIGHT) {
                        NavigateCarousel(1);
                    } else if (btn == SDL_CONTROLLER_BUTTON_LEFTSHOULDER) {
                        NavigateCarousel(-5);
                    } else if (btn == SDL_CONTROLLER_BUTTON_RIGHTSHOULDER) {
                        NavigateCarousel(5);
                    }
                }
            }
        }

        // Smooth analog stick navigation on carousel
        if (m_active_modal == ModalType::None && m_controller) {
            Sint16 axis_x = SDL_GameControllerGetAxis(m_controller, SDL_CONTROLLER_AXIS_LEFTX);
            uint32_t now = SDL_GetTicks();
            if (axis_x < -18000) {
                if (m_stick_centered || (now - m_stick_move_time > 220)) {
                    NavigateCarousel(-1);
                    m_stick_centered = false;
                    m_stick_move_time = now;
                }
            } else if (axis_x > 18000) {
                if (m_stick_centered || (now - m_stick_move_time > 220)) {
                    NavigateCarousel(1);
                    m_stick_centered = false;
                    m_stick_move_time = now;
                }
            } else if (std::abs(axis_x) < 8000) {
                m_stick_centered = true;
            }
        }

        // Dear ImGui new frame
        ImGui_ImplSDLRenderer2_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        RenderMainUI();

        // Render any active modal
        switch (m_active_modal) {
            case ModalType::ManageCover:
                RenderManageCoverModal();
                break;
            case ModalType::ImportChooser:
                RenderImportChooserModal();
                break;
            case ModalType::SteamGridDBSearch:
                RenderSteamGridDBSearchModal();
                break;
            case ModalType::SteamGridDBGrids:
                RenderSteamGridDBGridsModal();
                break;
            case ModalType::ApiKeyInput:
                RenderApiKeyModal();
                break;
            case ModalType::MessagePopup:
                RenderMessageModal();
                break;
            default:
                break;
        }

        ImGui::Render();
        SDL_SetRenderDrawColor(m_renderer, 20, 22, 26, 255);
        SDL_RenderClear(m_renderer);
        ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), m_renderer);
        SDL_RenderPresent(m_renderer);
    }
}

void Gui::RenderHeader() {
    ImDrawList* draw = ImGui::GetWindowDrawList();

    // Top logo bar
    draw->AddText(ImGui::GetFont(), ImGui::GetFontSize() * 1.3f, ImVec2(32, 20), IM_COL32(0, 200, 255, 255), "boxart-nx");
    draw->AddText(ImGui::GetFont(), ImGui::GetFontSize() * 0.95f, ImVec2(145, 24), IM_COL32(140, 150, 165, 255), "| Home Menu Icon Manager");

    auto& games = m_inventory.GetGames();
    if (!games.empty()) {
        char count_buf[64];
        std::snprintf(count_buf, sizeof(count_buf), "%d / %zu", m_selected_game_index + 1, games.size());
        ImVec2 sz = ImGui::CalcTextSize(count_buf);
        draw->AddText(ImGui::GetFont(), ImGui::GetFontSize() * 1.1f, ImVec2(1248 - sz.x, 22), IM_COL32(230, 235, 245, 255), count_buf);
    }

    // SteamGridDB indicator
    float pill_x = 940.0f;
    float pill_y = 20.0f;
    if (m_steam_client.HasApiKey()) {
        draw->AddRectFilled(ImVec2(pill_x, pill_y), ImVec2(pill_x + 160, pill_y + 24), IM_COL32(30, 70, 45, 200), 12.0f);
        draw->AddText(ImVec2(pill_x + 14, pill_y + 4), IM_COL32(80, 230, 120, 255), "SteamGridDB: Ready");
    } else {
        draw->AddRectFilled(ImVec2(pill_x - 30, pill_y), ImVec2(pill_x + 160, pill_y + 24), IM_COL32(80, 50, 20, 200), 12.0f);
        draw->AddText(ImVec2(pill_x - 16, pill_y + 4), IM_COL32(255, 175, 60, 255), "[-] Set SteamGridDB Key");
    }

    // Horizontal divider
    draw->AddLine(ImVec2(30, 56), ImVec2(1250, 56), IM_COL32(40, 45, 55, 255), 1.0f);
}

void Gui::RenderCarousel() {
    auto& games = m_inventory.GetGames();
    if (games.empty()) {
        ImGui::SetCursorPos(ImVec2(400, 220));
        ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.2f, 1.0f), "No installed games detected on SD / eMMC storage.");
        return;
    }

    ImDrawList* draw = ImGui::GetWindowDrawList();

    // Carousel center coordinates
    const float center_x = 640.0f;
    const float center_y = 220.0f;

    // Card dimensions by offset from center k in {-3, -2, -1, 0, 1, 2, 3}
    struct CardSlot {
        float width;
        float height;
        float offset_x;
        uint8_t alpha;
    };

    const CardSlot slots[7] = {
        { 90.0f,  90.0f, -500.0f,  50 }, // k = -3
        { 120.0f, 120.0f, -360.0f, 100 }, // k = -2
        { 160.0f, 160.0f, -190.0f, 175 }, // k = -1
        { 220.0f, 220.0f,    0.0f, 255 }, // k =  0 (Selected)
        { 160.0f, 160.0f,  190.0f, 175 }, // k = +1
        { 120.0f, 120.0f,  360.0f, 100 }, // k = +2
        { 90.0f,  90.0f,  500.0f,  50 }  // k = +3
    };

    for (int step = 0; step < 7; ++step) {
        int k = step - 3;
        int game_idx = m_selected_game_index + k;

        if (game_idx < 0 || game_idx >= (int)games.size()) {
            continue;
        }

        const auto& slot = slots[step];
        auto& game = games[game_idx];

        // Lazy load texture for visible game cards
        m_inventory.EnsureIconLoaded(m_renderer, game);

        float cx = center_x + slot.offset_x;
        float cy = center_y;
        ImVec2 p_min(cx - slot.width * 0.5f, cy - slot.height * 0.5f);
        ImVec2 p_max(cx + slot.width * 0.5f, cy + slot.height * 0.5f);

        // Dark rounded placeholder background
        draw->AddRectFilled(p_min, p_max, IM_COL32(28, 32, 40, slot.alpha), 10.0f);

        // Draw icon texture
        if (game.current_icon_texture) {
            ImU32 tint = IM_COL32(255, 255, 255, slot.alpha);
            draw->AddImageRounded(
                (ImTextureID)(intptr_t)game.current_icon_texture,
                p_min, p_max,
                ImVec2(0, 0), ImVec2(1, 1),
                tint,
                10.0f
            );
        }

        // Draw card borders
        if (k == 0) {
            // Selected active card has glowing Horizon Cyan border
            draw->AddRect(
                ImVec2(p_min.x - 4, p_min.y - 4),
                ImVec2(p_max.x + 4, p_max.y + 4),
                IM_COL32(0, 200, 255, 255),
                12.0f,
                0,
                4.0f
            );

            // Custom cover indicator badge in top-right corner of card
            if (game.has_custom_cover) {
                float badge_w = 64.0f;
                float badge_h = 22.0f;
                ImVec2 b_min(p_max.x - badge_w - 6, p_min.y + 6);
                ImVec2 b_max(p_max.x - 6, p_min.y + 6 + badge_h);
                draw->AddRectFilled(b_min, b_max, IM_COL32(0, 160, 230, 240), 6.0f);
                draw->AddText(ImVec2(b_min.x + 8, b_min.y + 3), IM_COL32(255, 255, 255, 255), "CUSTOM");
            }
        } else {
            draw->AddRect(p_min, p_max, IM_COL32(50, 58, 68, slot.alpha), 10.0f, 0, 1.5f);
        }

        // Interactive touch/click button on card
        ImGui::SetCursorScreenPos(p_min);
        char btn_id[32];
        std::snprintf(btn_id, sizeof(btn_id), "##Card_%d", game_idx);
        if (ImGui::InvisibleButton(btn_id, ImVec2(slot.width, slot.height))) {
            if (k == 0) {
                m_active_modal = ModalType::ManageCover;
            } else {
                m_selected_game_index = game_idx;
            }
        }
    }
}

void Gui::RenderGameInfo(const GameItem& game) {
    ImDrawList* draw = ImGui::GetWindowDrawList();

    // Centered Title Text
    ImVec2 title_sz = ImGui::CalcTextSize(game.name.c_str());
    float text_scale = 1.35f;
    float scaled_w = title_sz.x * text_scale;
    float text_x = 640.0f - scaled_w * 0.5f;
    if (text_x < 40.0f) text_x = 40.0f; // Prevent clipping on long names

    draw->AddText(ImGui::GetFont(), ImGui::GetFontSize() * text_scale, ImVec2(text_x, 360), IM_COL32(255, 255, 255, 255), game.name.c_str());

    // Publisher, Version & TitleID
    char sub_buf[256];
    std::snprintf(sub_buf, sizeof(sub_buf), "%s   •   v%s   •   0x%s", game.author.c_str(), game.version.c_str(), game.title_id_hex.c_str());
    ImVec2 sub_sz = ImGui::CalcTextSize(sub_buf);
    draw->AddText(ImGui::GetFont(), ImGui::GetFontSize() * 1.0f, ImVec2(640.0f - sub_sz.x * 0.5f, 400), IM_COL32(165, 175, 190, 255), sub_buf);

    // Status Pill Badge
    float pill_y = 435.0f;
    if (game.has_custom_cover) {
        float pill_w = 460.0f;
        float pill_x = 640.0f - pill_w * 0.5f;
        draw->AddRectFilled(ImVec2(pill_x, pill_y), ImVec2(pill_x + pill_w, pill_y + 46), IM_COL32(0, 150, 220, 35), 8.0f);
        draw->AddRect(ImVec2(pill_x, pill_y), ImVec2(pill_x + pill_w, pill_y + 46), IM_COL32(0, 180, 240, 160), 8.0f, 0, 1.5f);

        const char* status_t = "CUSTOM COVER ACTIVE (MitM Overriding Home Menu)";
        ImVec2 st_sz = ImGui::CalcTextSize(status_t);
        draw->AddText(ImVec2(640.0f - st_sz.x * 0.5f, pill_y + 6), IM_COL32(0, 220, 255, 255), status_t);

        char path_buf[128];
        std::snprintf(path_buf, sizeof(path_buf), "sdmc:/config/boxart-nx/covers/%s.jpg", game.title_id_hex.c_str());
        ImVec2 pt_sz = ImGui::CalcTextSize(path_buf);
        draw->AddText(ImVec2(640.0f - pt_sz.x * 0.5f, pill_y + 24), IM_COL32(140, 160, 180, 255), path_buf);
    } else {
        float pill_w = 420.0f;
        float pill_x = 640.0f - pill_w * 0.5f;
        draw->AddRectFilled(ImVec2(pill_x, pill_y), ImVec2(pill_x + pill_w, pill_y + 46), IM_COL32(40, 45, 55, 70), 8.0f);
        draw->AddRect(ImVec2(pill_x, pill_y), ImVec2(pill_x + pill_w, pill_y + 46), IM_COL32(70, 78, 90, 140), 8.0f, 0, 1.5f);

        const char* status_t = "OFFICIAL ICON (Untouched NAND System Save)";
        ImVec2 st_sz = ImGui::CalcTextSize(status_t);
        draw->AddText(ImVec2(640.0f - st_sz.x * 0.5f, pill_y + 6), IM_COL32(200, 205, 215, 255), status_t);

        const char* sub_t = "Original 256x256 icon from official NACP control data";
        ImVec2 sbt_sz = ImGui::CalcTextSize(sub_t);
        draw->AddText(ImVec2(640.0f - sbt_sz.x * 0.5f, pill_y + 24), IM_COL32(130, 140, 150, 255), sub_t);
    }

    // Matching local file notification if found in /import/
    if (!game.matched_import_path.empty()) {
        std::string imp_fname = game.matched_import_path.substr(game.matched_import_path.find_last_of('/') + 1);
        std::string notice = "Matching local import available: " + imp_fname + "  [Press A to apply]";
        ImVec2 n_sz = ImGui::CalcTextSize(notice.c_str());
        draw->AddText(ImVec2(640.0f - n_sz.x * 0.5f, 500), IM_COL32(80, 230, 130, 255), notice.c_str());
    }
}

void Gui::RenderBottomBar() {
    ImDrawList* draw = ImGui::GetWindowDrawList();

    // Notification banner if recently triggered
    if (!m_status_banner.empty() && (SDL_GetTicks() - m_status_banner_time < 4500)) {
        ImVec2 b_sz = ImGui::CalcTextSize(m_status_banner.c_str());
        float pw = b_sz.x + 40.0f;
        float px = 640.0f - pw * 0.5f;
        draw->AddRectFilled(ImVec2(px, 595), ImVec2(px + pw, 627), IM_COL32(20, 80, 45, 230), 16.0f);
        draw->AddRect(ImVec2(px, 595), ImVec2(px + pw, 627), IM_COL32(60, 200, 100, 255), 16.0f, 0, 1.5f);
        draw->AddText(ImVec2(px + 20, 603), IM_COL32(240, 255, 240, 255), m_status_banner.c_str());
    }

    // Bottom bar background pill
    draw->AddRectFilled(ImVec2(30, 645), ImVec2(1250, 700), IM_COL32(14, 16, 20, 240), 12.0f);
    draw->AddRect(ImVec2(30, 645), ImVec2(1250, 700), IM_COL32(35, 40, 50, 180), 12.0f, 0, 1.0f);

    // Controller shortcuts helper
    auto draw_badge = [&](float x, const char* btn, ImU32 col, const char* label) -> float {
        draw->AddCircleFilled(ImVec2(x + 12, 672), 11.0f, col);
        ImVec2 btn_sz = ImGui::CalcTextSize(btn);
        draw->AddText(ImVec2(x + 12 - btn_sz.x * 0.5f, 664), IM_COL32(255, 255, 255, 255), btn);
        draw->AddText(ImVec2(x + 28, 664), IM_COL32(220, 225, 235, 255), label);
        ImVec2 lbl_sz = ImGui::CalcTextSize(label);
        return x + 28 + lbl_sz.x + 28.0f;
    };

    float cur_x = 55.0f;
    cur_x = draw_badge(cur_x, "A", IM_COL32(0, 160, 230, 255), "Manage Cover");
    cur_x = draw_badge(cur_x, "X", IM_COL32(60, 120, 230, 255), "SteamGridDB Search");
    cur_x = draw_badge(cur_x, "Y", IM_COL32(220, 150, 30, 255), "Restore Official");
    cur_x = draw_badge(cur_x, "L/R", IM_COL32(80, 90, 105, 255), "Jump 5");
    cur_x = draw_badge(cur_x, "-", IM_COL32(70, 75, 85, 255), "Settings");
    draw_badge(cur_x, "+", IM_COL32(180, 50, 50, 255), "Exit");
}

void Gui::RenderMainUI() {
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImVec2(1280, 720));
    ImGui::Begin("##MainWindow", nullptr,
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoBringToFrontOnFocus
    );

    RenderHeader();
    RenderCarousel();

    auto& games = m_inventory.GetGames();
    if (m_selected_game_index >= 0 && m_selected_game_index < (int)games.size()) {
        RenderGameInfo(games[m_selected_game_index]);
    }

    RenderBottomBar();

    ImGui::End();
}

void Gui::RenderManageCoverModal() {
    auto& games = m_inventory.GetGames();
    if (m_selected_game_index < 0 || m_selected_game_index >= (int)games.size()) {
        m_active_modal = ModalType::None;
        return;
    }
    auto& game = games[m_selected_game_index];

    ImGui::OpenPopup("Manage Game Cover");
    ImGui::SetNextWindowSize(ImVec2(680, 430), ImGuiCond_Always);
    ImGui::SetNextWindowPos(ImVec2(300, 145), ImGuiCond_Always);

    if (ImGui::BeginPopupModal("Manage Game Cover", nullptr, ImGuiWindowFlags_NoResize)) {
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "%s", game.name.c_str());
        ImGui::TextDisabled("Title ID: 0x%s  |  Version: v%s", game.title_id_hex.c_str(), game.version.c_str());
        ImGui::Separator();
        ImGui::Spacing();

        // Option 1: Quick apply matching import
        if (!game.matched_import_path.empty()) {
            std::string fname = game.matched_import_path.substr(game.matched_import_path.find_last_of('/') + 1);
            std::string label = "Apply Matched Import (" + fname + ")";
            if (ImGui::Button(label.c_str(), ImVec2(648, 42))) {
                ApplyCoverFromFile(game, game.matched_import_path);
                m_active_modal = ModalType::None;
            }
            ImGui::Spacing();
        }

        // Option 2: Browse Import directory
        if (ImGui::Button("Browse Import Folder (sdmc:/config/boxart-nx/import/)", ImVec2(648, 42))) {
            m_inventory.ScanImportDirectory(m_renderer);
            m_active_modal = ModalType::ImportChooser;
        }
        ImGui::Spacing();

        // Option 3: SteamGridDB Search
        if (ImGui::Button("Search SteamGridDB Online...", ImVec2(648, 42))) {
            OpenSteamGridDBSearch();
        }
        ImGui::Spacing();

        // Option 4: Restore Original
        if (game.has_custom_cover) {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.70f, 0.22f, 0.22f, 1.00f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.85f, 0.28f, 0.28f, 1.00f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.95f, 0.35f, 0.35f, 1.00f));
            if (ImGui::Button("Restore Original Official NAND Icon", ImVec2(648, 42))) {
                RestoreOriginalCover(game);
                m_active_modal = ModalType::None;
            }
            ImGui::PopStyleColor(3);
            ImGui::Spacing();
        }

        ImGui::Separator();
        ImGui::Spacing();
        if (ImGui::Button("Close [B]", ImVec2(140, 36))) {
            m_active_modal = ModalType::None;
        }

        ImGui::EndPopup();
    }
}

void Gui::RenderImportChooserModal() {
    ImGui::OpenPopup("Import Directory Browser");
    ImGui::SetNextWindowSize(ImVec2(800, 560), ImGuiCond_Always);
    ImGui::SetNextWindowPos(ImVec2(240, 80), ImGuiCond_Always);

    if (ImGui::BeginPopupModal("Import Directory Browser", nullptr, ImGuiWindowFlags_NoResize)) {
        ImGui::Text("Images found in sdmc:/config/boxart-nx/import/");
        ImGui::TextDisabled("Select an image to normalize (256x256 Baseline JPEG) and apply to current game.");
        ImGui::Separator();

        auto& import_files = m_inventory.GetImportFiles();
        if (import_files.empty()) {
            ImGui::Spacing();
            ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.2f, 1.0f), "No .png, .jpg, or .webp images found in import directory.");
            ImGui::TextDisabled("Place images in 'sdmc:/config/boxart-nx/import/' on your SD card.");
        } else {
            ImGui::BeginChild("ImportScroll", ImVec2(0, 420), true);

            float panel_width = ImGui::GetContentRegionAvail().x;
            int columns = (int)(panel_width / 140.0f);
            if (columns < 1) columns = 1;

            if (ImGui::BeginTable("ImportGridTable", columns)) {
                for (size_t i = 0; i < import_files.size(); ++i) {
                    ImGui::TableNextColumn();
                    auto& item = import_files[i];

                    // Lazy load thumbnail when modal is open
                    if (!item.thumbnail_texture) {
                        item.thumbnail_texture = CreateTextureFromFile(m_renderer, item.full_path);
                    }

                    ImGui::PushID((int)i);
                    if (item.thumbnail_texture) {
                        if (ImGui::ImageButton("##ImpThumb", (ImTextureID)(intptr_t)item.thumbnail_texture, ImVec2(100, 100))) {
                            auto& games = m_inventory.GetGames();
                            if (m_selected_game_index >= 0 && m_selected_game_index < (int)games.size()) {
                                ApplyCoverFromFile(games[m_selected_game_index], item.full_path);
                                m_active_modal = ModalType::None;
                            }
                        }
                    } else {
                        if (ImGui::Button(item.filename.c_str(), ImVec2(100, 100))) {
                            auto& games = m_inventory.GetGames();
                            if (m_selected_game_index >= 0 && m_selected_game_index < (int)games.size()) {
                                ApplyCoverFromFile(games[m_selected_game_index], item.full_path);
                                m_active_modal = ModalType::None;
                            }
                        }
                    }

                    std::string short_name = item.filename;
                    if (short_name.length() > 14) short_name = short_name.substr(0, 12) + "...";
                    ImGui::TextWrapped("%s", short_name.c_str());

                    ImGui::PopID();
                }
                ImGui::EndTable();
            }

            ImGui::EndChild();
        }

        ImGui::Separator();
        if (ImGui::Button("Close [B]", ImVec2(140, 32))) {
            m_active_modal = ModalType::None;
        }

        ImGui::EndPopup();
    }
}

void Gui::RenderSteamGridDBSearchModal() {
    ImGui::OpenPopup("SteamGridDB Search");
    ImGui::SetNextWindowSize(ImVec2(800, 560), ImGuiCond_Always);
    ImGui::SetNextWindowPos(ImVec2(240, 80), ImGuiCond_Always);

    if (ImGui::BeginPopupModal("SteamGridDB Search", nullptr, ImGuiWindowFlags_NoResize)) {
        if (!m_steam_client.HasApiKey()) {
            ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "SteamGridDB API key required!");
            ImGui::Text("Please configure your API key in sdmc:/config/boxart-nx/config.ini");
            ImGui::Spacing();
            if (ImGui::Button("Enter API Key Now", ImVec2(180, 36))) {
                m_active_modal = ModalType::ApiKeyInput;
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel [B]", ImVec2(120, 36))) {
                m_active_modal = ModalType::None;
            }
            ImGui::EndPopup();
            return;
        }

        ImGui::Text("Search game on SteamGridDB:");
        ImGui::InputText("##SteamQuery", m_steam_search_query, sizeof(m_steam_search_query));
        ImGui::SameLine();

        if (ImGui::Button("Search", ImVec2(100, 0))) {
            m_steam_loading = true;
            std::string err;
            if (!m_steam_client.SearchGames(m_steam_search_query, m_steam_matches, err)) {
                m_steam_status = "Error: " + err;
            } else {
                m_steam_status = "Found " + std::to_string(m_steam_matches.size()) + " matches.";
            }
            m_steam_loading = false;
        }

        if (!m_steam_status.empty()) {
            ImGui::TextDisabled("%s", m_steam_status.c_str());
        }

        ImGui::Separator();

        ImGui::BeginChild("MatchesScroll", ImVec2(0, 380), true);
        for (size_t i = 0; i < m_steam_matches.size(); ++i) {
            auto& match = m_steam_matches[i];
            ImGui::PushID((int)i);

            std::string label = match.name + " (ID: " + std::to_string(match.id) + ")";
            if (ImGui::Selectable(label.c_str(), m_selected_steam_match == (int)i, 0, ImVec2(0, 32))) {
                m_selected_steam_match = (int)i;

                // Fetch grids for this game
                m_steam_loading = true;
                std::string err;
                for (auto& g : m_steam_grids) g.FreeTexture();
                m_steam_grids.clear();

                if (m_steam_client.FetchGridsForGame(match.id, m_steam_grids, err)) {
                    for (size_t j = 0; j < m_steam_grids.size() && j < 12; ++j) {
                        const std::string& t_url = !m_steam_grids[j].thumb_url.empty() ? m_steam_grids[j].thumb_url : m_steam_grids[j].url;
                        m_steam_grids[j].thumb_texture = m_steam_client.LoadPreviewTexture(m_renderer, t_url);
                    }
                    m_active_modal = ModalType::SteamGridDBGrids;
                } else {
                    m_steam_status = "Grids error: " + err;
                }
                m_steam_loading = false;
            }

            ImGui::PopID();
        }
        ImGui::EndChild();

        ImGui::Separator();
        if (ImGui::Button("Cancel [B]", ImVec2(140, 32))) {
            m_active_modal = ModalType::None;
        }

        ImGui::EndPopup();
    }
}

void Gui::RenderSteamGridDBGridsModal() {
    ImGui::OpenPopup("Select SteamGridDB Cover");
    ImGui::SetNextWindowSize(ImVec2(880, 600), ImGuiCond_Always);
    ImGui::SetNextWindowPos(ImVec2(200, 60), ImGuiCond_Always);

    if (ImGui::BeginPopupModal("Select SteamGridDB Cover", nullptr, ImGuiWindowFlags_NoResize)) {
        ImGui::Text("Square Covers (600x600, 1024x1024) - Found %zu covers", m_steam_grids.size());
        ImGui::TextDisabled("Select any cover to download, normalize (256x256 Baseline JPEG), and apply directly.");
        ImGui::Separator();

        ImGui::BeginChild("GridsScroll", ImVec2(0, 460), true);

        if (m_steam_grids.empty()) {
            ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.2f, 1.0f), "No square 600x600 or 1024x1024 covers found for this game.");
        } else {
            float panel_width = ImGui::GetContentRegionAvail().x;
            int columns = (int)(panel_width / 160.0f);
            if (columns < 1) columns = 1;

            if (ImGui::BeginTable("GridsTable", columns)) {
                for (size_t i = 0; i < m_steam_grids.size(); ++i) {
                    ImGui::TableNextColumn();
                    auto& grid = m_steam_grids[i];

                    ImGui::PushID((int)i);

                    // Lazy load thumbnail
                    if (!grid.thumb_texture) {
                        const std::string& t_url = !grid.thumb_url.empty() ? grid.thumb_url : grid.url;
                        grid.thumb_texture = m_steam_client.LoadPreviewTexture(m_renderer, t_url);
                    }

                    if (grid.thumb_texture) {
                        if (ImGui::ImageButton("##GridThumb", (ImTextureID)(intptr_t)grid.thumb_texture, ImVec2(130, 130))) {
                            std::vector<uint8_t> img_data;
                            std::string err;
                            if (m_steam_client.DownloadImage(grid.url, img_data, err)) {
                                auto& games = m_inventory.GetGames();
                                if (m_selected_game_index >= 0 && m_selected_game_index < (int)games.size()) {
                                    ApplyCoverFromMemory(games[m_selected_game_index], img_data.data(), img_data.size());
                                    m_active_modal = ModalType::None;
                                }
                            } else {
                                ShowMessage("Download Error", err);
                            }
                        }
                    } else {
                        if (ImGui::Button("Apply Cover", ImVec2(130, 130))) {
                            std::vector<uint8_t> img_data;
                            std::string err;
                            if (m_steam_client.DownloadImage(grid.url, img_data, err)) {
                                auto& games = m_inventory.GetGames();
                                if (m_selected_game_index >= 0 && m_selected_game_index < (int)games.size()) {
                                    ApplyCoverFromMemory(games[m_selected_game_index], img_data.data(), img_data.size());
                                    m_active_modal = ModalType::None;
                                }
                            } else {
                                ShowMessage("Download Error", err);
                            }
                        }
                    }

                    ImGui::TextDisabled("%dx%d | %s", grid.width, grid.height, grid.style.c_str());

                    ImGui::PopID();
                }
                ImGui::EndTable();
            }
        }

        ImGui::EndChild();

        ImGui::Separator();
        if (ImGui::Button("Back to Matches", ImVec2(160, 32))) {
            m_active_modal = ModalType::SteamGridDBSearch;
        }
        ImGui::SameLine();
        if (ImGui::Button("Close [B]", ImVec2(120, 32))) {
            m_active_modal = ModalType::None;
        }

        ImGui::EndPopup();
    }
}

void Gui::RenderApiKeyModal() {
    ImGui::OpenPopup("Configure SteamGridDB API Key");
    ImGui::SetNextWindowSize(ImVec2(600, 260), ImGuiCond_Always);
    ImGui::SetNextWindowPos(ImVec2(340, 230), ImGuiCond_Always);

    if (ImGui::BeginPopupModal("Configure SteamGridDB API Key", nullptr, ImGuiWindowFlags_NoResize)) {
        static char key_buf[128] = {0};
        if (key_buf[0] == '\0' && m_steam_client.HasApiKey()) {
            std::snprintf(key_buf, sizeof(key_buf), "%s", m_steam_client.GetApiKey().c_str());
        }

        ImGui::Text("Enter your SteamGridDB API Key:");
        ImGui::TextDisabled("Saved to sdmc:/config/boxart-nx/config.ini");
        ImGui::Spacing();

        ImGui::InputText("##ApiKeyInput", key_buf, sizeof(key_buf));

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        if (ImGui::Button("Save Key [A]", ImVec2(140, 36))) {
            if (key_buf[0] != '\0') {
                m_steam_client.SetApiKey(key_buf);
                m_status_banner = "SteamGridDB API key saved!";
                m_status_banner_time = SDL_GetTicks();
                m_active_modal = ModalType::None;
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel [B]", ImVec2(120, 36))) {
            m_active_modal = ModalType::None;
        }

        ImGui::EndPopup();
    }
}

void Gui::RenderMessageModal() {
    ImGui::OpenPopup(m_modal_title.c_str());
    ImGui::SetNextWindowSize(ImVec2(500, 220), ImGuiCond_Always);
    ImGui::SetNextWindowPos(ImVec2(390, 250), ImGuiCond_Always);

    if (ImGui::BeginPopupModal(m_modal_title.c_str(), nullptr, ImGuiWindowFlags_NoResize)) {
        ImGui::Spacing();
        ImGui::TextWrapped("%s", m_modal_message.c_str());
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        if (ImGui::Button("OK [A]", ImVec2(120, 32))) {
            m_active_modal = ModalType::None;
        }

        ImGui::EndPopup();
    }
}

} // namespace BoxArt
