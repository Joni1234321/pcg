#pragma once
#include "r_ui_node.hpp"

namespace pce::ui {
struct TickFrame {
    Handle<NodeTree> tree_handle { data.Create<NodeTree>() };
    TickFrame();
    void DisplayInfo() const;

private:
    HandleOptional<Node> ticks { };
    HandleOptional<Node> systems { };
    NodePool systems_pool { };
};
struct InspectorFrame {
    Handle<NodeTree> tree_handle { data.Create<NodeTree>() };
    void ShowElementStructure(HoveredType hovered) const;
};
struct TestFrame {
    Handle<NodeTree> tree_handle { data.Create<NodeTree>() };
    TestFrame();
};
struct DebugSystem {
    TickFrame tick_frame { };
    InspectorFrame inspector_frame { };
    void operator()() const;
};
} // namespace pce
