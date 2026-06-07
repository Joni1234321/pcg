#pragma once
#include "r_ui_node.hpp"

import pce.engine.colors;

namespace pce::ui {
struct TickComponent : NodeComponentBase {
    using Property = std::tuple<String&, nanoseconds64, nanoseconds64>;
    explicit TickComponent(const NodeReference parent) : NodeComponentBase { parent.tree, B(parent).Node(hug).Text(FontSizes::body, colors::RADIANT_ORANGE).Build() } { }
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
    Handle<Node> text_wrapper { B(root).Node(hug).Fill(colors::CLEAR).Build() };
    Handle<Node> text { B(text_wrapper).Node(hug).Text(FontSizes::small, colors::BLACK).Build() };
    Handle<Node> color_indicator { B(root).Node(10U).Build() };
};
static_assert(NodeComponent<DebugNodeComponent>);
struct TickFrame : Frame {
    void Update();

    Handle<Node> root { B(frame).Node(hug).Padding2({ 10U, 0U }).Direction(vertical).Build() };
    Handle<Node> ticks { B(root).Node(hug).Padding2({ 0U, 4U }).Text(FontSizes::body, colors::RADIANT_ORANGE).Build() };
    NodeComponentPool<TickComponent> systems { B(root).Pool<TickComponent>() };
};
struct DebugFrame : Frame {
    void SetInspector(HoveredType hovered);

    Handle<Node> root { B(frame).Node(hug).Padding2({ 10U, 30U }).Fill(colors::BEIGE).Direction(vertical).Build() };
    Handle<Node> hovered_label { B(root).Node(hug).Text(FontSizes::body, colors::BLACK).Build() };

    Handle<Node> box { B(root).Node(hug).Fill(colors::ORANGE).Padding(4U).Direction(vertical).Center().Build() };

    Handle<Node> padding_top { B(box).Node(hug).Text(FontSizes::body, colors::BLACK).Build() };
    Handle<Node> center { B(box).Node(hug).Gap(4U).Build() };
    Handle<Node> padding_bottom { B(box).Node(hug).Text(FontSizes::body, colors::BLACK).Center().Build() };

    Handle<Node> padding_left { B(center).Node(hug).Text(FontSizes::body, colors::BLACK).Build() };
    Handle<Node> inner { B(center).Node(hug).Fill(colors::BEIGE).Build() };
    Handle<Node> padding_right { B(center).Node(hug).Text(FontSizes::body, colors::BLACK).Build() };

    Handle<Node> size { B(inner).Node(hug).Text(FontSizes::body, colors::BLACK).Build() };

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
} // namespace pce::ui
