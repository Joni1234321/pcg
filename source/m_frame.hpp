#pragma once

#include "r_engine.hpp"
#include "r_ui.hpp"
#include "u_types.hpp"

namespace pce::frame {
class TickFrame {
    ui::Node::OptionalHandle tick_handle { };

public:
    ui::NodeTree tree;
    TickFrame() { tick_handle = ui::NodeBuilder(ui::hug).Text("Tick", ui::Fonts::tiny).Fill(colors::radiant_orange).BuildRoot(tree, { 10U, 0U }); }
    void SetInfo(u32 tick, u32 tps, u32 fps) { tree.GetNode(tick_handle.GetHandle()).text = std::format("Tick: {:>8}   |   TPS: {:>4}   |   FPS: {:>4}", tick, tps, fps); }
};
struct InspectorFrame {
    ui::NodeTree tree;
    void ShowElementStructure(const ui::NodeRenderSystem::HoveredType& hovered);
};
struct TestFrame {
    ui::NodeTree tree;
    TestFrame();
};
}
