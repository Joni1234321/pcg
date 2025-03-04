#include "t_debug_system.hpp"

#include "g_components.hpp"

#include "0_engine/u_colors.hpp"
#include "0_engine/u_types.hpp"

#include "1_systems/i_input_system.hpp"
#include "1_systems/u_orchestra.hpp"

namespace pce::ui {
namespace colors = colors;
void TickComponent::SetProperty(const Property& property) const {
    static constexpr f32 THOUSANDTH = 0.001F;
    const auto& [name, ns] = property;
    data[root.tree].node_properties[root.node].text = std::format("{:.3f}ms | {}", ns * THOUSANDTH * THOUSANDTH, name);
}
void DebugNodeComponent::SetProperty(const Property& property) const {
    constexpr u32 padding_offset = 10U;
    const NodeStyle& style = data[property.hovered.tree].styles[property.hovered.node];
    const NodeProperties& properties = data[property.hovered.tree].node_properties[property.hovered.node];
    const String type = !properties.text.Empty() ? properties.text.CString() : style.texture.IsValid() ? "image" : "node";
    const auto to_string = [] (LayoutLength c) -> String {
        switch (c.constraint) {
            case LayoutLength::Constraint::child_constraint:
                return "hug";
            case LayoutLength::Constraint::parent_constraint:
                return "fill";
            case LayoutLength::Constraint::fixed:
                return std::format("{} px", c.resolved);
            default:
                return "unknown";
        }
    };
    data[root.tree].styles[root.node].padding.x = padding_offset * property.layer;
    data[root.tree].node_properties[text].text = std::format("{} [{}, {}]", type, to_string(style.width), to_string(style.height));
    data[root.tree].styles[color_indicator].background_color = style.background_color;
}
void TickFrame::Update() {
    u32 tick = singleton.Get<TickState>().tick.Value();
    u32 fps = static_cast<u32>(1.0F / singleton.Get<TickState>().delta_time);
    data[tree].MarkDirty();
    static constexpr f32 THOUSAND = 1'000.0F;
    data[tree].node_properties[ticks].text = std::format("{:.3f}ms | Tick: {:>8} | TPS: {:>4} | FPS: {:>4} |", singleton.Get<TickState>().delta_time * THOUSAND, tick, fps, fps);
    systems.Set(std::views::zip(singleton.Get<OrchestraState>().names, singleton.Get<OrchestraState>().nano_seconds));
}

void DebugFrame::SetInspector(const HoveredType hovered) {
    if (!hovered.has_value() || hovered->tree == tree) { return; }
    data[tree].node_properties[hovered_label].text = std::format("[{} | {}]", hovered->tree.id, hovered->node.id);
    std::stack<DebugNodeComponent::Property> stack;
    stack.push(DebugNodeComponent::Property { .hovered = hovered.value(), .layer = 0U });
    List<DebugNodeComponent::Property> nodes { data[hovered->tree].children.Size() };

    while (!stack.empty()) {
        nodes.PushBack(stack.top());
        const auto [hovered, layer] = stack.top();
        stack.pop();
        for (const Handle child : data[hovered.tree].children[hovered.node] | std::views::reverse) { stack.push(DebugNodeComponent::Property { .hovered = { .tree = hovered.tree, .node = child }, .layer = layer + 1U }); }
    }
    debug_nodes.Set(nodes);
}
void DebugSystem::operator()() {
    InputState& input_state = singleton.Get<InputState>();
    b8 update_debug_frame = false;
    if (input_state.left_mouse_down) {
        update_debug_frame = true;
        if (input_state.keys[SDLK_LALT]) { debug_frame.SetInspector(singleton.Get<HoveredType>()); } else { debug_frame.debug_nodes.Hide(); }
    }
    if (input_state.keys[SDLK_LALT]) {
        update_debug_frame = true;
        u32 key_count = std::min(data.Get<NodeTree>().Size(), 10U);
        auto rng = std::views::iota(0U, key_count);
        const auto it = std::ranges::find_if(rng, [&] (const u32 i) { return input_state.keys_down[SDLK_0 + i]; });
        if (it != std::end(rng)) {
            const Handle<NodeTree> tree { *it };
            debug_frame.SetInspector(NodeReference { .tree = tree, .node = data[tree].Root() });
        }
    }
    if (update_debug_frame) { data[debug_frame.tree].MarkDirty(); }
    if (singleton.Get<TickState>().tick.Value() % 500U == 0U) {
        data[tick_frame.tree].MarkDirty();
        tick_frame.Update();
    }
}
TestFrame::TestFrame() {
    const Handle<Node> core_root = B(frame).Node(100U, 400U).Gap(20U).Fill(colors::clear).Build(); {
        const Handle<Node> root = B(core_root).Node(100U, 100U).Padding2({ 5U, 5U }).Fill(colors::forest_green).Build();
        const Handle<Node> box1 = B(root).Node(fill).Fill(colors::yellow).Padding2({ 5U, 5U }).Build();
        Handle<Node> box11 = B(box1).Node(fill).Fill(colors::blue).Build();
        Handle<Node> box12 = B(box1).Node(fill).Fill(colors::chocolate).Build();

        Handle<Node> box2 = B(root).Node(fill).Fill(colors::red).Build();
        Handle<Node> box3 = B(root).Node(hug, fill).Padding2({ 10U, 10U }).Fill(colors::black).Build();
    } {
        const Handle<Node> root = B(core_root).Node(100U, 100U).Padding2({ 5U, 5U }).Fill(colors::green).Build();
        Handle<Node> box1 = B(root).Node(fill).Fill(colors::yellow).Padding2({ 5U, 5U }).Build();
        Handle<Node> box2 = B(root).Node(fill).Fill(colors::red).Build();
    } {
        const Handle<Node> root = B(core_root).Node(hug).Padding2({ 5U, 5U }).Direction(vertical).Fill(colors::deep_purple).Build();
        Handle<Node> box1 = B(root).Node(hug).Text("Play", FontSizes::h1, colors::radiant_orange).Padding2({ 10U, 0U }).Build();
        Handle<Node> box2 = B(root).Node(hug).Text("Settings", FontSizes::h1, colors::cool_teal).Padding2({ 10U, 0U }).Build();
        Handle<Node> box3 = B(root).Node(hug).Text("Exit", FontSizes::h1, colors::ruby_red).Padding2({ 10U, 0U }).Build();
    } {
        const Handle<Node> root = B(core_root).Node(100U, 100U).Padding2({ 5U, 5U }).Fill(colors::sea_green).Build();
        Handle<Node> box1 = B(root).Node(fill).Fill(colors::yellow).Padding2({ 5U, 5U }).Build();
        Handle<Node> box2 = B(root).Node(fill).Fill(colors::red).Build();
        Handle<Node> box3 = B(root).Node(hug).Padding2({ 10U, 10U }).Fill(colors::black).Build();
    } {
        constexpr u32 width = 100U;
        const Handle<Node> root = B(core_root).Node(hug, 400U).Padding2({ 5U, 5U }).Fill(colors::forest_green).Build();
        Handle<Node> box1 = B(root).Node(width, fill).Fill(colors::yellow).Padding2({ 5U, 5U }).Build();
        Handle<Node> box2 = B(root).Node(width * 2, fill).Fill(colors::red).Build();
        const Handle<Node> box3 = B(root).Node(width * 3, fill).Padding2({ 4U, 4U }).Gap(2U).Fill(colors::black).Build();
        Handle<Node> box31 = B(box3).Node(fill).Fill(colors::cyan).Build();
        Handle<Node> box32 = B(box3).Node(fill).Fill(colors::chocolate).Build();
        Handle<Node> box33 = B(box3).Node(fill).Fill(colors::yellow).Build();
    }
}
}
