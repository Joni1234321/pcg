#pragma once

#include "r_engine.hpp"
#include "r_ui_node.hpp"
#include "r_ui_element.hpp"
#include "u_collections.hpp"
#include "u_types.hpp"

#include <SDL3/SDL_render.h>
#include <SDL3_ttf/SDL_ttf.h>

namespace pce::ui {
static const RelativePath font_path { "font.ttf" };
static const RelativePath font_bold_path { "TitilliumWeb-SemiBold.ttf" };
struct NodeRenderSystem {
    using HoveredType = std::optional<NodeReference>;

    HoveredType hovered { };
    FontCollection font { assets::Asset(font_path) };
    FontCollection font_bold { assets::Asset(font_bold_path) };
    TTF_TextEngine* text_engine;
    List<std::reference_wrapper<NodeTree>> node_trees { };

    [[nodiscard]] HoveredType GetHovered(uint2 mouse_position) const;
    void HoverClickEvents(const InputSystem& input_system);
    void RenderTrees(SDL_Renderer* renderer) const;

    [[nodiscard]] auto GetNodeTrees() const { return node_trees | std::views::transform(&std::reference_wrapper<NodeTree>::get); }
    explicit NodeRenderSystem(RenderSystem& render_system): text_engine(TTF_CreateRendererTextEngine(render_system.renderer)) { }
    ~NodeRenderSystem() { TTF_DestroyRendererTextEngine(text_engine); }
};
}
