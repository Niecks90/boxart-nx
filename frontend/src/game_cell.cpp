/*
 * Copyright (c) 2026 boxart-nx contributors
 * Distributed under the terms of the GNU General Public License v2
 */

#include "game_cell.hpp"
#include "jpeg_pipeline.hpp"

namespace BoxArt {

GameCell::GameCell() {
    this->setFocusable(true);
    this->setHeight(92.0f);
    this->setAxis(brls::Axis::ROW);
    this->setAlignItems(brls::AlignItems::CENTER);
    this->setJustifyContent(brls::JustifyContent::FLEX_START);
    this->setPadding(10.0f, 24.0f, 10.0f, 24.0f);

    // Thumbnail icon (68x68)
    m_thumbnail = new brls::Image();
    m_thumbnail->setWidth(68.0f);
    m_thumbnail->setHeight(68.0f);
    m_thumbnail->setScalingType(brls::ImageScalingType::FIT);
    m_thumbnail->setCornerRadius(6.0f);
    m_thumbnail->setMarginRight(20.0f);
    this->addView(m_thumbnail);

    // Text details (Title + Author / Version / TitleID)
    auto* textBox = new brls::Box(brls::Axis::COLUMN);
    textBox->setGrow(1.0f);
    textBox->setJustifyContent(brls::JustifyContent::CENTER);

    m_title_label = new brls::Label();
    m_title_label->setFontSize(20.0f);
    m_title_label->setTextColor(brls::Application::getTheme()["brls/text"]);
    textBox->addView(m_title_label);

    m_meta_label = new brls::Label();
    m_meta_label->setFontSize(14.0f);
    m_meta_label->setMarginTop(4.0f);
    m_meta_label->setTextColor(brls::Application::getTheme()["brls/text_disabled"]);
    textBox->addView(m_meta_label);

    this->addView(textBox);

    // Custom cover badge
    m_custom_badge = new brls::Label();
    m_custom_badge->setText("[Custom]");
    m_custom_badge->setFontSize(15.0f);
    m_custom_badge->setTextColor(nvgRGB(0, 220, 255)); // Horizon cyan
    m_custom_badge->setMarginLeft(16.0f);
    this->addView(m_custom_badge);
}

void GameCell::bind(const GameItem& game) {
    m_title_label->setText(game.name);
    m_meta_label->setText(game.author + "  |  v" + game.version + "  |  " + game.title_id_hex);

    if (game.has_custom_cover) {
        m_custom_badge->setVisibility(brls::Visibility::VISIBLE);
        std::string cover_path = JpegPipeline::GetCoverPath(game.title_id);
        m_thumbnail->setImageFromFile(cover_path);
    } else {
        m_custom_badge->setVisibility(brls::Visibility::GONE);
        if (!game.original_icon_jpeg.empty()) {
            m_thumbnail->setImageFromMem(game.original_icon_jpeg.data(), static_cast<int>(game.original_icon_jpeg.size()));
        } else {
            m_thumbnail->clear();
        }
    }
}

void GameCell::prepareForReuse() {
    if (m_thumbnail) {
        m_thumbnail->clear();
    }
}

void GameCell::onFocusGained() {
    brls::RecyclerCell::onFocusGained();
    if (m_on_focus) {
        m_on_focus(this->getIndexPath().row);
    }
}

GameCell* GameCell::create() {
    return new GameCell();
}

} // namespace BoxArt
