#pragma once

#include "collections.hpp"
#include "engine.hpp"
#include "types.hpp"

namespace pcg::frame {
pce::ui::TextElement::Handle tick_text;

class MainMenuFrame {
    std::vector<pce::ui::Element::Handle> elements { };

public:
    MainMenuFrame(pce::ui::UISystem& ui_system, const pce::ui::FontCollection& font) {
        namespace ui = pce::ui;

        (void)ui_system.CreateText("Hey Helene!", font.h1, ui::colors::light_sky_blue, 30.0F, 30.0F);

        SDL_FRect rect { .x = 200, .y = 200, .w = 30, .h = 30 };

        (void)elements.emplace_back(ui_system.CreateElement(rect, ui::colors::black));

        rect.x += 200;
        (void)elements.emplace_back(ui_system.CreateElement(rect, ui::colors::black));

    }

    void Tick(u32 i, pce::ui::UISystem& ui_system) {
        namespace ui = pce::ui;
        for (const ui::Element::Handle& sqr : elements) { ui_system[sqr].color = SDL_Color {static_cast<u8>(i / 3), static_cast<u8>(i / 10), static_cast<u8>(i / 67), 255 }; }
    }
};

class FPSFrame {
public:
    FPSFrame(pce::ui::UISystem& ui_system, const pce::ui::FontCollection& font) {
        namespace ui = pce::ui;

        tick_text = ui_system.CreateText("", font.small, ui::colors::blue, 10.0F, 0.0F);
    }
    void Tick(u32 i, pce::ui::UISystem& ui_system) {
        namespace ui = pce::ui;

        const std::string str = std::format("Tick {:6}", i);
        (void)TTF_SetTextString(ui_system[tick_text].text, str.c_str(), str.length());
    }
};
}
