#include "r_ui.hpp"

#include <ranges>
#include <stack>

#include "g_components.hpp"

namespace pce::ui {
namespace ui = ui;
namespace colors = colors;

NodeHandle CreateDebugNodeComponent(const u32 layer, const String& text, const SDL_Color color, NodeTree& tree, const NodeHandle frame) {
    constexpr u32 padding_offset = 10U;
    constexpr uint2 color_indicator_size { 10U, 20U };
    constexpr u32 gap_size { 2U };
    const NodeHandle component_handle = B(tree, frame, hug).Fill(colors::forest_green).Padding4(uint4 { padding_offset * layer, 0U, 0U, 0U }).Gap(gap_size).Build();
    B(tree, component_handle, hug).Text(text, FontSizes::body).Fill(colors::black).Build();
    B(tree, component_handle, color_indicator_size).Fill(color).Build();
    return component_handle;
}
void InspectorFrame::ShowElementStructure(const HoveredType& hovered) {
    using NodeHandleLayer = std::tuple<NodeHandle, u32>;

    tree.Clear();
    if (!hovered.has_value() || &hovered.value().tree.get() == &tree) { return; }
    const NodeHandle hovered_node = hovered.value().node_handle;
    const NodeTree& hovered_tree = hovered.value().tree;
    Stack<NodeHandleLayer> node_handles;
    node_handles.push(NodeHandleLayer { hovered_node, 0U });

    const NodeHandle frame = B(tree, hug, { 10U, 30U }).Fill(colors::clear).Fill(colors::white).Direction(vertical).Build();
    while (!node_handles.empty()) {
        const auto [node_handle, layer] = node_handles.top();
        node_handles.pop();
        for (const NodeHandle child_handle : hovered_tree.Children(node_handle)) { node_handles.push(NodeHandleLayer { child_handle, layer + 1 }); }
        const NodeStyle& node = hovered_tree.GetStyle(node_handle);
        const NodeProperties& node_properties = hovered_tree.GetProperties(node_handle);
        CreateDebugNodeComponent(layer, node_properties.text.Empty() ? "node" : std::format("text [{}, {}]", node.position.x, node.position.y), node.background_color, tree, frame);
    }
}
TestFrame::TestFrame() : tree { NodeRenderSystem::node_trees.EmplaceBack() } {
    NodeHandle frame = B(tree, hug, { 100U, 400U }).Gap(20U).Fill(colors::clear).Build(); {
        NodeHandle root = B(tree, frame, uint2 { 100U, 100U }).Padding2({ 5U, 5U }).Fill(colors::forest_green).Build();
        NodeHandle box1 = B(tree, root, fill).Fill(colors::yellow).Padding2({ 5U, 5U }).Build();
        NodeHandle box11 = B(tree, box1, fill).Fill(colors::blue).Build();
        NodeHandle box12 = B(tree, box1, fill).Fill(colors::chocolate).Build();

        NodeHandle box2 = B(tree, root, fill).Fill(colors::red).Build();
        NodeHandle box3 = B(tree, root, { hug, fill }).Padding2({ 10U, 10U }).Fill(colors::black).Build();
    } {
        NodeHandle root = B(tree, frame, uint2 { 100U, 100U }).Padding2({ 5U, 5U }).Fill(colors::green).Build();
        NodeHandle box1 = B(tree, root, fill).Fill(colors::yellow).Padding2({ 5U, 5U }).Build();
        NodeHandle box2 = B(tree, root, fill).Fill(colors::red).Build();
    } {
        NodeHandle root = B(tree, frame, hug).Padding2({ 5U, 5U }).Direction(vertical).Fill(colors::deep_purple).Build();
        NodeHandle box1 = B(tree, root, hug).Fill(colors::radiant_orange).Text(String { "Play" }, FontSizes::h1).Padding2({ 10U, 0U }).Build();
        NodeHandle box2 = B(tree, root, hug).Fill(colors::cool_teal).Text(String { "Settings" }, FontSizes::h1).Padding2({ 10U, 0U }).Build();
        NodeHandle box3 = B(tree, root, hug).Fill(colors::ruby_red).Text(String { "Exit" }, FontSizes::h1).Padding2({ 10U, 0U }).Build();
    } {
        NodeHandle root = B(tree, frame, uint2 { 100U, 100U }).Padding2({ 5U, 5U }).Fill(colors::sea_green).Build();
        NodeHandle box1 = B(tree, root, fill).Fill(colors::yellow).Padding2({ 5U, 5U }).Build();
        NodeHandle box2 = B(tree, root, fill).Fill(colors::red).Build();
        NodeHandle box3 = B(tree, root, hug).Padding2({ 10U, 10U }).Fill(colors::black).Build();
    } {
        constexpr u32 width = 100U;
        NodeHandle root = B(tree, frame, {hug, 400U}).Padding2({ 5U, 5U }).Fill(colors::forest_green).Build();
        NodeHandle box1 = B(tree, root, {width, fill}).Fill(colors::yellow).Padding2({ 5U, 5U }).Build();
        NodeHandle box2 = B(tree, root, {width * 2, fill}).Fill(colors::red).Build();
        NodeHandle box3 = B(tree, root,  {width * 3, fill}).Padding2({ 4U, 4U }).Gap(2U).Fill(colors::black).Build();
        NodeHandle box31 = B(tree, box3, fill).Fill(colors::cyan).Build();
        NodeHandle box32 = B(tree, box3, fill).Fill(colors::chocolate).Build();
        NodeHandle box33 = B(tree, box3, fill).Fill(colors::yellow).Build();
    }

}
}
