#include "t_debug_system.hpp"

#include <stack>

#include "g_components.hpp"
#include "orchestra.hpp"

#include "engine/u_types.hpp"

namespace pce::ui {
namespace colors = colors;
Handle<Node> CreateDebugNodeComponent(const u32 layer, const String& text, const SDL_Color color, Handle<NodeTree> tree, const Handle<Node> frame) {
    constexpr u32 padding_offset = 10U;
    constexpr uint2 color_indicator_size { 10U, 20U };
    constexpr u32 gap_size { 2U };
    const Handle<Node> component_handle = B(tree, frame, hug).Fill(colors::forest_green).Padding4(uint4 { padding_offset * layer, 0U, 0U, 0U }).Gap(gap_size).Build();
    B(tree, component_handle, hug).Text(text, FontSizes::body).Fill(colors::black).Build();
    B(tree, component_handle, color_indicator_size).Fill(color).Build();
    return component_handle;
}
void TickFrame::DisplayInfo() const {
    using namespace ui;
    u32 tick = TickSystem::tick_table.tick.Value();
    u32 fps = 1.0F / TickSystem::tick_table.delta_time;
    NodeRenderSystem::node_trees[tree_handle].Clear();
    Handle<Node> frame = B(tree_handle, hug, { 10U, 0U }).Direction(vertical).Build();
    Handle<Node> ticks = B(tree_handle, frame, hug).Text(std::format("Tick: {:>8}   |   TPS: {:>4}   |   FPS: {:>4}", tick, fps, fps), FontSizes::tiny).Fill(colors::radiant_orange).Build();
    Handle<Node> systems = B(tree_handle, frame, hug).Direction(vertical).Gap(10).Build();
    for (const auto [name, ns] : std::views::zip(Orchestra::orchestra_table.names, Orchestra::orchestra_table.nano_seconds)) {
        constexpr f32 THOUSANDTH = 0.001F;
        (void)B(tree_handle, systems, fill).Fill(colors::radiant_orange).Text(std::format("{:.3f}ms | {}", ns * THOUSANDTH * THOUSANDTH, name), FontSizes::tiny).Build();
    }
    NodeRenderSystem::node_trees[tree_handle].MarkDirty();
}
void InspectorFrame::ShowElementStructure(const HoveredType hovered) {
    using NodeHandleLayer = std::tuple<Handle<Node>, u32>;
    if (!hovered.has_value() || hovered->tree_handle.id == tree_handle.id) { return; }
    NodeRenderSystem::node_trees[tree_handle].Clear();
    Stack<NodeHandleLayer> node_handles;
    node_handles.push(NodeHandleLayer { hovered->node_handle, 0U });

    const Handle<Node> frame = B(tree_handle, hug, { 10U, 30U }).Fill(colors::clear).Fill(colors::white).Direction(vertical).Build();
    const NodeTree& hovered_tree = NodeRenderSystem::node_trees[hovered->tree_handle];
    while (!node_handles.empty()) {
        const auto [node_handle, layer] = node_handles.top();
        node_handles.pop();
        for (const Handle child_handle : hovered_tree.children[node_handle]) { node_handles.push(NodeHandleLayer { child_handle, layer + 1 }); }
        const NodeStyle& node = hovered_tree.node_styles[node_handle];
        const NodeProperties& node_properties = hovered_tree.node_properties[node_handle];
        CreateDebugNodeComponent(layer, node_properties.text.Empty() ? "node" : std::format("text [{}, {}]", node.position.x, node.position.y), node.background_color, tree_handle, frame);
    }
}
TestFrame::TestFrame() {
    Handle<Node> frame = B(tree_handle, hug, { 100U, 400U }).Gap(20U).Fill(colors::clear).Build(); {
        Handle<Node> root = B(tree_handle, frame, uint2 { 100U, 100U }).Padding2({ 5U, 5U }).Fill(colors::forest_green).Build();
        Handle<Node> box1 = B(tree_handle, root, fill).Fill(colors::yellow).Padding2({ 5U, 5U }).Build();
        Handle<Node> box11 = B(tree_handle, box1, fill).Fill(colors::blue).Build();
        Handle<Node> box12 = B(tree_handle, box1, fill).Fill(colors::chocolate).Build();

        Handle<Node> box2 = B(tree_handle, root, fill).Fill(colors::red).Build();
        Handle<Node> box3 = B(tree_handle, root, { hug, fill }).Padding2({ 10U, 10U }).Fill(colors::black).Build();
    } {
        Handle<Node> root = B(tree_handle, frame, uint2 { 100U, 100U }).Padding2({ 5U, 5U }).Fill(colors::green).Build();
        Handle<Node> box1 = B(tree_handle, root, fill).Fill(colors::yellow).Padding2({ 5U, 5U }).Build();
        Handle<Node> box2 = B(tree_handle, root, fill).Fill(colors::red).Build();
    } {
        Handle<Node> root = B(tree_handle, frame, hug).Padding2({ 5U, 5U }).Direction(vertical).Fill(colors::deep_purple).Build();
        Handle<Node> box1 = B(tree_handle, root, hug).Fill(colors::radiant_orange).Text(String { "Play" }, FontSizes::h1).Padding2({ 10U, 0U }).Build();
        Handle<Node> box2 = B(tree_handle, root, hug).Fill(colors::cool_teal).Text(String { "Settings" }, FontSizes::h1).Padding2({ 10U, 0U }).Build();
        Handle<Node> box3 = B(tree_handle, root, hug).Fill(colors::ruby_red).Text(String { "Exit" }, FontSizes::h1).Padding2({ 10U, 0U }).Build();
    } {
        Handle<Node> root = B(tree_handle, frame, uint2 { 100U, 100U }).Padding2({ 5U, 5U }).Fill(colors::sea_green).Build();
        Handle<Node> box1 = B(tree_handle, root, fill).Fill(colors::yellow).Padding2({ 5U, 5U }).Build();
        Handle<Node> box2 = B(tree_handle, root, fill).Fill(colors::red).Build();
        Handle<Node> box3 = B(tree_handle, root, hug).Padding2({ 10U, 10U }).Fill(colors::black).Build();
    } {
        constexpr u32 width = 100U;
        Handle<Node> root = B(tree_handle, frame, {hug, 400U}).Padding2({ 5U, 5U }).Fill(colors::forest_green).Build();
        Handle<Node> box1 = B(tree_handle, root, {width, fill}).Fill(colors::yellow).Padding2({ 5U, 5U }).Build();
        Handle<Node> box2 = B(tree_handle, root, {width * 2, fill}).Fill(colors::red).Build();
        Handle<Node> box3 = B(tree_handle, root,  {width * 3, fill}).Padding2({ 4U, 4U }).Gap(2U).Fill(colors::black).Build();
        Handle<Node> box31 = B(tree_handle, box3, fill).Fill(colors::cyan).Build();
        Handle<Node> box32 = B(tree_handle, box3, fill).Fill(colors::chocolate).Build();
        Handle<Node> box33 = B(tree_handle, box3, fill).Fill(colors::yellow).Build();
    }
}
void DebugSystem::operator()() {
    using namespace ui;
    if (InputSystem::input_table.left_mouse_down) {
        NodeRenderSystem::node_trees[inspector_frame.tree_handle].MarkDirty();
        if (InputSystem::input_table.keys[SDLK_LALT]) { inspector_frame.ShowElementStructure(NodeInputSystem::hovered); } else { NodeRenderSystem::node_trees[inspector_frame.tree_handle].Clear(); }
    }
    NodeRenderSystem::node_trees[tick_frame.tree_handle].MarkDirty();
    if (TickSystem::tick_table.tick.Value() % 100 == 0) { tick_frame.DisplayInfo(); }
}
}
