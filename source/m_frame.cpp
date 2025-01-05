#include "m_frame.hpp"

#include <g_components.hpp>
#include <u_algorithm.hpp>

namespace pcg::frame {
namespace ui = pce::ui;
namespace colors = pce::colors;
FPSFrame::FPSFrame(ui::UISystem& ui_system, const ui::FontCollection& font) : tick_text { ui_system.CreateText("", font.small, colors::blue, 10.0F, 0.0F, ui::TextAlign::center, 0U) } { }
void FPSFrame::Tick(u32 i, ui::UISystem& ui_system) {
    const std::string str = std::format("Tick {:6}", i);
    (void)TTF_SetTextString(ui_system[tick_text].text, str.c_str(), str.length());
}

MainMenuFrame::MainMenuFrame(ui::UISystem& ui_system, const ui::FontCollection& font) {
    (void)ui_system.CreateText("Hey Helene!", font.h1, colors::light_sky_blue, 30.0F, 30.0F, ui::TextAlign::left, 0U);

    SDL_FRect rect { .x = 200, .y = 200, .w = 30, .h = 30 };

    (void)elements.emplace_back(ui_system.CreateElement(rect, colors::black, nullptr));

    rect.x += 200;
    (void)elements.emplace_back(ui_system.CreateElement(rect, colors::black, nullptr));

    rect.y = 400;
    ui_system.CreateList(colors::white, rect, 10U, 25.0F, ui::UISystem::ListDirection::vertical);
    rect.h *= 2;
    ui_system.CreateList(colors::green, rect, 10U, 25.0F, ui::UISystem::ListDirection::horizontal);

}
void MainMenuFrame::Tick(u32 i, ui::UISystem& ui_system) {
    for (const ui::Element::Handle& sqr : elements) { ui_system[sqr].color = SDL_Color { static_cast<u8>(i / 3), static_cast<u8>(i / 10), static_cast<u8>(i / 67), 255 }; }
}


pce::Table CreateFarmTable() {
    pce::Table table { "Farms" };
    table.AddColumn("Type ", farm_archetype.types);
    table.AddColumn("Assets       ", Select(farm_archetype.finances, [] (const Finance& finance) -> Money { return finance.assets.Total(); }));
    table.AddColumn("Equity       ", Select(farm_archetype.finances, [] (const Finance& finance) -> Money { return finance.equity; }));
    table.AddColumn("Liabilities  ", Select(farm_archetype.finances, [] (const Finance& finance) -> Money { return finance.liabilities; }));
    table.AddColumn("Last result  ", Select(farm_archetype.finances, [] (const Finance& finance) -> Money { return finance.last_result; }));
    table.AddColumn("Population balance", farm_archetype.population_balance);
    return table;
}
OverviewFrame::OverviewFrame(ui::UISystem &ui_system, const ui::FontCollection &font) {
    constexpr f32 table_x = 200.0F;
    constexpr f32 table_y = 200.0F;
    const pce::Table table = CreateFarmTable();
    table_handle = ui_system.CreateTable(table, font, colors::light_sky_blue, table_x, table_y);
}

void OverviewFrame::Tick(ui::UISystem& ui_system) {
    const pce::Table table = CreateFarmTable();
    ui_system.UpdateTable(table_handle, table);
}
}
