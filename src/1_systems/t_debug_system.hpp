#pragma once
#include "r_ui_node.hpp"

#include "0_engine/u_colors.hpp"

namespace pce::ui {
struct TickComponent : NodeComponentBase {
    using Property = std::tuple<String&, nanoseconds64, nanoseconds64>;
    explicit TickComponent(const NodeReference parent) : NodeComponentBase { parent.tree, B(parent).Node(hug).Text(FontSizes::tiny, colors::radiant_orange).Build() } { }
    void SetProperty(const Property& property) const;
};
static_assert(NodeComponent<TickComponent>);
struct DebugNodeComponent : NodeComponentBase {
    struct Property {
        NodeReference hovered;
        u32 layer;
    };
    explicit DebugNodeComponent(const NodeReference parent) : NodeComponentBase { parent.tree, B(parent).Node(hug).Center().Build() } { }
    void SetProperty(const Property& property) const;

private:
    Handle<Node> text_wrapper { B(root).Node(hug).Fill(colors::clear).Build() };
    Handle<Node> text { B(text_wrapper).Node(hug).Text(FontSizes::small, colors::black).Build() };
    Handle<Node> color_indicator { B(root).Node(10U).Build() };
};
static_assert(NodeComponent<DebugNodeComponent>);
struct TickFrame : Frame {
    void Update();

    Handle<Node> root { B(frame).Node(hug).Padding2({ 10U, 0U }).Direction(vertical).Build() };
    Handle<Node> ticks { B(root).Node(hug).Padding2({ 0U, 4U }).Text(FontSizes::tiny, colors::radiant_orange).Build() };
    NodeComponentPool<TickComponent> systems { B(root).Pool<TickComponent>() };
};
struct DebugFrame : Frame {
    void SetInspector(HoveredType hovered);

    Handle<Node> root { B(frame).Node(hug).Padding2({ 10U, 30U }).Fill(colors::beige).Direction(vertical).Build() };
    Handle<Node> hovered_label { B(root).Node(hug).Text(FontSizes::body, colors::black).Build() };

    Handle<Node> box { B(root).Node(hug).Fill(colors::orange).Padding(4U).Direction(vertical).Center().Build() };

    Handle<Node> padding_top { B(box).Node(hug).Text(FontSizes::body, colors::black).Build() };
    Handle<Node> center { B(box).Node(hug).Gap(4U).Build() };
    Handle<Node> padding_bottom { B(box).Node(hug).Text(FontSizes::body, colors::black).Center().Build() };

    Handle<Node> padding_left { B(center).Node(hug).Text(FontSizes::body, colors::black).Build() };
    Handle<Node> inner { B(center).Node(hug).Fill(colors::beige).Build() };
    Handle<Node> padding_right { B(center).Node(hug).Text(FontSizes::body, colors::black).Build() };

    Handle<Node> size { B(inner).Node(hug).Text(FontSizes::body, colors::black).Build() };

    Handle<Node> space { B(root).Node(hug).Padding(4U).Build() };
    Handle<Node> tree_visualizer { B(root).Node(hug).Direction(vertical).Build() };
    NodeComponentPool<DebugNodeComponent> debug_nodes { B(tree_visualizer).Pool<DebugNodeComponent>() };
};
struct TestFrame : Frame {
    TestFrame();
};
struct DebugSystem {
    TickFrame tick_frame { };
    DebugFrame debug_frame { };
    void operator()();
};
} // namespace pce
