#pragma once

#include <SDL3/SDL_render.h>

#include "0_engine/u_assets.hpp"
#include "0_engine/u_collections.hpp"
#include "0_engine/u_types.hpp"
#include "1_systems/r_ui_node_data.hpp"

namespace pce::ui {
struct NodeInputSystem {
    static HoveredType hovered;
    void operator()();
    ~NodeInputSystem() { hovered = { }; }
};
struct NodeRenderSystem {
    static HandleList<NodeTree> node_trees;
    static FontCollection font;
    ~NodeRenderSystem() { node_trees.Clear(); font.Clear(); };
    void operator()();
};
static const RelativePath font_path { "font.ttf" };
static const RelativePath font_bold_path { "TitilliumWeb-SemiBold.ttf" };
inline HoveredType NodeInputSystem::hovered { };
inline HandleList<NodeTree> NodeRenderSystem::node_trees { 120U };
inline FontCollection NodeRenderSystem::font { Asset(font_path) };

class NodeBuilder {
    NodeReference node_reference;
    NodeStyle& style { NodeRenderSystem::node_trees[node_reference.tree_handle].node_styles[node_reference.node_handle] };
    NodeProperties& properties { NodeRenderSystem::node_trees[node_reference.tree_handle].node_properties[node_reference.node_handle] };

public:
    NodeBuilder(Handle<NodeTree> tree_handle, Layout new_layout, uint2 position);
    NodeBuilder(Handle<NodeTree> tree_handle, Handle<Node> parent_handle, Layout new_layout);
    [[nodiscard]] NodeBuilder& Name(const String& name);
    [[nodiscard]] NodeBuilder& Fill(SDL_Color color);
    [[nodiscard]] NodeBuilder& Padding(u32 padding);
    [[nodiscard]] NodeBuilder& Padding2(uint2 padding);
    [[nodiscard]] NodeBuilder& Padding4(uint4 padding);
    [[nodiscard]] NodeBuilder& Gap(u32 gap);
    [[nodiscard]] NodeBuilder& GapAuto();
    [[nodiscard]] NodeBuilder& Direction(FlexDirection direction);
    [[nodiscard]] NodeBuilder& Text(const String& string);
    [[nodiscard]] NodeBuilder& Text(String&& string);
    [[nodiscard]] NodeBuilder& Text(const String& string, FontSizes font_size);
    [[nodiscard]] NodeBuilder& Text(String&& string, FontSizes font_size);
    [[nodiscard]] NodeBuilder& Alignment(Alignment alignment);
    [[nodiscard]] NodeBuilder& Right();
    [[nodiscard]] NodeBuilder& Center();
    [[nodiscard]] NodeBuilder& Left();
    Handle<Node> Build() const;
};
using B = NodeBuilder;
} // pce::ui
