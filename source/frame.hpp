#pragma once

#include "engine.hpp"
#include "collections.hpp"
#include "types.hpp"

namespace pcg::frame {
pce::ui::TextElement::Handle tick_handle;
pce::ui::Element::Handle element_handle;

void MainMenuBuild() {
    namespace ui = pce::ui;
    using pce::String;

    const String welcome = "Welcome to PCG!\nFISH 2 \n THE SEQUEL";
    constexpr SDL_FColor textColor { 0.0F, 0.0F, 1.0F, 1.0F };
    tick_handle = ui::CreateText(welcome, ui::font.small, textColor, 10.0F, 0.0F);
    element_handle = ui::CreateElement(SDL_FRect { .x = 400, .y = 200, .w = 30, .h = 30 }, textColor);

    ui::CreateText(welcome, ui::font.normal, textColor, 10.0F, 10.0F);
    ui::CreateElement(SDL_FRect { .x = 100, .y = 200, .w = 30, .h = 30 },textColor);
}

void MainMenuTick(u32 i) {
    namespace ui = pce::ui;

    ui::elements[element_handle.id].color = SDL_FColor { i++ % 3 / 3.0F, i % 10 / 10.0F, i % 67 / 67.0F, 100.0F};
    const std::string str = std::format("Tick {:6}", i);
    TTF_SetTextString(ui::textElements[tick_handle.id].text, str.c_str(), str.length());
}
}
namespace pcg::screen {

}