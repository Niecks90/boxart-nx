/*
 * Copyright (c) 2026 boxart-nx contributors
 * Distributed under the terms of the GNU General Public License v2
 */

#include "cover_select_activity.hpp"
#include "jpeg_pipeline.hpp"

#include <borealis/views/progress_spinner.hpp>
#include <borealis/views/dialog.hpp>
#include <borealis/views/button.hpp>
#include <borealis/views/scrolling_frame.hpp>
#include <borealis/views/tab_frame.hpp>

#include <dirent.h>
#include <sys/stat.h>
#include <algorithm>

namespace BoxArt {

static std::string ToLower(const std::string& str) {
    std::string lower = str;
    std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char c) { return std::tolower(c); });
    return lower;
}

CoverSelectActivity::CoverSelectActivity(const GameItem& game, std::function<void()> on_applied)
    : m_game(game), m_on_applied(on_applied), m_search_query(game.name) {

    auto* tabFrame = new brls::TabFrame();

    tabFrame->addTab("SteamGridDB", [this]() {
        return CreateSteamGridTab();
    });

    tabFrame->addTab("Carte SD (import/)", [this]() {
        return CreateSdImportTab();
    });

    auto* applet = new brls::AppletFrame(tabFrame);
    applet->setTitle(m_game.name);
    this->setContentView(applet);

    // Register B button to return
    this->registerAction("Retour", brls::BUTTON_B, [](brls::View* v) {
        brls::Application::popActivity();
        return true;
    });
}

brls::View* CoverSelectActivity::CreateSteamGridTab() {
    auto* tabRoot = new brls::Box(brls::Axis::COLUMN);
    tabRoot->setGrow(1.0f);
    tabRoot->setPadding(16.0f, 24.0f, 16.0f, 24.0f);

    // Search header bar
    auto* searchBar = new brls::Box(brls::Axis::ROW);
    searchBar->setFocusable(true);
    searchBar->setHeight(56.0f);
    searchBar->setPadding(8.0f, 16.0f, 8.0f, 16.0f);
    searchBar->setAlignItems(brls::AlignItems::CENTER);
    searchBar->setJustifyContent(brls::JustifyContent::SPACE_BETWEEN);
    searchBar->setCornerRadius(8.0f);
    searchBar->setLineBottom(1);
    searchBar->setLineColor(brls::Application::getTheme()["brls/sidebar/separator"]);

    m_search_label = new brls::Label();
    m_search_label->setFontSize(18.0f);
    m_search_label->setText("Recherche : " + m_search_query);
    searchBar->addView(m_search_label);

    auto* searchHint = new brls::Label();
    searchHint->setFontSize(14.0f);
    searchHint->setTextColor(brls::Application::getTheme()["brls/text_disabled"]);
    searchHint->setText("Ⓐ Modifier la recherche");
    searchBar->addView(searchHint);

    searchBar->registerClickAction([this](brls::View* v) {
        brls::Application::getPlatform()->getImeManager()->openForText([this](std::string text) {
            if (!text.empty()) {
                m_search_query = text;
                if (m_search_label) {
                    m_search_label->setText("Recherche : " + m_search_query);
                }
                SearchSteamGridDB();
            }
        }, "Rechercher sur SteamGridDB", "", 64, m_search_query);
        return true;
    });

    tabRoot->addView(searchBar);

    // Results container
    m_steam_results_container = new brls::Box(brls::Axis::COLUMN);
    m_steam_results_container->setGrow(1.0f);
    m_steam_results_container->setMarginTop(12.0f);
    tabRoot->addView(m_steam_results_container);

    // Check API key first
    if (!m_steam_client.HasApiKey()) {
        auto* keyBox = new brls::Box(brls::Axis::COLUMN);
        keyBox->setGrow(1.0f);
        keyBox->setAlignItems(brls::AlignItems::CENTER);
        keyBox->setJustifyContent(brls::JustifyContent::CENTER);

        auto* keyMsg = new brls::Label();
        keyMsg->setFontSize(18.0f);
        keyMsg->setText("Aucune clé API configurée dans sdmc:/config/boxart-nx/config.ini");
        keyBox->addView(keyMsg);

        auto* keyBtn = new brls::Button();
        keyBtn->setText("Entrer une clé API SteamGridDB");
        keyBtn->setStyle(&brls::BUTTONSTYLE_PRIMARY);
        keyBtn->setMarginTop(20.0f);
        keyBtn->registerClickAction([this](brls::View* v) {
            brls::Application::getPlatform()->getImeManager()->openForText([this](std::string key) {
                if (!key.empty()) {
                    m_steam_client.SetApiKey(key);
                    brls::Application::notify("Clé API enregistrée !");
                    SearchSteamGridDB();
                }
            }, "Clé API SteamGridDB", "Entrez votre clé API", 64, "");
            return true;
        });
        keyBox->addView(keyBtn);

        m_steam_results_container->addView(keyBox);
    } else {
        // Automatically start search for game name
        SearchSteamGridDB();
    }

    return tabRoot;
}

