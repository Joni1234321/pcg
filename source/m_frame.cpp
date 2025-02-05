#include "m_frame.hpp"

#include <ranges>
#include <stack>

#include "g_components.hpp"

namespace pce::frame {
namespace ui = ui;
namespace colors = colors;

ui::Node::Handle CreateDebugNodeComponent(const u32 layer, const String& text, const SDL_Color color, ui::NodeTree& tree, const ui::Node::Handle frame) {
    constexpr u32 padding_offset = 10U;
    constexpr uint2 color_indicator_size { 10U, 20U };
    constexpr u32 gap_size { 2U };
    const ui::Node::Handle component_handle = ui::B(tree, frame, ui::hug).Fill(colors::forest_green).Padding4(uint4 { padding_offset * layer, 0U, 0U, 0U }).Gap(gap_size).Build();
    ui::B(tree, component_handle, ui::hug).Text(text, ui::Fonts::body).Fill(colors::black).Build();
    ui::B(tree, component_handle, color_indicator_size).Fill(color).Build();
    return component_handle;
}
void InspectorFrame::ShowElementStructure(const ui::HoveredType& hovered) {
    using NodeHandleLayer = std::tuple<ui::Node::Handle, u32>;

    tree.Clear();
    if (!hovered.has_value() || &hovered.value().tree.get() == &tree) { return; }
    const ui::Node::Handle hovered_node = hovered.value().node_handle;
    const ui::NodeTree& hovered_tree = hovered.value().tree;
    Stack<NodeHandleLayer> node_handles;
    node_handles.push(NodeHandleLayer { hovered_node, 0U });

    const ui::Node::Handle frame = ui::B(tree, ui::hug, { 10U, 30U }).Fill(colors::clear).Fill(colors::white).Direction(ui::vertical).Build();
    while (!node_handles.empty()) {
        const auto [node_handle, layer] = node_handles.top();
        node_handles.pop();
        for (const ui::Node::Handle child_handle : hovered_tree.Children(node_handle)) { node_handles.push(NodeHandleLayer { child_handle, layer + 1 }); }
        const ui::Node& node = hovered_tree.GetNode(node_handle);
        CreateDebugNodeComponent(layer, node.IsText() ? std::format("text [{}, {}]", node.position.x, node.position.y) : "node", node.background_color, tree, frame);
    }
}
TestFrame::TestFrame() {
    ui::Node::Handle frame = ui::B(tree, ui::hug, { 100U, 400U }).Gap(20U).Fill(colors::clear).Build(); {
        ui::Node::Handle root = ui::B(tree, frame, uint2 { 100U, 100U }).Padding2({ 5U, 5U }).Fill(colors::forest_green).Build();
        ui::Node::Handle box1 = ui::B(tree, root, ui::fill).Fill(colors::yellow).Padding2({ 5U, 5U }).Build();
        ui::Node::Handle box11 = ui::B(tree, box1, ui::fill).Fill(colors::blue).Build();
        ui::Node::Handle box12 = ui::B(tree, box1, ui::fill).Fill(colors::chocolate).Build();

        ui::Node::Handle box2 = ui::B(tree, root, ui::fill).Fill(colors::red).Build();
        ui::Node::Handle box3 = ui::B(tree, root, { ui::hug, ui::fill }).Padding2({ 10U, 10U }).Fill(colors::black).Build();
    } {
        ui::Node::Handle root = ui::B(tree, frame, uint2 { 100U, 100U }).Padding2({ 5U, 5U }).Fill(colors::green).Build();
        ui::Node::Handle box1 = ui::B(tree, root, ui::fill).Fill(colors::yellow).Padding2({ 5U, 5U }).Build();
        ui::Node::Handle box2 = ui::B(tree, root, ui::fill).Fill(colors::red).Build();
    } {
        ui::Node::Handle root = ui::B(tree, frame, ui::hug).Padding2({ 5U, 5U }).Direction(ui::vertical).Fill(colors::deep_purple).Build();
        ui::Node::Handle box1 = ui::B(tree, root, ui::hug).Fill(colors::radiant_orange).Text(String { "Play" }, ui::Fonts::h1).Padding2({ 10U, 0U }).Build();
        ui::Node::Handle box2 = ui::B(tree, root, ui::hug).Fill(colors::cool_teal).Text(String { "Settings" }, ui::Fonts::h1).Padding2({ 10U, 0U }).Build();
        ui::Node::Handle box3 = ui::B(tree, root, ui::hug).Fill(colors::ruby_red).Text(String { "Exit" }, ui::Fonts::h1).Padding2({ 10U, 0U }).Build();
    } {
        ui::Node::Handle root = ui::B(tree, frame, uint2 { 100U, 100U }).Padding2({ 5U, 5U }).Fill(colors::sea_green).Build();
        ui::Node::Handle box1 = ui::B(tree, root, ui::fill).Fill(colors::yellow).Padding2({ 5U, 5U }).Build();
        ui::Node::Handle box2 = ui::B(tree, root, ui::fill).Fill(colors::red).Build();
        ui::Node::Handle box3 = ui::B(tree, root, ui::hug).Padding2({ 10U, 10U }).Fill(colors::black).Build();
    } {
        constexpr u32 width = 100U;
        ui::Node::Handle root = ui::B(tree, frame, {ui::hug, 400U}).Padding2({ 5U, 5U }).Fill(colors::forest_green).Build();
        ui::Node::Handle box1 = ui::B(tree, root, {width, ui::fill}).Fill(colors::yellow).Padding2({ 5U, 5U }).Build();
        ui::Node::Handle box2 = ui::B(tree, root, {width * 2, ui::fill}).Fill(colors::red).Build();
        ui::Node::Handle box3 = ui::B(tree, root,  {width * 3, ui::fill}).Padding2({ 4U, 4U }).Gap(2U).Fill(colors::black).Build();
        ui::Node::Handle box31 = ui::B(tree, box3, ui::fill).Fill(colors::cyan).Build();
        ui::Node::Handle box32 = ui::B(tree, box3, ui::fill).Fill(colors::chocolate).Build();
        ui::Node::Handle box33 = ui::B(tree, box3, ui::fill).Fill(colors::yellow).Build();
    }

}
}
