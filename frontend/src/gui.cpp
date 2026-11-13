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
#include <cstdio>
#include <cstring>

namespace BoxArt {

static void SetupImGuiStyle() {
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 8.0f;
    style.FrameRounding = 6.0f;
    style.PopupRounding = 8.0f;
    style.ScrollbarRounding = 6.0f;
    style.GrabRounding = 6.0f;
    style.WindowBorderSize = 1.0f;
    style.FrameBorderSize = 0.0f;
    style.ItemSpacing = ImVec2(10.0f, 8.0f);
    style.ItemInnerSpacing = ImVec2(8.0f, 6.0f);
    style.WindowPadding = ImVec2(12.0f, 12.0f);

    ImVec4* colors = style.Colors;
    colors[ImGuiCol_WindowBg]             = ImVec4(0.10f, 0.11f, 0.13f, 1.00f);
    colors[ImGuiCol_ChildBg]              = ImVec4(0.13f, 0.14f, 0.17f, 1.00f);
    colors[ImGuiCol_PopupBg]              = ImVec4(0.12f, 0.13f, 0.16f, 0.98f);
    colors[ImGuiCol_Border]               = ImVec4(0.24f, 0.27f, 0.32f, 0.60f);
    colors[ImGuiCol_FrameBg]              = ImVec4(0.18f, 0.20f, 0.24f, 1.00f);
    colors[ImGuiCol_FrameBgHovered]       = ImVec4(0.25f, 0.28f, 0.34f, 1.00f);
    colors[ImGuiCol_FrameBgActive]        = ImVec4(0.28f, 0.32f, 0.38f, 1.00f);
    colors[ImGuiCol_TitleBg]              = ImVec4(0.08f, 0.09f, 0.11f, 1.00f);
    colors[ImGuiCol_TitleBgActive]        = ImVec4(0.12f, 0.28f, 0.42f, 1.00f);
    colors[ImGuiCol_Button]               = ImVec4(0.18f, 0.35f, 0.52f, 1.00f);
    colors[ImGuiCol_ButtonHovered]        = ImVec4(0.24f, 0.45f, 0.66f, 1.00f);
    colors[ImGuiCol_ButtonActive]         = ImVec4(0.28f, 0.52f, 0.76f, 1.00f);
    colors[ImGuiCol_Header]               = ImVec4(0.20f, 0.38f, 0.56f, 0.65f);
    colors[ImGuiCol_HeaderHovered]        = ImVec4(0.25f, 0.46f, 0.68f, 0.85f);
    colors[ImGuiCol_HeaderActive]         = ImVec4(0.29f, 0.54f, 0.79f, 1.00f);
    colors[ImGuiCol_ScrollbarBg]          = ImVec4(0.08f, 0.09f, 0.11f, 0.60f);
    colors[ImGuiCol_ScrollbarGrab]        = ImVec4(0.26f, 0.30f, 0.36f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.34f, 0.38f, 0.46f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive]  = ImVec4(0.42f, 0.48f, 0.56f, 1.00f);
    colors[ImGuiCol_Text]                 = ImVec4(0.92f, 0.94f, 0.96f, 1.00f);
    colors[ImGuiCol_TextDisabled]         = ImVec4(0.50f, 0.55f, 0.60f, 1.00f);
    colors[ImGuiCol_NavHighlight]         = ImVec4(0.28f, 0.62f, 0.92f, 1.00f);
}

Gui::Gui() {
}

Gui::~Gui() {
    Shutdown();
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
    io.IniFilename = nullptr; // Disable imgui.ini writing on SD

    SetupImGuiStyle();

    // Setup Platform/Renderer backends
    ImGui_ImplSDL2_InitForSDLRenderer(m_window, m_renderer);
    ImGui_ImplSDLRenderer2_Init(m_renderer);

    // Initial inventory load
    std::string err;
    m_inventory.ReloadGames(m_renderer, err);

    return true;
}

void Gui::Shutdown() {
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

void Gui::ShowMessage(const std::string& title, const std::string& message) {
    m_modal_title = title;
    m_modal_message = message;
    m_active_modal = ModalType::MessagePopup;
}

void Gui::ApplyCoverFromFile(GameItem& game, const std::string& path) {
    std::string err;
    if (JpegPipeline::ProcessAndSaveFromFile(game.title_id, path, err)) {
        m_inventory.RefreshGameTextures(m_renderer, game);
        m_status_banner = "Cover applied successfully from: " + path;
        m_status_banner_time = SDL_GetTicks();
    } else {
        ShowMessage("Error applying cover", err);
    }
}

void Gui::ApplyCoverFromMemory(GameItem& game, const uint8_t* data, size_t size) {
    std::string err;
    if (JpegPipeline::ProcessAndSaveFromMemory(game.title_id, data, size, err)) {
        m_inventory.RefreshGameTextures(m_renderer, game);
        m_status_banner = "Cover applied successfully from SteamGridDB!";
        m_status_banner_time = SDL_GetTicks();
    } else {
        ShowMessage("Error applying cover", err);
    }
}

void Gui::RestoreOriginalCover(GameItem& game) {
    std::string err;
    if (JpegPipeline::RevertCustomCover(game.title_id, err)) {
        m_inventory.RefreshGameTextures(m_renderer, game);
        m_status_banner = "Original icon restored for: " + game.name;
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
                    // Plus button exits
                    running = false;
                } else if (btn == SDL_CONTROLLER_BUTTON_B) {
                    // B button closes modal if active
                    if (m_active_modal != ModalType::None) {
                        m_active_modal = ModalType::None;
                    }
                } else if (btn == SDL_CONTROLLER_BUTTON_X && m_active_modal == ModalType::None) {
                    // X button triggers SteamGridDB search for selected game
                    auto& games = m_inventory.GetGames();
                    if (m_selected_game_index >= 0 && m_selected_game_index < (int)games.size()) {
                        std::snprintf(m_steam_search_query, sizeof(m_steam_search_query), "%s", games[m_selected_game_index].name.c_str());
                        m_steam_matches.clear();
                        for (auto& g : m_steam_grids) g.FreeTexture();
                        m_steam_grids.clear();
                        m_selected_steam_match = -1;
                        m_selected_steam_grid = -1;
                        m_active_modal = ModalType::SteamGridDBSearch;
                    }
                } else if (btn == SDL_CONTROLLER_BUTTON_Y && m_active_modal == ModalType::None) {
                    // Y button restores original icon
                    auto& games = m_inventory.GetGames();
                    if (m_selected_game_index >= 0 && m_selected_game_index < (int)games.size()) {
                        RestoreOriginalCover(games[m_selected_game_index]);
                    }
                }
            }
        }