void CoverSelectActivity::SearchSteamGridDB() {
    if (!m_steam_results_container) return;

    m_steam_results_container->clearViews();

    auto* loadingBox = new brls::Box(brls::Axis::COLUMN);
    loadingBox->setGrow(1.0f);
    loadingBox->setAlignItems(brls::AlignItems::CENTER);
    loadingBox->setJustifyContent(brls::JustifyContent::CENTER);

    auto* spinner = new brls::ProgressSpinner(brls::ProgressSpinnerSize::NORMAL);
    spinner->animate(true);
    loadingBox->addView(spinner);

    auto* loadText = new brls::Label();
    loadText->setFontSize(16.0f);
    loadText->setMarginTop(16.0f);
    loadText->setText(fmt::format("Recherche de jaquettes pour \"{}\"...", m_search_query));
    loadingBox->addView(loadText);

    m_steam_results_container->addView(loadingBox);

    std::string query = m_search_query;
    brls::async([this, query]() {
        std::vector<SteamGameMatch> matches;
        std::string err;
        bool ok = m_steam_client.SearchGames(query, matches, err);

        std::vector<SteamGridResult> grids;
        if (ok && !matches.empty()) {
            m_steam_client.FetchGridsForGame(matches[0].id, grids, err);
        }

        brls::sync([this, ok, matches, grids, err]() {
            if (!m_steam_results_container) return;
            m_steam_results_container->clearViews();

            if (!ok || matches.empty() || grids.empty()) {
                auto* emptyBox = new brls::Box(brls::Axis::COLUMN);
                emptyBox->setGrow(1.0f);
                emptyBox->setAlignItems(brls::AlignItems::CENTER);
                emptyBox->setJustifyContent(brls::JustifyContent::CENTER);

                auto* label = new brls::Label();
                label->setFontSize(18.0f);
                label->setTextColor(brls::Application::getTheme()["brls/text_disabled"]);
                if (!ok) {
                    label->setText("Erreur SteamGridDB : " + err);
                } else if (matches.empty()) {
                    label->setText(fmt::format("Aucun jeu trouvé pour \"{}\"", m_search_query));
                } else {
                    label->setText(fmt::format("Aucune jaquette carrée disponible pour \"{}\"", matches[0].name));
                }
                emptyBox->addView(label);
                m_steam_results_container->addView(emptyBox);
                return;
            }

            auto* scroll = new brls::ScrollingFrame();
            scroll->setGrow(1.0f);

            auto* list = new brls::Box(brls::Axis::COLUMN);
            list->setGrow(1.0f);

            for (const auto& grid : grids) {
                auto* item = new brls::Box(brls::Axis::ROW);
                item->setFocusable(true);
                item->setHeight(104.0f);
                item->setPadding(8.0f, 16.0f, 8.0f, 16.0f);
                item->setAlignItems(brls::AlignItems::CENTER);
                item->setLineBottom(1);
                item->setLineColor(brls::Application::getTheme()["brls/sidebar/separator"]);

                auto* img = new brls::Image();
                img->setWidth(84.0f);
                img->setHeight(84.0f);
                img->setScalingType(brls::ImageScalingType::FIT);
                img->setCornerRadius(6.0f);
                img->setMarginRight(20.0f);
                item->addView(img);

                // Download thumbnail asynchronously
                std::string thumb_url = grid.thumb_url.empty() ? grid.url : grid.thumb_url;
                brls::async([this, img, thumb_url]() {
                    std::vector<uint8_t> data;
                    std::string d_err;
                    if (m_steam_client.DownloadImage(thumb_url, data, d_err)) {
                        brls::sync([img, data]() {
                            img->setImageFromMem(data.data(), static_cast<int>(data.size()));
                        });
                    }
                });

                auto* infoBox = new brls::Box(brls::Axis::COLUMN);
                infoBox->setGrow(1.0f);
                infoBox->setJustifyContent(brls::JustifyContent::CENTER);

                auto* dimLabel = new brls::Label();
                dimLabel->setFontSize(18.0f);
                dimLabel->setText(fmt::format("Dimensions : {}x{}  •  Score : {}", grid.width, grid.height, grid.score));
                infoBox->addView(dimLabel);

                auto* styleLabel = new brls::Label();
                styleLabel->setFontSize(14.0f);
                styleLabel->setMarginTop(4.0f);
                styleLabel->setTextColor(brls::Application::getTheme()["brls/text_disabled"]);
                styleLabel->setText(fmt::format("Style : {}", grid.style.empty() ? "Original" : grid.style));
                infoBox->addView(styleLabel);

                item->addView(infoBox);

                auto* applyHint = new brls::Label();
                applyHint->setFontSize(15.0f);
                applyHint->setTextColor(nvgRGB(0, 220, 255));
                applyHint->setText("Ⓐ Appliquer");
                item->addView(applyHint);

                std::string full_url = grid.url;
                item->registerClickAction([this, full_url](brls::View* v) {
                    ApplyCoverFromMemory(full_url);
                    return true;
                });

                list->addView(item);
            }

            scroll->setContentView(list);
            m_steam_results_container->addView(scroll);
        });
    });
}

