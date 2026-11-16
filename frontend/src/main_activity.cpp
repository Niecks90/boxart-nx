/*
 * Copyright (c) 2026 boxart-nx contributors
 * Distributed under the terms of the GNU General Public License v2
 */

#include "main_activity.hpp"
#include "game_cell.hpp"
#include "cover_select_activity.hpp"
#include "jpeg_pipeline.hpp"

#include <borealis/views/progress_spinner.hpp>
#include <borealis/views/dialog.hpp>

namespace BoxArt {

class GamesDataSource : public brls::RecyclerDataSource {
public:
    GamesDataSource(Inventory& inventory,
                    std::function<void(int row)> on_select,
                    std::function<void(int row)> on_focus)
        : m_inventory(inventory), m_on_select(on_select), m_on_focus(on_focus) {}

    int numberOfRows(brls::RecyclerFrame* recycler, int section) override {
        return static_cast<int>(m_inventory.GetGames().size());
    }

    float heightForRow(brls::RecyclerFrame* recycler, brls::IndexPath index) override {
        return 92.0f;
    }

    brls::RecyclerCell* cellForRow(brls::RecyclerFrame* recycler, brls::IndexPath index) override {
        GameCell* cell = dynamic_cast<GameCell*>(recycler->dequeueReusableCell("GameCell"));
        if (!cell) {
            cell = new GameCell();
        }
        cell->setOnFocusCallback(m_on_focus);
        if (index.row >= 0 && index.row < static_cast<int>(m_inventory.GetGames().size())) {
            cell->bind(m_inventory.GetGames()[index.row]);
        }
        return cell;
    }

