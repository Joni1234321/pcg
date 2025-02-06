#pragma once

#include "r_engine.hpp"
#include "r_ui_node.hpp"
#include "u_collections.hpp"

#include <SDL3/SDL_render.h>
#include <SDL3_ttf/SDL_ttf.h>

namespace pce::ui {
using HoveredType = std::optional<WeakNodeReference>;
static const RelativePath font_path { "font.ttf" };
static const RelativePath font_bold_path { "TitilliumWeb-SemiBold.ttf" };
struct DestroyRenderTextEngine {
    void operator()(TTF_TextEngine* engine) {
        Logger().Destroyed("TTF_TextEngine");
        TTF_DestroyRendererTextEngine(engine);
    }
};
struct NodeRenderSystem {
    const RenderSystem& render_system;
    HoveredType hovered { };
    FontCollection font { assets::Asset(font_path) };
    FontCollection font_bold { assets::Asset(font_bold_path) };
    UniquePointer<TTF_TextEngine, DestroyRenderTextEngine> text_engine { TTF_CreateRendererTextEngine(render_system.renderer) };
    List<std::reference_wrapper<NodeTree>> node_trees { };

    explicit NodeRenderSystem(const RenderSystem& render_system) : render_system(render_system) { }

    void HoverClickEvents(const InputSystem& input_system);
    void RenderTrees(SDL_Renderer* renderer);

    [[nodiscard]] NodeTree& AddNodeTree(NodeTree& node_tree);
    [[nodiscard]] auto GetNodeTrees() const { return node_trees | std::views::transform(&std::reference_wrapper<NodeTree>::get); }
};

class TickFrame {
    NodeHandleOptional tick_handle { };

public:
    NodeTree tree;
    TickFrame() { tick_handle = B(tree, hug, { 10U, 0U }).Text("Tick", FontSizes::tiny).Fill(colors::radiant_orange).Build(); }
    void SetInfo(u32 tick, u32 tps, u32 fps) { tree.GetProperties(tick_handle.GetHandle()).text = std::format("Tick: {:>8}   |   TPS: {:>4}   |   FPS: {:>4}", tick, tps, fps); }
};
struct InspectorFrame {
    NodeTree tree;
    void ShowElementStructure(const HoveredType& hovered);
};
struct TestFrame {
    NodeTree tree;
    TestFrame();
};

struct DebugSystem {
    NodeRenderSystem& node_render_system;

    TickFrame tick_frame { };
    InspectorFrame debug_frame { };

    explicit DebugSystem(NodeRenderSystem& node_render_system) : node_render_system { node_render_system } {
        node_render_system.node_trees.EmplaceBack(tick_frame.tree);
        node_render_system.node_trees.EmplaceBack(debug_frame.tree);
    }
    void operator()(const InputSystem& input_system, const TickSystem& tick_system) {
        if (input_system.LeftMouseDown()) {
            debug_frame.tree.MarkDirty();
            debug_frame.ShowElementStructure(node_render_system.hovered);
        }
        tick_frame.tree.MarkDirty();
        tick_frame.SetInfo(tick_system.tick.Value(), 1.0F / tick_system.tick_time, 1.0F / tick_system.delta_time);
    }
};
}