        // Start Dear ImGui frame
        ImGui_ImplSDLRenderer2_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        RenderMainUI();

        // Render any active modal
        switch (m_active_modal) {
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

        // ImGui Render
        ImGui::Render();
        SDL_SetRenderDrawColor(m_renderer, 18, 20, 24, 255);
        SDL_RenderClear(m_renderer);
        ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), m_renderer);
        SDL_RenderPresent(m_renderer);
    }
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

    // Header bar
    ImGui::BeginGroup();
    ImGui::TextColored(ImVec4(0.35f, 0.75f, 1.00f, 1.00f), "boxart-nx");
    ImGui::SameLine();
    ImGui::TextDisabled("| Non-Destructive Home Menu Icon Manager");

    ImGui::SameLine(1000);
    if (m_steam_client.HasApiKey()) {
        ImGui::TextColored(ImVec4(0.4f, 0.9f, 0.4f, 1.0f), "SteamGridDB: Ready");
    } else {
        if (ImGui::SmallButton("Set SteamGridDB API Key")) {
            m_active_modal = ModalType::ApiKeyInput;
        }
    }
    ImGui::EndGroup();

    ImGui::Separator();

    // Main Columns: Left (Games List) - Right (Selected Game Details)
    float content_height = 620.0f;
    RenderGameList(420.0f, content_height);
    ImGui::SameLine();
    RenderGameDetails(830.0f, content_height);

    // Footer bar
    ImGui::Separator();
    if (!m_status_banner.empty() && (SDL_GetTicks() - m_status_banner_time < 5000)) {
        ImGui::TextColored(ImVec4(0.4f, 0.95f, 0.4f, 1.0f), "%s", m_status_banner.c_str());
    } else {
        ImGui::TextDisabled("[D-Pad/Stick] Navigate | [A] Select | [X] SteamGridDB | [Y] Restore Original | [B] Back | [+] Exit");
    }

    ImGui::End();
}

