#pragma once

#include "r_engine.hpp"
#include "r_ui_node.hpp"
#include "u_collections.hpp"
#include "u_types.hpp"

#include <SDL3/SDL_render.h>
#include <SDL3_ttf/SDL_ttf.h>

namespace pce::ui {
using HoveredType = std::optional<NodeReference>;
static const RelativePath font_path { "font.ttf" };
static const RelativePath font_bold_path { "TitilliumWeb-SemiBold.ttf" };
struct DestroyRenderTextEngine {
    void operator()(TTF_TextEngine* engine) {
        Logger().Destroyed("RenderTextEngine");
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

    [[nodiscard]] auto GetNodeTrees() const { return node_trees | std::views::transform(&std::reference_wrapper<NodeTree>::get); }
};
}
