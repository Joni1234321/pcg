#pragma once

#include "r_engine.hpp"
#include "u_types.hpp"

namespace pcg::frame {

class MainMenuFrame {
    std::vector<pce::ui::Element::Handle> elements { };
public:
    MainMenuFrame(pce::ui::UISystem& ui_system, const pce::ui::FontCollection& font);
    void Tick(u32 i, pce::ui::UISystem& ui_system);
};

class FPSFrame {
public:
    FPSFrame(pce::ui::UISystem& ui_system, const pce::ui::FontCollection& font);
    void Tick(u32 i, pce::ui::UISystem& ui_system);
private:
    pce::ui::TextElement::Handle tick_text;
};
}
