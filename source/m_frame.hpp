#pragma once

#include "r_engine.hpp"
#include "r_ui.hpp"
#include "u_types.hpp"

namespace pce::frame {
class TickFrame {
    ui::Node::OptionalHandle tick_handle { };

public:
    ui::NodeTree tree;
    explicit TickFrame(ui::NodeRenderSystem& ui_system) : tree { ui_system.text_engine, ui_system.font }
    {
        tick_handle = ui::NodeBuilder(ui::hug).Text("Tick", ui::Fonts::tiny).Fill(colors::radiant_orange).BuildRoot(tree, { 10U, 0U });
    }
    void SetInfo(u32 tick, u32 tps, u32 fps) { tree.GetNode(tick_handle.GetHandle()).text = std::format("Tick: {:>8}   |   TPS: {:>4}   |   FPS: {:>4}", tick, tps, fps); }
};
struct InspectorFrame {
    ui::NodeTree tree;
    explicit InspectorFrame(ui::NodeRenderSystem& ui_system);
    void ShowElementStructure(const ui::NodeRenderSystem::HoveredType& hovered);
};
struct TestFrame {
    std::vector<pce::ui::RectangleElement::Handle> elements { };
    pce::ui::NodeTree tree;
    explicit TestFrame(pce::ui::NodeRenderSystem& ui_system);
    void Tick(u32 tick, pce::ui::NodeRenderSystem& ui_system);
};

class OverviewFrame {
    pce::ui::TableElement::Handle table_handle;

public:
    explicit OverviewFrame(pce::ui::NodeRenderSystem& ui_system);
    void Tick(pce::ui::NodeRenderSystem& ui_system);
};
}
