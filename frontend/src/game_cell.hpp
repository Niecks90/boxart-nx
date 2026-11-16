/*
 * Copyright (c) 2026 boxart-nx contributors
 * Distributed under the terms of the GNU General Public License v2
 */

#pragma once

#include <borealis.hpp>
#include "inventory.hpp"

namespace BoxArt {

class GameCell : public brls::RecyclerCell {
public:
    GameCell();
    ~GameCell() override = default;

    void bind(const GameItem& game);
    void prepareForReuse() override;

    void onFocusGained() override;

    void setOnFocusCallback(std::function<void(int row)> cb) {
        m_on_focus = cb;
    }

    static GameCell* create();

private:
    brls::Image* m_thumbnail = nullptr;
    brls::Label* m_title_label = nullptr;
    brls::Label* m_meta_label = nullptr;
    brls::Label* m_custom_badge = nullptr;

    std::function<void(int row)> m_on_focus;
};

} // namespace BoxArt
