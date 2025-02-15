#pragma once
#include "r_ui_node.hpp"

#include "engine/u_colors.hpp"

namespace pce::ui {
struct TickFrame {
    Handle<NodeTree> tree_handle { NodeRenderSystem::node_trees.EmplaceBack() };
    TickFrame() { DisplayInfo(); }
    void DisplayInfo() {
        using namespace ui;
        u32 tick = TickSystem::tick_table.tick.Value();
        u32 tps = 1.0F / TickSystem::tick_table.tick_time;
        u32 fps = 1.0F / TickSystem::tick_table.delta_time;
        NodeRenderSystem::node_trees[tree_handle].Clear();
        Handle<Node> frame = B(tree_handle, hug, { 10U, 0U }).Direction(vertical).Build();
        Handle<Node> ticks = B(tree_handle, frame, hug).Text(std::format("Tick: {:>8}   |   TPS: {:>4}   |   FPS: {:>4}", tick, tps, fps), FontSizes::tiny).Fill(colors::radiant_orange).Build();
        Handle<Node> systems = B(tree_handle, frame, hug).Direction(vertical).Gap(10).Build();
        for (const auto [name, ns] : std::views::zip(Orchestra::orchestra_table.names, Orchestra::orchestra_table.nano_seconds)) {
            constexpr f32 THOUSANDTH = 0.001F;
            (void)B(tree_handle, systems, fill).Fill(colors::radiant_orange).Text(std::format("{:.3f}ms | {}", ns * THOUSANDTH * THOUSANDTH, name), FontSizes::tiny).Build();
        }
        NodeRenderSystem::node_trees[tree_handle].MarkDirty();
    }
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
    void operator()() {
        using namespace ui;
        if (InputSystem::input_table.left_mouse_down) {
            NodeRenderSystem::node_trees[inspector_frame.tree_handle].MarkDirty();
            if (InputSystem::input_table.keys[SDLK_LALT]) { inspector_frame.ShowElementStructure(NodeInputSystem::hovered); } else { NodeRenderSystem::node_trees[inspector_frame.tree_handle].Clear(); }
        }
        NodeRenderSystem::node_trees[tick_frame.tree_handle].MarkDirty();
        if (TickSystem::tick_table.tick.Value() % 100 == 0) { tick_frame.DisplayInfo(); }
    }
};
} // namespace pce
