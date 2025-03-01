#pragma once
#include "r_ui_node.hpp"

namespace pce::ui {
struct TickFrame {
    Handle<NodeTree> tree { data.Create<NodeTree>() };
    TickFrame();
    void DisplayInfo();

private:
    HandleOptional<Node> ticks { };
    HandleOptional<Node> systems { };
    NodePool systems_pool { tree };
};
struct InspectorFrame {
    Handle<NodeTree> tree { data.Create<NodeTree>() };
    void ShowElementStructure(HoveredType hovered) const;
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