#pragma once

#include "collections.hpp"
#include "engine.hpp"
#include "types.hpp"

namespace pcg::frame {
pce::ui::TextElement::Handle tick_handle;

class MainMenuFrame {
    std::vector<pce::ui::Element::Handle> elements { };

public:
    MainMenuFrame() {
        namespace ui = pce::ui;

        (void)ui::CreateText("Hey Helene!", ui::font.h1, ui::colors::light_sky_blue, 30.0F, 30.0F);

        SDL_FRect rect { .x = 200, .y = 200, .w = 30, .h = 30 };

        (void)elements.emplace_back(ui::CreateElement(rect, ui::colors::black));

        rect.x += 200;
        (void)elements.emplace_back(ui::CreateElement(rect, ui::colors::black));

    }

    void Tick(u32 i) {
        namespace ui = pce::ui;
        for (const ui::Element::Handle& handle : elements) { ui::elements[handle.id].color = SDL_Color {static_cast<u8>(i / 3), static_cast<u8>(i / 10), static_cast<u8>(i / 67), 255 }; }
    }
};

class FPSFrame {
public:
    FPSFrame() {
        namespace ui = pce::ui;

        tick_handle = ui::CreateText("", ui::font.small, ui::colors::blue, 10.0F, 0.0F);
    }
    void Tick(u32 i) {
        namespace ui = pce::ui;

        const std::string str = std::format("Tick {:6}", i);
        (void)TTF_SetTextString(ui::textElements[tick_handle.id].text, str.c_str(), str.length());
    }
};
}
