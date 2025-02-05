#pragma once

#include "r_engine.hpp"
#include "r_ui.hpp"
#include "u_types.hpp"

namespace pce::frame {
class TickFrame {
    ui::NodeHandleOptional tick_handle { };

public:
    ui::NodeTree tree;
    TickFrame() { tick_handle = ui::B(tree, ui::hug, { 10U, 0U }).Text("Tick", ui::FontSizes::tiny).Fill(colors::radiant_orange).Build(); }
    void SetInfo(u32 tick, u32 tps, u32 fps) { tree.GetProperties(tick_handle.GetHandle()).text = std::format("Tick: {:>8}   |   TPS: {:>4}   |   FPS: {:>4}", tick, tps, fps); }
};
struct InspectorFrame {
    ui::NodeTree tree;
    void ShowElementStructure(const ui::HoveredType& hovered);
};
struct TestFrame {
    ui::NodeTree tree;
    TestFrame();
};
}