void Gui::RenderGameList(float width, float height) {
    ImGui::BeginChild("GameListPane", ImVec2(width, height), true);

    auto& games = m_inventory.GetGames();

    ImGui::Text("Installed Games (%zu)", games.size());
    ImGui::InputTextWithHint("##Filter", "Filter games...", m_search_filter, sizeof(m_search_filter));
    ImGui::Separator();

    ImGui::BeginChild("GameScrollList", ImVec2(0, 0), false);

    std::string filter_lower = m_search_filter;
    std::transform(filter_lower.begin(), filter_lower.end(), filter_lower.begin(), [](unsigned char c) { return std::tolower(c); });

    for (size_t i = 0; i < games.size(); ++i) {
        auto& game = games[i];

        if (!filter_lower.empty()) {
            std::string name_lower = game.name;
            std::transform(name_lower.begin(), name_lower.end(), name_lower.begin(), [](unsigned char c) { return std::tolower(c); });
            if (name_lower.find(filter_lower) == std::string::npos && game.title_id_hex.find(filter_lower) == std::string::npos) {
                continue;
            }
        }

        bool is_selected = (m_selected_game_index == (int)i);

        ImGui::PushID((int)i);

        // Thumbnail icon (36x36)
        if (game.current_icon_texture) {
            ImGui::Image((ImTextureID)(intptr_t)game.current_icon_texture, ImVec2(36, 36));
            ImGui::SameLine();
        } else {
            ImGui::Dummy(ImVec2(36, 36));
            ImGui::SameLine();
        }

        ImGui::BeginGroup();
        if (ImGui::Selectable(game.name.c_str(), is_selected, 0, ImVec2(0, 36))) {
            m_selected_game_index = (int)i;
        }

        // Badge indicator
        ImGui::SameLine(width - 110);
        if (game.has_custom_cover) {
            ImGui::TextColored(ImVec4(0.2f, 0.85f, 0.95f, 1.0f), "[CUSTOM]");
        } else {
            ImGui::TextDisabled("[ORIG]");
        }
        ImGui::EndGroup();

        ImGui::PopID();
    }

    ImGui::EndChild();
    ImGui::EndChild();
}

