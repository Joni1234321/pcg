#pragma once

#include <SDL3/SDL_render.h>

#include "0_engine/u_assets.hpp"
#include "0_engine/u_collections.hpp"
#include "0_engine/u_types.hpp"

#include "1_systems/u_orchestra.hpp"
#include "1_systems/r_ui_node_data.hpp"

namespace pce::ui {
struct NodeInputSystem {
    static HoveredType hovered;
    void operator()() const;
    ~NodeInputSystem() { hovered = { }; }
};
struct NodeRenderSystem {
    NodeRenderSystem() { }
    ~NodeRenderSystem() {
        data.Get<NodeTree>().Clear();
    };
    void operator()();
};
inline HoveredType NodeInputSystem::hovered { };

class NodeBuilder {
    NodeReference node_reference;
    NodeStyle& style { data[node_reference.tree_handle].node_styles[node_reference.node_handle] };
    NodeProperties& properties { data[node_reference.tree_handle].node_properties[node_reference.node_handle] };

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
