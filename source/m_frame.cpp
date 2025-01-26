#include "m_frame.hpp"

#include <ranges>
#include <g_components.hpp>
#include <u_algorithm.hpp>

namespace pcg::frame {
namespace ui = pce::ui;
namespace colors = pce::colors;
void TestNodeExample() {
//    ui::NodeBuilder().Fill(ui::Fill (0x12, 0x12, 0x12, 0xFF));
}
FPSFrame::FPSFrame(ui::UISystem& ui_system) : tick_text { ui_system.CreateText("", ui_system.font.small, colors::blue, 10.0F, 0.0F) } { }
void FPSFrame::Tick(u32 i, ui::UISystem& ui_system) {
    const std::string str = std::format("Tick {:6}", i);
    (void)TTF_SetTextString(ui_system[tick_text].text, str.c_str(), str.length());
}

MainMenuFrame::MainMenuFrame(ui::UISystem& ui_system) {
    constexpr f32 title_y = 30.0F;
    (void)ui_system.CreateTextAligned("Hey Helene!", ui_system.font_bold.h1, colors::light_sky_blue, 0, title_y, ui::TextAlign::center, ui_system.screen_width);

    pce::Table main_menu = pce::Table("Main Menu");

    constexpr f32 menu_x = 200.0F;
    constexpr f32 list_start_y = 100.0F;
    pce::List<pce::String> buttons = { "Play", "Settings", "Exit" };
    ui_system.CreateText("Play", ui_system.font_bold.large, colors::blue, menu_x, list_start_y);
    ui_system.CreateText("Settings", ui_system.font.large, colors::black, menu_x, list_start_y + 50.0F);
    ui_system.CreateText("Exit", ui_system.font.large, colors::red, menu_x, list_start_y + 100.0F);

    constexpr SDL_FRect background_rect { .x = menu_x, .y = list_start_y, .w = 100.0F, .h = 300.0F };
    ui_system.CreateElement(background_rect, colors::gray, nullptr);

    main_menu.AddColumn("Fish Column", pce::List<pce::String> { "Hej", "Fem", "Femten" });
    ui_system.CreateTable(main_menu, colors::sea_green, ui_system.screen_width / 2.0F - 100.0F, 300.0F);

    SDL_FRect rect { .x = 200.0F, .y = 200.0F, .w = 30.0F, .h = 30.0F };

    (void)elements.emplace_back(ui_system.CreateElement(rect, colors::black, nullptr));

    rect.x += 200;
    (void)elements.emplace_back(ui_system.CreateElement(rect, colors::red, nullptr));

    rect.y = 400;
    ui_system.CreateList(colors::white, rect, 10U, 25.0F, ui::UISystem::ListDirection::vertical);
    rect.h *= 2;
    ui_system.CreateList(colors::green, rect, 10U, 25.0F, ui::UISystem::ListDirection::horizontal);

    ui::Node::Handle frame = ui::NodeBuilder(uint2{ 100U, 100U }).Gap(10U).BuildRoot(tree, { 600U, 100U });

    {
        ui::Node::Handle root = ui::NodeBuilder(uint2{ 100U, 100U }).Padding({ 5U, 5U }).Fill(colors::cyan).Build(tree, frame);
        ui::Node::Handle box1 = ui::NodeBuilder(ui::LayoutLength::fill).Fill(colors::yellow).Padding({ 5U, 5U}).Build(tree, root);
        ui::Node::Handle box11 = ui::NodeBuilder(ui::LayoutLength::fill).Fill(colors::blue).Build(tree, box1);
        ui::Node::Handle box12 = ui::NodeBuilder(ui::LayoutLength::fill).Fill(colors::chocolate).Build(tree, box1);

        ui::Node::Handle box2 = ui::NodeBuilder(ui::LayoutLength::fill).Fill(colors::red).Build(tree, root);
        ui::Node::Handle box3 = ui::NodeBuilder(ui::LayoutLength::hug, ui::LayoutLength::fill).Padding({ 10U, 10U }).Fill(colors::black).Build(tree, root);
    }
    {
        ui::Node::Handle root = ui::NodeBuilder(uint2{ 100U, 100U }).Padding({ 5U, 5U }).Fill(colors::cyan).Build(tree, frame);
        ui::Node::Handle box1 = ui::NodeBuilder(ui::LayoutLength::fill).Fill(colors::yellow).Padding({ 5U, 5U}).Build(tree, root);
        ui::Node::Handle box2 = ui::NodeBuilder(ui::LayoutLength::fill).Fill(colors::red).Build(tree, root);
    }
    {
        ui::Node::Handle root = ui::NodeBuilder(uint2{ 100U, 100U }).Padding({ 5U, 5U }).Fill(colors::cyan).Build(tree, frame);
        ui::Node::Handle box1 = ui::NodeBuilder(ui::LayoutLength::fill).Fill(colors::yellow).Padding({ 5U, 5U}).Build(tree, root);
        ui::Node::Handle box2 = ui::NodeBuilder(ui::LayoutLength::fill).Fill(colors::red).Build(tree, root);
    }
    {
        ui::Node::Handle root = ui::NodeBuilder(uint2{ 100U, 100U }).Padding({ 5U, 5U }).Fill(colors::cyan).Build(tree, frame);
        ui::Node::Handle box1 = ui::NodeBuilder(ui::LayoutLength::fill).Fill(colors::yellow).Padding({ 5U, 5U}).Build(tree, root);
        ui::Node::Handle box2 = ui::NodeBuilder(ui::LayoutLength::fill).Fill(colors::red).Build(tree, root);
        ui::Node::Handle box3 = ui::NodeBuilder(ui::LayoutLength::hug).Padding({ 10U, 10U }).Fill(colors::black).Build(tree, root);
    }
    {
        constexpr u32 width = 100U;
        ui::Node::Handle root = ui::NodeBuilder(ui::LayoutLength::hug, 400U).Padding({ 5U, 5U }).Fill(colors::forest_green).Build(tree, frame);
        ui::Node::Handle box1 = ui::NodeBuilder(width, ui::LayoutLength::fill).Fill(colors::yellow).Padding({ 5U, 5U}).Build(tree, root);
        ui::Node::Handle box2 = ui::NodeBuilder(width * 2, ui::LayoutLength::fill).Fill(colors::red).Build(tree, root);
        ui::Node::Handle box3 = ui::NodeBuilder(width * 3, ui::LayoutLength::fill).Padding({ 4U, 4U }).Gap(2U).Fill(colors::black).Build(tree, root);
        ui::Node::Handle box31 = ui::NodeBuilder(ui::LayoutLength::fill).Fill(colors::red).Build(tree, box3);
        ui::Node::Handle box32 = ui::NodeBuilder(ui::LayoutLength::fill).Fill(colors::chocolate).Build(tree, box3);
        ui::Node::Handle box33 = ui::NodeBuilder(ui::LayoutLength::fill).Fill(colors::yellow).Build(tree, box3);
    }



    tree.MarkDirty();
}
void MainMenuFrame::Tick(const u32 i, ui::UISystem& ui_system) {
    for (const ui::RectangleElement::Handle sqr : elements) { ui_system[sqr].color = SDL_Color { static_cast<u8>(i / 3U), static_cast<u8>(i / 10U), static_cast<u8>(i / 67U), 255 }; }
}

pce::Table CreateFarmTable() {
    pce::Table table { "Farms" };
    table.AddColumn("Type ", farm_archetype.types);
    table.AddColumn("Assets       ", Select(farm_archetype.finances, [&] (const Finance& finance) -> Money { return finance.assets.Total(); }));
    table.AddColumn("Equity       ", Select(farm_archetype.finances, [&] (const Finance& finance) -> Money { return finance.equity; }));
    table.AddColumn("Liabilities  ", Select(farm_archetype.finances, [&] (const Finance& finance) -> Money { return finance.liabilities; }));
    table.AddColumn("Last result  ", Select(farm_archetype.finances, [&] (const Finance& finance) -> Money { return finance.last_result; }));
    table.AddColumn("Population balance", farm_archetype.population_balance);
    return table;
}
OverviewFrame::OverviewFrame(ui::UISystem& ui_system) {
    constexpr f32 table_x = 200.0F;
    constexpr f32 table_y = 200.0F;
    const pce::Table table = CreateFarmTable();
    table_handle = ui_system.CreateTable(table, colors::light_sky_blue, table_x, table_y);
}
void OverviewFrame::Tick(ui::UISystem& ui_system) {
    const pce::Table table = CreateFarmTable();
    ui_system.UpdateTable(table_handle, table);
}
}
