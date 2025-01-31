#include "m_frame.hpp"

#include <ranges>
#include <g_components.hpp>
#include <stack>
#include <u_algorithm.hpp>

namespace pcg::frame {
namespace ui = pce::ui;
namespace colors = pce::colors;

ui::Node::Handle CreateDebugNodeComponent(const u32 layer, const pce::String& text, const SDL_Color color, ui::NodeTree& tree, const ui::Node::Handle frame) {
    constexpr u32 padding_offset = 5U;
    constexpr uint2 color_indicator_size { 15U, 15U };
    constexpr u32 gap_size { 2U };
    ui::Node::Handle component_handle = ui::NodeBuilder(ui::hug).Fill(colors::forest_green).Padding({ padding_offset * layer, 0U }).Gap(gap_size).Build(tree, frame);
    ui::NodeBuilder(color_indicator_size).Fill(color).Build(tree, component_handle);
    ui::NodeBuilder(ui::hug).Text(text).Build(tree, component_handle);
    return component_handle;
}
void DebugNode(ui::NodeTree& output_tree, const ui::NodeTree& tree, const ui::Node::OptionalHandle root_handle) {
    using NodeHandleLayer = std::tuple<ui::Node::Handle, u32>;

    output_tree.Clear();

    if (!root_handle.IsValid()) { return; }

    pce::Stack<NodeHandleLayer> node_handles;
    node_handles.push(NodeHandleLayer { root_handle.GetHandle(), 0U });

    ui::Node::Handle frame = ui::NodeBuilder(ui::hug).Fill(colors::clear).Fill(colors::white).Direction(ui::vertical).BuildRoot(output_tree, { 10U, 30U });
    while (!node_handles.empty()) {
        const auto [node_handle, layer] = node_handles.top();
        node_handles.pop();
        const ui::Node& node = tree.GetNode(node_handle);
        for (const ui::Node::Handle child_handle : tree.Children(node_handle)) { node_handles.push(NodeHandleLayer { child_handle, layer + 1 }); }

        CreateDebugNodeComponent(layer, "Node", node.background_color, output_tree, frame);
        if (node.HasText()) { CreateDebugNodeComponent(layer + 1, "Text", node.background_color, output_tree, frame); }
    }
}
void TestNodeExample() {
//    ui::NodeBuilder().Fill(ui::Fill (0x12, 0x12, 0x12, 0xFF));
}
FPSFrame::FPSFrame(ui::UISystem& ui_system) : tick_text { ui_system.CreateText("", ui_system.font.GetFont(ui::FontCollection::small).ToSDL(), colors::blue, 10.0F, 0.0F) } { }
void FPSFrame::Tick(u32 i, ui::UISystem& ui_system) {
    const std::string str = std::format("Tick {:6}", i);
    (void)TTF_SetTextString(ui_system[tick_text].text, str.c_str(), str.length());
}
UIDebuggerFrame::UIDebuggerFrame(ui::UISystem& ui_system) : tree { ui_system.text_engine, ui_system.font } {
    ui::Node::Handle frame = ui::NodeBuilder({ 400U, 600U }).Gap(50U).Fill(colors::gray).BuildRoot(tree, { 300U, 600U });
    ui::NodeBuilder(ui::fill, ui::hug).Text("Debug Frame").Fill(colors::light_gray).Build(tree, frame);
    ui::NodeBuilder(ui::hug).Text("Hej").Build(tree, frame);
}
void UIDebuggerFrame::Tick(ui::UISystem& ui_system) { }

MainMenuFrame::MainMenuFrame(ui::UISystem& ui_system) : tree { ui_system.text_engine, ui_system.font }, debug_tree { ui_system.text_engine, ui_system.font } {
    constexpr f32 title_y = 30.0F;
    (void)ui_system.CreateTextAligned("Hey Helene!", ui_system.font_bold.GetFont(ui::FontCollection::h1).ToSDL(), colors::light_sky_blue, 0, title_y, ui::TextAlign::center, ui_system.screen_width);

    if (false) {
        pce::Table main_menu = pce::Table("Main Menu");

        constexpr f32 menu_x = 200.0F;
        constexpr f32 list_start_y = 100.0F;
        pce::List<pce::String> buttons = { "Play", "Settings", "Exit" };
        ui_system.CreateText("Play", ui_system.font_bold.GetFont(ui::FontCollection::large).ToSDL(), colors::blue, menu_x, list_start_y);
        ui_system.CreateText("Settings", ui_system.font.GetFont(ui::FontCollection::large).ToSDL(), colors::black, menu_x, list_start_y + 50.0F);
        ui_system.CreateText("Exit", ui_system.font.GetFont(ui::FontCollection::large).ToSDL(), colors::red, menu_x, list_start_y + 100.0F);

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
    }

    ui::Node::Handle frame = ui::NodeBuilder(ui::hug).Gap(20U).Fill(colors::clear).BuildRoot(tree, { 100U, 200U }); {
        ui::Node::Handle root = ui::NodeBuilder(uint2 { 100U, 100U }).Padding({ 5U, 5U }).Fill(colors::forest_green).Build(tree, frame);
        ui::Node::Handle box1 = ui::NodeBuilder(ui::fill).Fill(colors::yellow).Padding({ 5U, 5U }).Build(tree, root);
        ui::Node::Handle box11 = ui::NodeBuilder(ui::fill).Fill(colors::blue).Build(tree, box1);
        ui::Node::Handle box12 = ui::NodeBuilder(ui::fill).Fill(colors::chocolate).Build(tree, box1);

        ui::Node::Handle box2 = ui::NodeBuilder(ui::fill).Fill(colors::red).Build(tree, root);
        ui::Node::Handle box3 = ui::NodeBuilder(ui::hug, ui::fill).Padding({ 10U, 10U }).Fill(colors::black).Build(tree, root);
    } {
        ui::Node::Handle root = ui::NodeBuilder(uint2 { 100U, 100U }).Padding({ 5U, 5U }).Fill(colors::green).Build(tree, frame);
        ui::Node::Handle box1 = ui::NodeBuilder(ui::fill).Fill(colors::yellow).Padding({ 5U, 5U }).Build(tree, root);
        ui::Node::Handle box2 = ui::NodeBuilder(ui::fill).Fill(colors::red).Build(tree, root);
    } {
        ui::Node::Handle root = ui::NodeBuilder(ui::hug).Padding({ 5U, 5U }).Fill(colors::cyan).Build(tree, frame);
        ui::Node::Handle box1 = ui::NodeBuilder(ui::hug).Fill(colors::yellow).Text(pce::String { "Play" }).Padding({ 10U, 0U }).Build(tree, root);
        ui::Node::Handle box2 = ui::NodeBuilder(ui::hug).Fill(colors::red).Text(pce::String { "Settings" }).Padding({ 10U, 0U }).Build(tree, root);
        ui::Node::Handle box3 = ui::NodeBuilder(ui::hug).Fill(colors::green).Text(pce::String { "Exit" }).Padding({ 10U, 0U }).Build(tree, root);
        ui::Node::Handle box4 = ui::NodeBuilder(ui::hug).Fill(colors::yellow).Text(pce::String { "Start Game" }).Padding({ 10U, 0U }).Padding({ 50U, 0U }).Build(tree, root);
    } {
        ui::Node::Handle root = ui::NodeBuilder(uint2 { 100U, 100U }).Padding({ 5U, 5U }).Fill(colors::sea_green).Build(tree, frame);
        ui::Node::Handle box1 = ui::NodeBuilder(ui::fill).Fill(colors::yellow).Padding({ 5U, 5U }).Build(tree, root);
        ui::Node::Handle box2 = ui::NodeBuilder(ui::fill).Fill(colors::red).Build(tree, root);
        ui::Node::Handle box3 = ui::NodeBuilder(ui::hug).Padding({ 10U, 10U }).Fill(colors::black).Build(tree, root);
    } {
        constexpr u32 width = 100U;
        ui::Node::Handle root = ui::NodeBuilder(ui::hug, 400U).Padding({ 5U, 5U }).Fill(colors::forest_green).Build(tree, frame);
        ui::Node::Handle box1 = ui::NodeBuilder(width, ui::fill).Fill(colors::yellow).Padding({ 5U, 5U }).Build(tree, root);
        ui::Node::Handle box2 = ui::NodeBuilder(width * 2, ui::fill).Fill(colors::red).Build(tree, root);
        ui::Node::Handle box3 = ui::NodeBuilder(width * 3, ui::fill).Padding({ 4U, 4U }).Gap(2U).Fill(colors::black).Build(tree, root);
        ui::Node::Handle box31 = ui::NodeBuilder(ui::fill).Fill(colors::cyan).Build(tree, box3);
        ui::Node::Handle box32 = ui::NodeBuilder(ui::fill).Fill(colors::chocolate).Build(tree, box3);
        ui::Node::Handle box33 = ui::NodeBuilder(ui::fill).Fill(colors::yellow).Build(tree, box3);
    }
}
void MainMenuFrame::Tick(const u32 i, ui::UISystem& ui_system) {
    for (const ui::RectangleElement::Handle sqr : elements) { ui_system[sqr].color = SDL_Color { static_cast<u8>(i / 3U), static_cast<u8>(i / 10U), static_cast<u8>(i / 67U), 255 }; }
    if (ui_system.left_mouse_down) {
        DebugNode(debug_tree, tree, ui_system.hovered_node);
        debug_tree.MarkDirty();
    }
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
