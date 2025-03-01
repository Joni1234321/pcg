#include "t_debug_system.hpp"

#include <stack>

#include "g_components.hpp"

#include "0_engine/u_colors.hpp"
#include "0_engine/u_types.hpp"

#include "1_systems/i_input_system.hpp"
#include "1_systems/u_orchestra.hpp"

namespace pce::ui {
namespace colors = colors;
Handle<Node> CreateDebugNodeComponent(const u32 layer, const String& text, const SDL_Color color, const Handle<NodeTree> tree, const Handle<Node> frame) {
    constexpr u32 padding_offset = 10U;
    constexpr uint2 color_indicator_size { 10U, 20U };
    constexpr u32 gap_size { 2U };
    const Handle<Node> component_handle = B(tree, frame, hug).Fill(colors::forest_green).Padding4(uint4 { padding_offset * layer, 0U, 0U, 0U }).Gap(gap_size).Build();
    (void)B(tree, component_handle, hug).Text(text, FontSizes::body).Fill(colors::black).Build();
    (void)B(tree, component_handle, color_indicator_size).Fill(color).Build();
    return component_handle;
}
TickFrame::TickFrame() {
    Handle<Node> frame = B(tree, hug, { 10U, 0U }).Direction(vertical).Build();
    ticks = B(tree, frame, hug).Text("", FontSizes::tiny).Fill(colors::radiant_orange).Build();
    systems = B(tree, frame, hug).Direction(vertical).Gap(10).Build();

    systems_pool.SetPrefab(B(tree, systems.GetHandle(), fill).Fill(colors::radiant_orange).FontSize(FontSizes::tiny).Build());
}
void TickFrame::DisplayInfo() {
    using namespace ui;
    u32 tick = singleton.Get<TickState>().tick.Value();
    u32 fps = 1.0F / singleton.Get<TickState>().delta_time;
    data[tree].node_properties[ticks.GetHandle()].text = std::format("Tick: {:>8}   |   TPS: {:>4}   |   FPS: {:>4}", tick, fps, fps);    data[tree].node_properties[ticks.GetHandle()].text = std::format("Tick: {:>8}   |   TPS: {:>4}   |   FPS: {:>4}", tick, fps, fps);

    constexpr f32 THOUSANDTH = 0.001F;
    auto values = std::views::zip(singleton.Get<OrchestraState>().names, singleton.Get<OrchestraState>().nano_seconds);
    auto pred = [](const Handle<NodeTree> tree, const Handle<Node> node, const std::tuple<String&, u32> tuple) {
        const auto& [name, ns] = tuple;
        data[tree].node_properties[node].text =  std::format("{:.3f}ms | {}", ns * THOUSANDTH * THOUSANDTH, name);
    };
    systems_pool.Set(values, pred);

    data[tree].MarkDirty();
}
void InspectorFrame::ShowElementStructure(const HoveredType hovered) const {
    using NodeHandleLayer = std::tuple<Handle<Node>, u32>;
    if (!hovered.has_value() || hovered->tree.id == tree.id) { return; }
    data[tree].Clear();
    Stack<NodeHandleLayer> node_handles;
    node_handles.push(NodeHandleLayer { hovered->node, 0U });

    const Handle<Node> frame = B(tree, hug, { 10U, 30U }).Fill(colors::clear).Fill(colors::white).Direction(vertical).Build();
    const NodeTree& hovered_tree = data[hovered->tree];
    while (!node_handles.empty()) {
        const auto [node_handle, layer] = node_handles.top();
        node_handles.pop();
        for (const Handle child_handle : hovered_tree.children[node_handle]) { node_handles.push(NodeHandleLayer { child_handle, layer + 1 }); }
        const NodeStyle& style = hovered_tree.styles[node_handle];
        const NodeProperties& node_properties = hovered_tree.node_properties[node_handle];
        const String type = !node_properties.text.Empty() ? "text" : style.texture.IsValid() ? "image" : "node";
        CreateDebugNodeComponent(layer, std::format("{} [{}, {}]", type, style.position.x, style.position.y), style.background_color, tree, frame);
    }
}
TestFrame::TestFrame() {
    Handle<Node> frame = B(tree, hug, { 100U, 400U }).Gap(20U).Fill(colors::clear).Build(); {
        Handle<Node> root = B(tree, frame, uint2 { 100U, 100U }).Padding2({ 5U, 5U }).Fill(colors::forest_green).Build();
        Handle<Node> box1 = B(tree, root, fill).Fill(colors::yellow).Padding2({ 5U, 5U }).Build();
        Handle<Node> box11 = B(tree, box1, fill).Fill(colors::blue).Build();
        Handle<Node> box12 = B(tree, box1, fill).Fill(colors::chocolate).Build();

        Handle<Node> box2 = B(tree, root, fill).Fill(colors::red).Build();
        Handle<Node> box3 = B(tree, root, { hug, fill }).Padding2({ 10U, 10U }).Fill(colors::black).Build();
    } {
        Handle<Node> root = B(tree, frame, uint2 { 100U, 100U }).Padding2({ 5U, 5U }).Fill(colors::green).Build();
        Handle<Node> box1 = B(tree, root, fill).Fill(colors::yellow).Padding2({ 5U, 5U }).Build();
        Handle<Node> box2 = B(tree, root, fill).Fill(colors::red).Build();
    } {
        Handle<Node> root = B(tree, frame, hug).Padding2({ 5U, 5U }).Direction(vertical).Fill(colors::deep_purple).Build();
        Handle<Node> box1 = B(tree, root, hug).Fill(colors::radiant_orange).Text(String { "Play" }, FontSizes::h1).Padding2({ 10U, 0U }).Build();
        Handle<Node> box2 = B(tree, root, hug).Fill(colors::cool_teal).Text(String { "Settings" }, FontSizes::h1).Padding2({ 10U, 0U }).Build();
        Handle<Node> box3 = B(tree, root, hug).Fill(colors::ruby_red).Text(String { "Exit" }, FontSizes::h1).Padding2({ 10U, 0U }).Build();
    } {
        Handle<Node> root = B(tree, frame, uint2 { 100U, 100U }).Padding2({ 5U, 5U }).Fill(colors::sea_green).Build();
        Handle<Node> box1 = B(tree, root, fill).Fill(colors::yellow).Padding2({ 5U, 5U }).Build();
        Handle<Node> box2 = B(tree, root, fill).Fill(colors::red).Build();
        Handle<Node> box3 = B(tree, root, hug).Padding2({ 10U, 10U }).Fill(colors::black).Build();
    } {
        constexpr u32 width = 100U;
        Handle<Node> root = B(tree, frame, {hug, 400U}).Padding2({ 5U, 5U }).Fill(colors::forest_green).Build();
        Handle<Node> box1 = B(tree, root, {width, fill}).Fill(colors::yellow).Padding2({ 5U, 5U }).Build();
        Handle<Node> box2 = B(tree, root, {width * 2, fill}).Fill(colors::red).Build();
        Handle<Node> box3 = B(tree, root,  {width * 3, fill}).Padding2({ 4U, 4U }).Gap(2U).Fill(colors::black).Build();
        Handle<Node> box31 = B(tree, box3, fill).Fill(colors::cyan).Build();
        Handle<Node> box32 = B(tree, box3, fill).Fill(colors::chocolate).Build();
        Handle<Node> box33 = B(tree, box3, fill).Fill(colors::yellow).Build();
    }
}
void DebugSystem::operator()() {
    using namespace ui;
    InputState& input_state = singleton.Get<InputState>();
    if (input_state.left_mouse_down) {
        data[inspector_frame.tree].MarkDirty();
        if (input_state.keys[SDLK_LALT]) { inspector_frame.ShowElementStructure(singleton.Get<HoveredType>()); } else { data[inspector_frame.tree].Clear(); }
    }
    data[tick_frame.tree].MarkDirty();
    if (singleton.Get<TickState>().tick.Value() % 100 == 0) { tick_frame.DisplayInfo(); }
}
}