    void didSelectRowAt(brls::RecyclerFrame* recycler, brls::IndexPath index) override {
        if (m_on_select && index.row >= 0 && index.row < static_cast<int>(m_inventory.GetGames().size())) {
            m_on_select(index.row);
        }
    }

private:
    Inventory& m_inventory;
    std::function<void(int row)> m_on_select;
    std::function<void(int row)> m_on_focus;
};

MainActivity::MainActivity() {
    // Main content container
    m_content_box = new brls::Box(brls::Axis::COLUMN);
    m_content_box->setGrow(1.0f);

    // Initial loading container with spinner
    m_loading_box = new brls::Box(brls::Axis::COLUMN);
    m_loading_box->setGrow(1.0f);
    m_loading_box->setAlignItems(brls::AlignItems::CENTER);
    m_loading_box->setJustifyContent(brls::JustifyContent::CENTER);

    auto* spinner = new brls::ProgressSpinner(brls::ProgressSpinnerSize::LARGE);
    spinner->animate(true);
    m_loading_box->addView(spinner);

    m_loading_label = new brls::Label();
    m_loading_label->setFontSize(18.0f);
    m_loading_label->setMarginTop(24.0f);
    m_loading_label->setTextColor(brls::Application::getTheme()["brls/text"]);
    m_loading_label->setText("Recherche des titres installés...");
    m_loading_box->addView(m_loading_label);

    m_content_box->addView(m_loading_box);

    m_applet_frame = new brls::AppletFrame(m_content_box);
    m_applet_frame->setTitle("boxart-nx");

    // Add subtitle in the header next/below title
    auto* header = m_applet_frame->getHeader();
    if (header && !header->getChildren().empty()) {
        auto* titleBox = dynamic_cast<brls::Box*>(header->getChildren()[0]);
        if (titleBox) {
            titleBox->setAxis(brls::Axis::COLUMN);
            titleBox->setJustifyContent(brls::JustifyContent::CENTER);

            m_subtitle_label = new brls::Label();
            m_subtitle_label->setFontSize(14.0f);
            m_subtitle_label->setMarginTop(3.0f);
            m_subtitle_label->setTextColor(brls::Application::getTheme()["brls/text_disabled"]);
            m_subtitle_label->setText("Chargement des jeux installés...");
            titleBox->addView(m_subtitle_label);
        }
    }

    this->setContentView(m_applet_frame);

    // Register footer action hints
    this->registerAction("Changer la cover", brls::BUTTON_A, [this](brls::View* v) {
        OpenCoverSelection(m_selected_row);
        return true;
    });

    this->registerAction("Restaurer l'icône d'origine", brls::BUTTON_X, [this](brls::View* v) {
        PromptRestoreOriginalIcon(m_selected_row);
        return true;
    });

    this->registerAction("Quitter", brls::BUTTON_B, [](brls::View* v) {
        brls::Application::quit();
        return true;
    });

    // Start background scan immediately
    StartInventoryScan();
}

bool MainActivity::CheckSysmoduleActive(uint64_t* out_pid) {
    constexpr uint64_t SYSMODULE_TITLE_ID = 0x010000000000B0A7ul;
    uint64_t pid = 0;

    if (R_SUCCEEDED(pmdmntInitialize())) {
        Result rc = pmdmntGetProcessId(&pid, SYSMODULE_TITLE_ID);
        pmdmntExit();
        if (R_SUCCEEDED(rc) && pid != 0) {
            if (out_pid) *out_pid = pid;
            return true;
        }
    }

    if (R_SUCCEEDED(pmshellInitialize())) {
        Result rc = pmshellGetProcessId(&pid, SYSMODULE_TITLE_ID);
        pmshellExit();
        if (R_SUCCEEDED(rc) && pid != 0) {
            if (out_pid) *out_pid = pid;
            return true;
        }
    }

    return false;
}

void MainActivity::StartInventoryScan() {
    brls::async([this]() {
        m_inventory.ReloadGames([this](int current, int total) {
            brls::sync([this, current, total]() {
                if (m_loading_label) {
                    if (total > 0) {
                        m_loading_label->setText(fmt::format("Chargement des titres ({}/{})...", current, total));
                    } else {
                        m_loading_label->setText("Scan des titres...");
                    }
                }
            });
        });

        uint64_t pid = 0;
        bool sys_active = CheckSysmoduleActive(&pid);

        brls::sync([this, sys_active, pid]() {
            OnInventoryLoaded(sys_active, pid);
        });
    });
}

void MainActivity::OnInventoryLoaded(bool sys_active, uint64_t pid) {
    // Remove loading container
    if (m_loading_box) {
        m_content_box->removeView(m_loading_box);
        m_loading_box = nullptr;
    }

    const size_t count = m_inventory.GetGames().size();

    // Update header subtitle
    if (m_subtitle_label) {
        std::string status_str;
        if (sys_active) {
            status_str = fmt::format("{} jeux détectés  •  Sysmodule MitM : Actif (PID {})", count, pid);
        } else {
            status_str = fmt::format("{} jeux détectés  •  Sysmodule MitM : Inactif", count);
        }
        m_subtitle_label->setText(status_str);
    }

    if (count == 0) {
        auto* emptyBox = new brls::Box(brls::Axis::COLUMN);
        emptyBox->setGrow(1.0f);
        emptyBox->setAlignItems(brls::AlignItems::CENTER);
        emptyBox->setJustifyContent(brls::JustifyContent::CENTER);

        auto* emptyLabel = new brls::Label();
        emptyLabel->setFontSize(20.0f);
        emptyLabel->setTextColor(brls::Application::getTheme()["brls/text_disabled"]);
        emptyLabel->setText("Aucun jeu installé détecté sur la console.");
        emptyBox->addView(emptyLabel);

        m_content_box->addView(emptyBox);
        return;
    }

    // Create RecyclerFrame
    m_recycler = new brls::RecyclerFrame();
    m_recycler->setGrow(1.0f);
    m_recycler->estimatedRowHeight = 92.0f;

    m_recycler->registerCell("GameCell", []() {
        return GameCell::create();
    });

    m_recycler->setDataSource(new GamesDataSource(
        m_inventory,
        [this](int row) {
            m_selected_row = row;
            OpenCoverSelection(row);
        },
        [this](int row) {
            m_selected_row = row;
        }
    ));

    m_content_box->addView(m_recycler);
    m_selected_row = 0;
    brls::Application::giveFocus(m_recycler);
}

void MainActivity::OpenCoverSelection(int row) {
    if (row < 0 || row >= static_cast<int>(m_inventory.GetGames().size())) {
        return;
    }

    auto& game = m_inventory.GetGames()[row];
    brls::Application::pushActivity(new CoverSelectActivity(game, [this, row]() {
        m_inventory.GetGames()[row].has_custom_cover = true;
        if (m_recycler) {
            m_recycler->reloadData();
        }
    }));
}

void MainActivity::PromptRestoreOriginalIcon(int row) {
    if (row < 0 || row >= static_cast<int>(m_inventory.GetGames().size())) {
        return;
    }

    auto& game = m_inventory.GetGames()[row];
    if (!game.has_custom_cover) {
        brls::Application::notify("Ce jeu utilise déjà son icône d'origine.");
        return;
    }

    std::string prompt = "Voulez-vous restaurer l'icône originale de :\n" + game.name + " ?";
    auto* dialog = new brls::Dialog(prompt);

    dialog->addButton("Annuler", []() {});

    dialog->addButton("Restaurer", [this, &game]() {
        std::string err;
        if (JpegPipeline::RevertCustomCover(game.title_id, err)) {
            game.has_custom_cover = false;
            brls::Application::notify("Icône d'origine restaurée !");
            if (m_recycler) {
                m_recycler->reloadData();
            }
        } else {
            brls::Application::notify("Erreur : " + err);
        }
    });

    dialog->open();
}

} // namespace BoxArt
