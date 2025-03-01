#pragma once
#include "r_ui_node.hpp"

namespace pce::ui {
struct TickComponent {
    struct Property {
        String name;
        u32 ns;
    };
    NodeReference root;
    explicit TickComponent(NodeReference parent);
    void SetProperty(Property property) const;

private:

};
static_assert(NodeComponent<TickComponent>);

struct DebugNodeComponent {
    struct Property {
        NodeReference hovered;
        u32 layer;
    };
    NodeReference root;
    explicit DebugNodeComponent(NodeReference parent);
    void SetProperty(Property property) const;

private:
    Handle<Node> text;
    Handle<Node> color_indicator;
};
static_assert(NodeComponent<DebugNodeComponent>);

struct TickFrame {
    Handle<NodeTree> tree { data.Create<NodeTree>() };
    TickFrame();
    void DisplayInfo();

private:
    HandleOptional<Node> ticks { };
    HandleOptional<Node> systems { };
    NodePool system_nodes { tree };
};

struct InspectorFrame {
    Handle<NodeTree> tree { data.Create<NodeTree>() };
    InspectorFrame();
    void ShowElementStructure(HoveredType hovered);
    void Hide();

private:
    NodeComponentPool<DebugNodeComponent> debug_nodes { .parent = NodeReference { .tree = tree, .node = Handle<Node> { 0U } } };
};
struct TestFrame {
    Handle<NodeTree> tree { data.Create<NodeTree>() };
    TestFrame();
};
struct DebugSystem {
    TickFrame tick_frame { };
    InspectorFrame inspector_frame { };
    void operator()();
};
} // namespace pce