void CoverSelectActivity::ApplyCoverFromMemory(const std::string& url) {
    auto* dialog = new brls::Dialog("Téléchargement et normalisation de la cover...");
    dialog->setCancelable(false);
    dialog->open();

    brls::async([this, url, dialog]() {
        std::vector<uint8_t> data;
        std::string err;
        bool ok = m_steam_client.DownloadImage(url, data, err);
        if (ok) {
            ok = JpegPipeline::ProcessAndSaveFromMemory(m_game.title_id, data.data(), data.size(), err);
        }

        brls::sync([this, ok, err, dialog]() {
            dialog->close([this, ok, err]() {
                if (ok) {
                    brls::Application::notify("Cover appliquée !");
                    if (m_on_applied) {
                        m_on_applied();
                    }
                    brls::Application::popActivity();
                } else {
                    brls::Application::notify("Erreur : " + err);
                }
            });
        });
    });
}

void CoverSelectActivity::ApplyCoverFromFile(const std::string& path) {
    auto* dialog = new brls::Dialog("Conversion en JPEG Baseline 256x256...");
    dialog->setCancelable(false);
    dialog->open();

    brls::async([this, path, dialog]() {
        std::string err;
        bool ok = JpegPipeline::ProcessAndSaveFromFile(m_game.title_id, path, err);

        brls::sync([this, ok, err, dialog]() {
            dialog->close([this, ok, err]() {
                if (ok) {
                    brls::Application::notify("Cover appliquée !");
                    if (m_on_applied) {
                        m_on_applied();
                    }
                    brls::Application::popActivity();
                } else {
                    brls::Application::notify("Erreur : " + err);
                }
            });
        });
    });
}