void Gui::RenderGameDetails(float width, float height) {
    ImGui::BeginChild("GameDetailsPane", ImVec2(width, height), true);

    auto& games = m_inventory.GetGames();
    if (m_selected_game_index < 0 || m_selected_game_index >= (int)games.size()) {
        ImGui::TextDisabled("Select a game from the list to manage its icon.");
        ImGui::EndChild();
        return;
    }

    auto& game = games[m_selected_game_index];

    // Top Section: Icon Preview + Metadata
    ImGui::BeginGroup();
    if (game.current_icon_texture) {
        ImGui::Image((ImTextureID)(intptr_t)game.current_icon_texture, ImVec2(256, 256));
    } else {
        ImGui::Dummy(ImVec2(256, 256));
    }
    ImGui::EndGroup();

    ImGui::SameLine();

    ImGui::BeginGroup();
    ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "%s", game.name.c_str());
    ImGui::Spacing();

    ImGui::TextDisabled("Title ID:");
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "0x%s", game.title_id_hex.c_str());

    ImGui::TextDisabled("Publisher:");
    ImGui::SameLine();
    ImGui::Text("%s", game.author.c_str());

    ImGui::TextDisabled("Version:");
    ImGui::SameLine();
    ImGui::Text("v%s", game.version.c_str());

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::TextDisabled("Active Icon State:");
    if (game.has_custom_cover) {
        ImGui::TextColored(ImVec4(0.2f, 0.9f, 1.0f, 1.0f), "CUSTOM OVERRIDE ACTIVE");
        ImGui::TextDisabled("(Loaded by MitM from sdmc:/config/boxart-nx/covers/%s.jpg)", game.title_id_hex.c_str());
    } else {
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "OFFICIAL NAND / NACP ICON");
        ImGui::TextDisabled("(Untouched official application control data)");
    }

    ImGui::EndGroup();

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // Action Buttons
    ImGui::Text("Icon Actions:");
    ImGui::Spacing();

    // Action 1: Matched Import File (Fast apply)
    if (!game.matched_import_path.empty()) {
        std::string btn_label = "Apply Matched File (" + game.matched_import_path.substr(game.matched_import_path.find_last_of('/') + 1) + ")";
        if (ImGui::Button(btn_label.c_str(), ImVec2(400, 36))) {
            ApplyCoverFromFile(game, game.matched_import_path);
        }
    }

    // Action 2: Choose from Import Directory
    if (ImGui::Button("Browse Import Folder (sdmc:/config/boxart-nx/import/)", ImVec2(400, 36))) {
        m_inventory.ScanImportDirectory(m_renderer);
        m_active_modal = ModalType::ImportChooser;
    }

    // Action 3: SteamGridDB Search
    if (ImGui::Button("Search SteamGridDB Online...", ImVec2(400, 36))) {
        std::snprintf(m_steam_search_query, sizeof(m_steam_search_query), "%s", game.name.c_str());
        m_steam_matches.clear();
        for (auto& g : m_steam_grids) g.FreeTexture();
        m_steam_grids.clear();
        m_selected_steam_match = -1;
        m_selected_steam_grid = -1;
        m_active_modal = ModalType::SteamGridDBSearch;
    }

    // Action 4: Restore Original
    if (game.has_custom_cover) {
        ImGui::Spacing();
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.70f, 0.22f, 0.22f, 1.00f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.85f, 0.28f, 0.28f, 1.00f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.95f, 0.35f, 0.35f, 1.00f));

        if (ImGui::Button("Restore Original Official Icon", ImVec2(400, 36))) {
            RestoreOriginalCover(game);
        }

        ImGui::PopStyleColor(3);
    }

    ImGui::EndChild();
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
            ImGui::TextDisabled("Drop images into 'sdmc:/config/boxart-nx/import/' on your SD card.");
        } else {
            ImGui::BeginChild("ImportScroll", ImVec2(0, 420), true);

            float panel_width = ImGui::GetContentRegionAvail().x;
            int columns = (int)(panel_width / 140.0f);
            if (columns < 1) columns = 1;

            if (ImGui::BeginTable("ImportGridTable", columns)) {
                for (size_t i = 0; i < import_files.size(); ++i) {
                    ImGui::TableNextColumn();
                    auto& item = import_files[i];

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

                    // Trim filename for display
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
        if (ImGui::Button("Close", ImVec2(120, 32))) {
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
            if (ImGui::Button("Cancel", ImVec2(120, 36))) {
                m_active_modal = ModalType::None;
            }
            ImGui::EndPopup();
            return;
        }

        ImGui::Text("Search for game on SteamGridDB:");
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
                    // Pre-load thumbnail textures for the first several grids
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
        if (ImGui::Button("Cancel", ImVec2(120, 32))) {
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
        ImGui::TextDisabled("Select any cover to download, normalize (256x256 JPEG), and apply directly.");
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

                    // Lazy load thumbnail if not yet loaded
                    if (!grid.thumb_texture) {
                        const std::string& t_url = !grid.thumb_url.empty() ? grid.thumb_url : grid.url;
                        grid.thumb_texture = m_steam_client.LoadPreviewTexture(m_renderer, t_url);
                    }

                    if (grid.thumb_texture) {
                        if (ImGui::ImageButton("##GridThumb", (ImTextureID)(intptr_t)grid.thumb_texture, ImVec2(130, 130))) {
                            // Download full cover and apply
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
        if (ImGui::Button("Back to Search", ImVec2(140, 32))) {
            m_active_modal = ModalType::SteamGridDBSearch;
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120, 32))) {
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

        if (ImGui::Button("Save Key", ImVec2(120, 32))) {
            if (key_buf[0] != '\0') {
                m_steam_client.SetApiKey(key_buf);
                m_active_modal = ModalType::None;
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(100, 32))) {
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

        if (ImGui::Button("OK", ImVec2(120, 32))) {
            m_active_modal = ModalType::None;
        }

        ImGui::EndPopup();
    }
}

} // namespace BoxArt
