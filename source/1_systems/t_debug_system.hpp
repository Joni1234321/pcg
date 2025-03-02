#pragma once
#include "r_ui_node.hpp"

#include "0_engine/u_colors.hpp"

namespace pce::ui {
struct TickComponent {
    using Property = std::tuple<String&, u32>;
    NodeReference root;
    explicit TickComponent(NodeReference parent);
    void SetProperty(const Property& property) const;
};
static_assert(NodeComponent<TickComponent>);

struct DebugNodeComponent {
    struct Property {
        NodeReference hovered;
        u32 layer;
    };
    NodeReference root;
    explicit DebugNodeComponent(NodeReference parent);
    void SetProperty(const Property& property) const;

private:
    Handle<Node> text;
    Handle<Node> color_indicator;
};
static_assert(NodeComponent<DebugNodeComponent>);

struct TickFrame : Frame {
    void DisplayInfo();

    Handle<Node> root { B(frame).Node(hug).Padding2({ 10U, 0U }).Direction(vertical).Gap(10U).Build() };
    Handle<Node> ticks { B(root).Node(hug).Text(FontSizes::tiny, colors::radiant_orange).Build() };
    NodeComponentPool<TickComponent> systems { B(root).Pool<TickComponent>() };
};
struct InspectorFrame : Frame {
    void ShowElementStructure(HoveredType hovered);

    Handle<Node> root { B(frame).Node(hug).Padding2({ 10U, 30U }).Direction(vertical).Build() };
    Handle<Node> hovered_label { B(root).Node(hug).Text(FontSizes::h1, colors::black).Build() };
    NodeComponentPool<DebugNodeComponent> nodes { B(root).Pool<DebugNodeComponent>() };
};
struct TestFrame : Frame {
    TestFrame();
};
struct DebugSystem {
    TickFrame tick_frame { };
    InspectorFrame inspector_frame { };
    void operator()();
};
} // namespace pce
