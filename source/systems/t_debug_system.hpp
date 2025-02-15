#pragma once
#include "r_ui_node.hpp"

namespace pce::ui {
struct TickFrame {
    Handle<NodeTree> tree_handle { NodeRenderSystem::node_trees.EmplaceBack() };
    TickFrame() { DisplayInfo(); }
    void DisplayInfo() const;
};
struct InspectorFrame {
    Handle<NodeTree> tree_handle { NodeRenderSystem::node_trees.EmplaceBack() };
    void ShowElementStructure(HoveredType hovered);
};
struct TestFrame {
    Handle<NodeTree> tree_handle { NodeRenderSystem::node_trees.EmplaceBack() };
    TestFrame();
};
struct DebugSystem {
    TickFrame tick_frame { };
    InspectorFrame inspector_frame { };
    void operator()();
};
} // namespace pce
