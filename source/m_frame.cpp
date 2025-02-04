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
    const ui::Node::Handle component_handle = ui::NodeBuilder(ui::hug).Fill(colors::forest_green).Padding4(uint4 { padding_offset * layer, 0U, 0U, 0U }).Gap(gap_size).Build(tree, frame);
    ui::NodeBuilder(ui::hug).Text(text, ui::Fonts::body).Fill(colors::black).Build(tree, component_handle);
    ui::NodeBuilder(color_indicator_size).Fill(color).Build(tree, component_handle);
    return component_handle;
}
void InspectorFrame::ShowElementStructure(const ui::NodeRenderSystem::HoveredType& hovered) {
    using NodeHandleLayer = std::tuple<ui::Node::Handle, u32>;

    tree.Clear();
    if (!hovered.has_value() || &hovered.value().tree.get() == &tree) { return; }
    const ui::Node::Handle hovered_node = hovered.value().node_handle;
    const ui::NodeTree& hovered_tree = hovered.value().tree;
    Stack<NodeHandleLayer> node_handles;
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
TestFrame::TestFrame() {
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
        ui::Node::Handle box1 = ui::NodeBuilder(ui::hug).Fill(colors::radiant_orange).Text(String { "Play" }, ui::Fonts::h1).Padding({ 10U, 0U }).Build(tree, root);
        ui::Node::Handle box2 = ui::NodeBuilder(ui::hug).Fill(colors::cool_teal).Text(String { "Settings" }, ui::Fonts::h1).Padding({ 10U, 0U }).Build(tree, root);
        ui::Node::Handle box3 = ui::NodeBuilder(ui::hug).Fill(colors::ruby_red).Text(String { "Exit" }, ui::Fonts::h1).Padding({ 10U, 0U }).Build(tree, root);
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
}
