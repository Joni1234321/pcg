#include "t_debug_system.hpp"

#include <stack>

#include "g_components.hpp"

#include "0_engine/u_colors.hpp"
#include "0_engine/u_types.hpp"

#include "1_systems/i_input_system.hpp"
#include "1_systems/u_orchestra.hpp"

namespace pce::ui {
namespace colors = colors;
TickFrame::TickFrame() {
    Handle<Node> frame = B(tree, hug, { 10U, 0U }).Direction(vertical).Build();
    ticks = B(tree, frame, hug).Text("", FontSizes::tiny).Fill(colors::radiant_orange).Build();
    systems = B(tree, frame, hug).Direction(vertical).Gap(10).Build();

    system_nodes.SetPrefab(B(tree, systems.GetHandle(), fill).Fill(colors::radiant_orange).FontSize(FontSizes::tiny).Build());
}
void TickFrame::DisplayInfo() {
    using namespace ui;
    u32 tick = singleton.Get<TickState>().tick.Value();
    u32 fps = static_cast<u32>(1.0F / singleton.Get<TickState>().delta_time);
    data[tree].node_properties[ticks.GetHandle()].text = std::format("Tick: {:>8}   |   TPS: {:>4}   |   FPS: {:>4}", tick, fps, fps);
    data[tree].node_properties[ticks.GetHandle()].text = std::format("Tick: {:>8}   |   TPS: {:>4}   |   FPS: {:>4}", tick, fps, fps);

    constexpr f32 THOUSANDTH = 0.001F;
    auto values = std::views::zip(singleton.Get<OrchestraState>().names, singleton.Get<OrchestraState>().nano_seconds);
    auto pred = [] (const Handle<NodeTree> tree, const Handle<Node> node, const std::tuple<String&, u32&>& tuple) {
        const auto& [name, ns] = tuple;
        data[tree].node_properties[node].text = std::format("{:.3f}ms | {}", ns * THOUSANDTH * THOUSANDTH, name);
    };
    system_nodes.Set(values, pred);

    data[tree].MarkDirty();
}

static constexpr u32 GAP_SIZE { 2U };
static constexpr uint2 COLOR_INDICATOR_SIZE { 10U, 20U };
TickComponent::TickComponent(NodeReference parent) { }
void TickComponent::SetProperty(const Property property) const {
    static constexpr f32 THOUSANDTH = 0.001F;
    data[root.tree].node_properties[root.node].text = std::format("{:.3f}ms | {}", property.ns * THOUSANDTH * THOUSANDTH, property.name);
}
DebugNodeComponent::DebugNodeComponent(const NodeReference parent): root { parent.tree, B(parent.tree, parent.node, hug).Fill(colors::forest_green).Gap(GAP_SIZE).Build() },
                                                                    text { B(root.tree, root.node, hug).FontSize(FontSizes::body).Fill(colors::black).Build() },
                                                                    color_indicator { B(root.tree, root.node, COLOR_INDICATOR_SIZE).Build() } { }
void DebugNodeComponent::SetProperty(const Property property) const {
    constexpr u32 padding_offset = 10U;
    const NodeStyle& style = data[property.hovered.tree].styles[property.hovered.node];
    const NodeProperties& properties = data[property.hovered.tree].node_properties[property.hovered.node];
    const String type = !properties.text.Empty() ? "text" : style.texture.IsValid() ? "image" : "node";

    data[root.tree].styles[root.node].padding.x = padding_offset * property.layer;
    data[root.tree].node_properties[text].text = std::format("{} [{}, {}]", type, style.position.x, style.position.y);
    data[root.tree].styles[color_indicator].background_color = style.background_color;
}
InspectorFrame::InspectorFrame() {
    const Handle<Node> frame = B(tree, hug, { 10U, 30U }).Fill(colors::clear).Fill(colors::white).Direction(vertical).Build();
    debug_nodes.parent.node = frame;
}
void InspectorFrame::ShowElementStructure(const HoveredType hovered) {
    if (!hovered.has_value() || hovered->tree.id == tree.id) { return; }
    List properties { DebugNodeComponent::Property { .hovered = hovered.value(), .layer = 0U } };
    for (u32 i = 0U; i < properties.Size(); ++i) {
        const auto [hovered, layer] = properties.Back();
        for (const Handle child : data[hovered.tree].children[hovered.node]) {
            const DebugNodeComponent::Property child_property { .hovered = { .tree = hovered.tree, .node = child}, .layer = layer + 1U };
            properties.EmplaceBack(child_property);
        }
    }
    debug_nodes.Set(properties);
}
void InspectorFrame::Hide() {
    debug_nodes.SetSize(0U);
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
        Handle<Node> root = B(tree, frame, { hug, 400U }).Padding2({ 5U, 5U }).Fill(colors::forest_green).Build();
        Handle<Node> box1 = B(tree, root, { width, fill }).Fill(colors::yellow).Padding2({ 5U, 5U }).Build();
        Handle<Node> box2 = B(tree, root, { width * 2, fill }).Fill(colors::red).Build();
        Handle<Node> box3 = B(tree, root, { width * 3, fill }).Padding2({ 4U, 4U }).Gap(2U).Fill(colors::black).Build();
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
        if (input_state.keys[SDLK_LALT]) {
            inspector_frame.ShowElementStructure(singleton.Get<HoveredType>());
        } else {
            inspector_frame.Hide();
        }
    }
    data[tick_frame.tree].MarkDirty();
    if (singleton.Get<TickState>().tick.Value() % 100 == 0) { tick_frame.DisplayInfo(); }
}
}
