#pragma once

#include "r_engine.hpp"
#include "r_ui.hpp"
#include "u_types.hpp"

namespace pcg::frame {

class MainMenuFrame {
    std::vector<pce::ui::RectangleElement::Handle> elements { };
public:
    pce::ui::Node root;

    MainMenuFrame(pce::ui::UISystem& ui_system);
    void Tick(u32 i, pce::ui::UISystem& ui_system);
};

class FPSFrame {
    pce::ui::TextElement::Handle tick_text;
public:
    FPSFrame(pce::ui::UISystem& ui_system);
    void Tick(u32 i, pce::ui::UISystem& ui_system);
};

class OverviewFrame {
    pce::ui::TableElement::Handle table_handle;
public:
    OverviewFrame(pce::ui::UISystem& ui_system);
    void Tick(pce::ui::UISystem& ui_system);
};
}
