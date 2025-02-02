#include "m_frame.hpp"

#include <ranges>
#include <g_components.hpp>
#include <stack>
#include <u_algorithm.hpp>

namespace pcg::frame {
namespace ui = pce::ui;
namespace colors = pce::colors;

MainMenuFrame::MainMenuFrame(ui::UISystem& ui_system) : tree { ui_system.text_engine, ui_system.font } {
    const pce::String title = "Hey Helene!";
    ui::Node::Handle frame = ui::NodeBuilder(ui_system.screen_size).Direction(ui::vertical).BuildRoot(tree, { 100U, 30U });
    ui::NodeBuilder(ui::hug).Text(title, ui::Fonts::title).Fill(colors::light_sky_blue).Build(tree, frame);
    ui::Node::Handle root = ui::NodeBuilder(ui::hug).Padding({ 5U, 5U }).Direction(ui::vertical).Center().Fill(colors::deep_purple).Build(tree, frame);
    ui::NodeBuilder(ui::hug).Fill(colors::radiant_orange).Text(pce::String { "Play" }, ui::Fonts::h1).Padding({ 10U, 0U }).Build(tree, root);
    ui::NodeBuilder(ui::hug).Fill(colors::cool_teal).Text(pce::String { "Settings" }, ui::Fonts::h1).Padding({ 10U, 0U }).Build(tree, root);
    ui::NodeBuilder(ui::hug).Fill(colors::ruby_red).Text(pce::String { "Exit" }, ui::Fonts::h1).Padding({ 10U, 0U }).Build(tree, root);
}
GameFrame::GameFrame(ui::UISystem& ui_system) : tree { ui_system.text_engine, ui_system.font } {
    const ui::Node::Handle frame = ui::NodeBuilder(ui_system.screen_size).Center().Direction(ui::vertical).BuildRoot(tree, { 0U, 30U });
    const ui::Node::Handle root = ui::NodeBuilder(ui::hug).Fill(colors::forest_green).Direction(ui::vertical).Center().Build(tree, frame);
    time_label = ui::NodeBuilder(ui::hug).Text("Time", ui::Fonts::h1).Build(tree, root);
    score_label = ui::NodeBuilder(ui::hug).Text("Score", ui::Fonts::h1).Build(tree, root);

    game_area = ui::NodeBuilder(ui::fill).Padding(uint2{ 300U, 100U}).Build(tree, frame);
    constexpr u32 box_size = 100U;
    box = ui::NodeBuilder(uint2 { box_size, box_size } ).Fill(colors::ruby_red).Build(tree, game_area.GetHandle());
}

ui::Node::Handle CreateDebugNodeComponent(const u32 layer, const pce::String& text, const SDL_Color color, ui::NodeTree& tree, const ui::Node::Handle frame) {
    constexpr u32 padding_offset = 10U;
    constexpr uint2 color_indicator_size { 10U, 20U };
    constexpr u32 gap_size { 2U };
    ui::Node::Handle component_handle = ui::NodeBuilder(ui::hug).Fill(colors::forest_green).Padding4(uint4 { padding_offset * layer, 0U, 0U, 0U }).Gap(gap_size).Build(tree, frame);
    ui::NodeBuilder(ui::hug).Text(text, ui::Fonts::body).Fill(colors::black).Build(tree, component_handle);
    ui::NodeBuilder(color_indicator_size).Fill(color).Build(tree, component_handle);
    return component_handle;
}
DebugFrame::DebugFrame(pce::ui::UISystem& ui_system) : tree { ui_system.text_engine, ui_system.font } { }
void DebugFrame::ShowElementStructure(const ui::UISystem::HoveredType& hovered) {
    using NodeHandleLayer = std::tuple<ui::Node::Handle, u32>;

    tree.Clear();
    if (!hovered.has_value() || &hovered.value().tree.get() == &tree) { return; }
    const ui::Node::Handle hovered_node = hovered.value().node_handle;
    const ui::NodeTree& hovered_tree = hovered.value().tree;
    pce::Stack<NodeHandleLayer> node_handles;
    node_handles.push(NodeHandleLayer { hovered_node, 0U });

    const ui::Node::Handle frame = ui::NodeBuilder(ui::hug).Fill(colors::clear).Fill(colors::white).Direction(ui::vertical).BuildRoot(tree, { 10U, 30U });
    while (!node_handles.empty()) {
        const auto [node_handle, layer] = node_handles.top();
        node_handles.pop();
        for (const ui::Node::Handle child_handle : hovered_tree.Children(node_handle)) { node_handles.push(NodeHandleLayer { child_handle, layer + 1 }); }
        const ui::Node& node = hovered_tree.GetNode(node_handle);
        CreateDebugNodeComponent(layer, node.IsText() ? "text" : "node", node.background_color, tree, frame);
    }
}
TestFrame::TestFrame(ui::UISystem& ui_system) : tree { ui_system.text_engine, ui_system.font } {
    ui::Node::Handle frame = ui::NodeBuilder(ui::hug).Gap(20U).Fill(colors::clear).BuildRoot(tree, { 100U, 400U }); {
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
        ui::Node::Handle root = ui::NodeBuilder(ui::hug).Padding({ 5U, 5U }).Direction(ui::vertical).Fill(colors::deep_purple).Build(tree, frame);
        ui::Node::Handle box1 = ui::NodeBuilder(ui::hug).Fill(colors::radiant_orange).Text(pce::String { "Play" }, ui::Fonts::h1).Padding({ 10U, 0U }).Build(tree, root);
        ui::Node::Handle box2 = ui::NodeBuilder(ui::hug).Fill(colors::cool_teal).Text(pce::String { "Settings" }, ui::Fonts::h1).Padding({ 10U, 0U }).Build(tree, root);
        ui::Node::Handle box3 = ui::NodeBuilder(ui::hug).Fill(colors::ruby_red).Text(pce::String { "Exit" }, ui::Fonts::h1).Padding({ 10U, 0U }).Build(tree, root);
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
    if (false) {
        pce::Table main_menu = pce::Table("Main Menu");

        constexpr f32 menu_x = 200.0F;
        constexpr f32 list_start_y = 100.0F;
        pce::List<pce::String> buttons = { "Play", "Settings", "Exit" };
        ui_system.CreateText("Play", ui_system.font_bold.GetFont(ui::Fonts::h2).ToSDL(), colors::blue, menu_x, list_start_y);
        ui_system.CreateText("Settings", ui_system.font.GetFont(ui::Fonts::h2).ToSDL(), colors::black, menu_x, list_start_y + 50.0F);
        ui_system.CreateText("Exit", ui_system.font.GetFont(ui::Fonts::h2).ToSDL(), colors::red, menu_x, list_start_y + 100.0F);

        constexpr SDL_FRect background_rect { .x = menu_x, .y = list_start_y, .w = 100.0F, .h = 300.0F };
        ui_system.CreateElement(background_rect, colors::gray, nullptr);

        main_menu.AddColumn("Fish Column", pce::List<pce::String> { "Hej", "Fem", "Femten" });
        ui_system.CreateTable(main_menu, colors::sea_green, ui_system.screen_size.x / 2.0F - 100.0F, 300.0F);

        SDL_FRect rect { .x = 200.0F, .y = 200.0F, .w = 30.0F, .h = 30.0F };

        (void)elements.emplace_back(ui_system.CreateElement(rect, colors::black, nullptr));

        rect.x += 200;
        (void)elements.emplace_back(ui_system.CreateElement(rect, colors::red, nullptr));

        rect.y = 400;
        ui_system.CreateList(colors::white, rect, 10U, 25.0F, ui::UISystem::ListDirection::vertical);
        rect.h *= 2;
        ui_system.CreateList(colors::green, rect, 10U, 25.0F, ui::UISystem::ListDirection::horizontal);
    }
}
void TestFrame::Tick(u32 tick, ui::UISystem& ui_system) { }

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