brls::View* CoverSelectActivity::CreateSdImportTab() {
    auto* tabRoot = new brls::Box(brls::Axis::ROW);
    tabRoot->setGrow(1.0f);
    tabRoot->setPadding(16.0f, 24.0f, 16.0f, 24.0f);

    // Scan import files
    std::vector<ImportFileItem> import_files;
    const char* dir_path = "sdmc:/config/boxart-nx/import";
    DIR* d = opendir(dir_path);
    if (!d) {
        mkdir("sdmc:/config", 0777);
        mkdir("sdmc:/config/boxart-nx", 0777);
        mkdir("sdmc:/config/boxart-nx/import", 0777);
        d = opendir(dir_path);
    }

    if (d) {
        struct dirent* entry = nullptr;
        while ((entry = readdir(d)) != nullptr) {
            if (entry->d_name[0] == '.') continue;
            std::string fname = entry->d_name;
            std::string lower = ToLower(fname);
            if (lower.ends_with(".png") || lower.ends_with(".jpg") || lower.ends_with(".jpeg") || lower.ends_with(".webp")) {
                ImportFileItem item;
                item.filename = fname;
                item.full_path = std::string(dir_path) + "/" + fname;
                import_files.push_back(std::move(item));
            }
        }
        closedir(d);
    }

    // Sort matching files first
    std::string tid_hex = m_game.title_id_hex;
    std::sort(import_files.begin(), import_files.end(), [tid_hex](const ImportFileItem& a, const ImportFileItem& b) {
        bool a_match = ToLower(a.filename).starts_with(tid_hex);
        bool b_match = ToLower(b.filename).starts_with(tid_hex);
        if (a_match != b_match) return a_match > b_match;
        return a.filename < b.filename;
    });

    if (import_files.empty()) {
        auto* emptyBox = new brls::Box(brls::Axis::COLUMN);
        emptyBox->setGrow(1.0f);
        emptyBox->setAlignItems(brls::AlignItems::CENTER);
        emptyBox->setJustifyContent(brls::JustifyContent::CENTER);

        auto* label1 = new brls::Label();
        label1->setFontSize(18.0f);
        label1->setText("Aucun fichier d'image trouvé dans :");
        emptyBox->addView(label1);

        auto* pathLabel = new brls::Label();
        pathLabel->setFontSize(16.0f);
        pathLabel->setTextColor(nvgRGB(0, 220, 255));
        pathLabel->setMarginTop(8.0f);
        pathLabel->setText("sdmc:/config/boxart-nx/import/");
        emptyBox->addView(pathLabel);

        auto* hintLabel = new brls::Label();
        hintLabel->setFontSize(14.0f);
        hintLabel->setTextColor(brls::Application::getTheme()["brls/text_disabled"]);
        hintLabel->setMarginTop(12.0f);
        hintLabel->setText("Déposez-y vos images (.png, .jpg, .webp). Nommez-les <TitleID>.png pour l'association auto !");
        emptyBox->addView(hintLabel);

        tabRoot->addView(emptyBox);
        return tabRoot;
    }

    // Left column: scrollable file list
    auto* scroll = new brls::ScrollingFrame();
    scroll->setWidth(480.0f);
    scroll->setHeight(brls::View::AUTO);

    auto* fileList = new brls::Box(brls::Axis::COLUMN);
    fileList->setGrow(1.0f);

    // Right column: preview panel
    auto* previewPanel = new brls::Box(brls::Axis::COLUMN);
    previewPanel->setGrow(1.0f);
    previewPanel->setAlignItems(brls::AlignItems::CENTER);
    previewPanel->setJustifyContent(brls::JustifyContent::CENTER);
    previewPanel->setPadding(20.0f, 20.0f, 20.0f, 20.0f);

    m_sd_preview_image = new brls::Image();
    m_sd_preview_image->setWidth(256.0f);
    m_sd_preview_image->setHeight(256.0f);
    m_sd_preview_image->setScalingType(brls::ImageScalingType::FIT);
    m_sd_preview_image->setCornerRadius(8.0f);
    previewPanel->addView(m_sd_preview_image);

    m_sd_preview_label = new brls::Label();
    m_sd_preview_label->setFontSize(16.0f);
    m_sd_preview_label->setMarginTop(16.0f);
    m_sd_preview_label->setTextColor(brls::Application::getTheme()["brls/text"]);
    previewPanel->addView(m_sd_preview_label);

    auto* applyBtn = new brls::Button();
    applyBtn->setText("Appliquer cette cover");
    applyBtn->setStyle(&brls::BUTTONSTYLE_PRIMARY);
    applyBtn->setMarginTop(20.0f);
    applyBtn->registerClickAction([this](brls::View* v) {
        if (!m_selected_import_path.empty()) {
            ApplyCoverFromFile(m_selected_import_path);
        }
        return true;
    });
    previewPanel->addView(applyBtn);

    // Populate file list
    m_selected_import_path = import_files[0].full_path;
    m_sd_preview_image->setImageFromFile(m_selected_import_path);
    m_sd_preview_label->setText(import_files[0].filename);

    for (const auto& file : import_files) {
        auto* row = new brls::Box(brls::Axis::ROW);
        row->setFocusable(true);
        row->setHeight(56.0f);
        row->setPadding(8.0f, 16.0f, 8.0f, 16.0f);
        row->setAlignItems(brls::AlignItems::CENTER);
        row->setJustifyContent(brls::JustifyContent::SPACE_BETWEEN);
        row->setLineBottom(1);
        row->setLineColor(brls::Application::getTheme()["brls/sidebar/separator"]);

        auto* nameLabel = new brls::Label();
        nameLabel->setFontSize(16.0f);
        nameLabel->setText(file.filename);
        row->addView(nameLabel);

        bool is_match = ToLower(file.filename).starts_with(tid_hex);
        if (is_match) {
            auto* matchBadge = new brls::Label();
            matchBadge->setFontSize(14.0f);
            matchBadge->setTextColor(nvgRGB(0, 220, 255));
            matchBadge->setText("★ Auto-match");
            row->addView(matchBadge);
        }

        std::string path = file.full_path;
        std::string fname = file.filename;

        // On click or focus, update preview
        row->registerClickAction([this, path, fname](brls::View* v) {
            m_selected_import_path = path;
            if (m_sd_preview_image) m_sd_preview_image->setImageFromFile(path);
            if (m_sd_preview_label) m_sd_preview_label->setText(fname);
            ApplyCoverFromFile(path);
            return true;
        });

        fileList->addView(row);
    }

    scroll->setContentView(fileList);
    tabRoot->addView(scroll);
    tabRoot->addView(previewPanel);

    return tabRoot;
}

} // namespace BoxArt
