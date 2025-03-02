#include "t_debug_system.hpp"

#include <stack>

#include "g_components.hpp"

#include "0_engine/u_colors.hpp"
#include "0_engine/u_types.hpp"

#include "1_systems/i_input_system.hpp"
#include "1_systems/u_orchestra.hpp"

namespace pce::ui {
namespace colors = colors;
TickComponent::TickComponent(const NodeReference parent) : root { .tree = parent.tree, .node = B(parent.tree, parent.node, fill).Text(FontSizes::tiny, colors::radiant_orange).Build() } { }
void TickComponent::SetProperty(const Property& property) const {
    static constexpr f32 THOUSANDTH = 0.001F;
    const auto& [name, ns] = property;
    data[root.tree].node_properties[root.node].text = std::format("{:.3f}ms | {}", ns * THOUSANDTH * THOUSANDTH, name);
}
static constexpr u32 GAP_SIZE { 2U };
static constexpr uint2 COLOR_INDICATOR_SIZE { 10U, 20U };
DebugNodeComponent::DebugNodeComponent(const NodeReference parent): root { parent.tree, B(parent.tree, parent.node, hug).Fill(colors::forest_green).Gap(GAP_SIZE).Build() },
                                                                    text { B(root.tree, root.node, hug).Text(FontSizes::body, colors::black).Build() },
                                                                    color_indicator { B(root.tree, root.node, COLOR_INDICATOR_SIZE).Build() } { }
void DebugNodeComponent::SetProperty(const Property& property) const {
    constexpr u32 padding_offset = 10U;
    const NodeStyle& style = data[property.hovered.tree].styles[property.hovered.node];
    const NodeProperties& properties = data[property.hovered.tree].node_properties[property.hovered.node];
    const String type = !properties.text.Empty() ? "text" : style.texture.IsValid() ? "image" : "node";

    data[root.tree].styles[root.node].padding.x = padding_offset * property.layer;
    data[root.tree].node_properties[text].text = std::format("{} [{}, {}]", type, style.position.x, style.position.y);
    data[root.tree].styles[color_indicator].background_color = style.background_color;
}
void TickFrame::DisplayInfo() {
    u32 tick = singleton.Get<TickState>().tick.Value();
    u32 fps = static_cast<u32>(1.0F / singleton.Get<TickState>().delta_time);
    data[tree].MarkDirty();
    data[tree].node_properties[ticks].text = std::format("Tick: {:>8}   |   TPS: {:>4}   |   FPS: {:>4}", tick, fps, fps);

    systems.Set(std::views::zip(singleton.Get<OrchestraState>().names, singleton.Get<OrchestraState>().nano_seconds));
}
void InspectorFrame::ShowElementStructure(const HoveredType hovered) {
    if (!hovered.has_value() || hovered->tree.id == tree.id) { return; }
    data[tree].node_properties[hovered_label].text = std::format("[{} | {}]", hovered->tree.id, hovered->node.id);
    List properties { DebugNodeComponent::Property { .hovered = hovered.value(), .layer = 0U } };
    for (u32 i = 0U; i < properties.Size(); ++i) {
        const auto [hovered, layer] = properties.Back();
        for (const Handle child : data[hovered.tree].children[hovered.node]) { properties.EmplaceBack(DebugNodeComponent::Property { .hovered = { .tree = hovered.tree, .node = child }, .layer = layer + 1U }); }
    }
    nodes.Set(properties);
}
TestFrame::TestFrame() {
    Handle<Node> core_root = B(frame).Node(100U, 400U).Gap(20U).Fill(colors::clear).Build(); {
        Handle<Node> root = B(core_root).Node(100U, 100U).Padding2({ 5U, 5U }).Fill(colors::forest_green).Build();
        Handle<Node> box1 = B(root).Node(fill).Fill(colors::yellow).Padding2({ 5U, 5U }).Build();
        Handle<Node> box11 = B(box1).Node(fill).Fill(colors::blue).Build();
        Handle<Node> box12 = B(box1).Node(fill).Fill(colors::chocolate).Build();

        Handle<Node> box2 = B(root).Node(fill).Fill(colors::red).Build();
        Handle<Node> box3 = B(root).Node(hug, fill).Padding2({ 10U, 10U }).Fill(colors::black).Build();
    } {
        Handle<Node> root = B(core_root).Node(100U, 100U).Padding2({ 5U, 5U }).Fill(colors::green).Build();
        Handle<Node> box1 = B(root).Node(fill).Fill(colors::yellow).Padding2({ 5U, 5U }).Build();
        Handle<Node> box2 = B(root).Node(fill).Fill(colors::red).Build();
    } {
        Handle<Node> root = B(core_root).Node(hug).Padding2({ 5U, 5U }).Direction(vertical).Fill(colors::deep_purple).Build();
        Handle<Node> box1 = B(root).Node(hug).Text("Play", FontSizes::h1, colors::radiant_orange).Padding2({ 10U, 0U }).Build();
        Handle<Node> box2 = B(root).Node(hug).Text("Settings", FontSizes::h1, colors::cool_teal).Padding2({ 10U, 0U }).Build();
        Handle<Node> box3 = B(root).Node(hug).Text("Exit", FontSizes::h1, colors::ruby_red).Padding2({ 10U, 0U }).Build();
    } {
        Handle<Node> root = B(core_root).Node(100U, 100U).Padding2({ 5U, 5U }).Fill(colors::sea_green).Build();
        Handle<Node> box1 = B(root).Node(fill).Fill(colors::yellow).Padding2({ 5U, 5U }).Build();
        Handle<Node> box2 = B(root).Node(fill).Fill(colors::red).Build();
        Handle<Node> box3 = B(root).Node(hug).Padding2({ 10U, 10U }).Fill(colors::black).Build();
    } {
        constexpr u32 width = 100U;
        Handle<Node> root = B(core_root).Node(hug, 400U).Padding2({ 5U, 5U }).Fill(colors::forest_green).Build();
        Handle<Node> box1 = B(root).Node(width, fill).Fill(colors::yellow).Padding2({ 5U, 5U }).Build();
        Handle<Node> box2 = B(root).Node(width * 2, fill).Fill(colors::red).Build();
        Handle<Node> box3 = B(root).Node(width * 3, fill).Padding2({ 4U, 4U }).Gap(2U).Fill(colors::black).Build();
        Handle<Node> box31 = B(box3).Node(fill).Fill(colors::cyan).Build();
        Handle<Node> box32 = B(box3).Node(fill).Fill(colors::chocolate).Build();
        Handle<Node> box33 = B(box3).Node(fill).Fill(colors::yellow).Build();
    }
}
void DebugSystem::operator()() {
    InputState& input_state = singleton.Get<InputState>();
    data[inspector_frame.tree].MarkDirty();
    if (input_state.left_mouse_down) { if (input_state.keys[SDLK_LALT]) { inspector_frame.ShowElementStructure(singleton.Get<HoveredType>()); } else { inspector_frame.nodes.Hide(); } }
    if (input_state.keys[SDLK_LALT]) {
        u32 key_count = std::min(data.Get<NodeTree>().Size(), 10U);
        auto rng = std::views::iota(0U, key_count);
        auto it = std::ranges::find_if(rng, [&] (const u32 i) { return input_state.keys_down[SDLK_0 + i]; });
        if (it != std::end(rng)) {
            const Handle<NodeTree> tree { *it };
            inspector_frame.ShowElementStructure(NodeReference { .tree = tree, .node = data[tree].Root() });
        }
    }
    data[tick_frame.tree].MarkDirty();
    if (singleton.Get<TickState>().tick.Value() % 500U == 0U) { tick_frame.DisplayInfo(); }
}
}
