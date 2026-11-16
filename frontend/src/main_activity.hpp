/*
 * Copyright (c) 2026 boxart-nx contributors
 * Distributed under the terms of the GNU General Public License v2
 */

#pragma once

#include <borealis.hpp>
#include "inventory.hpp"

namespace BoxArt {

class MainActivity : public brls::Activity {
public:
    MainActivity();
    ~MainActivity() override = default;

private:
    void StartInventoryScan();
    void OnInventoryLoaded(bool sys_active, uint64_t pid);
    void OpenCoverSelection(int row);
    void PromptRestoreOriginalIcon(int row);
    bool CheckSysmoduleActive(uint64_t* out_pid);

    Inventory m_inventory;
    int m_selected_row = 0;

    brls::AppletFrame* m_applet_frame = nullptr;
    brls::Label* m_subtitle_label = nullptr;
    brls::Box* m_content_box = nullptr;
    brls::Box* m_loading_box = nullptr;
    brls::Label* m_loading_label = nullptr;
    brls::RecyclerFrame* m_recycler = nullptr;
};

} // namespace BoxArt
