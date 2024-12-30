#include "m_frame.hpp"

namespace pcg::frame {
namespace ui = pce::ui;
FPSFrame::FPSFrame(ui::UISystem& ui_system, const ui::FontCollection& font) : tick_text { ui_system.CreateText("", font.small, ui::colors::blue, 10.0F, 0.0F) } { }
void FPSFrame::Tick(u32 i, ui::UISystem& ui_system) {
    const std::string str = std::format("Tick {:6}", i);
    (void)TTF_SetTextString(ui_system[tick_text].text, str.c_str(), str.length());
}

MainMenuFrame::MainMenuFrame(ui::UISystem& ui_system, const ui::FontCollection& font) {
    (void)ui_system.CreateText("Hey Helene!", font.h1, ui::colors::light_sky_blue, 30.0F, 30.0F);

    SDL_FRect rect { .x = 200, .y = 200, .w = 30, .h = 30 };

    (void)elements.emplace_back(ui_system.CreateElement(rect, ui::colors::black, nullptr));

    rect.x += 200;
    (void)elements.emplace_back(ui_system.CreateElement(rect, ui::colors::black, nullptr));
}
void MainMenuFrame::Tick(u32 i, ui::UISystem& ui_system) {
    for (const ui::Element::Handle& sqr : elements) { ui_system[sqr].color = SDL_Color { static_cast<u8>(i / 3), static_cast<u8>(i / 10), static_cast<u8>(i / 67), 255 }; }
}
}
