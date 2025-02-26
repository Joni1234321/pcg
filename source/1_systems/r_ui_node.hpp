#pragma once

#include <SDL3/SDL_render.h>

#include "0_engine/u_collections.hpp"
#include "0_engine/u_texture.hpp"
#include "0_engine/u_types.hpp"

#include "1_systems/u_orchestra.hpp"
#include "1_systems/r_ui_node_data.hpp"

namespace pce::ui {
struct NodeInputSystem {
    void operator()() const;
    ~NodeInputSystem() { singleton.Get<HoveredType>() = { }; }
};
struct NodeRenderSystem {
    NodeRenderSystem() { }
    ~NodeRenderSystem() {
        data.Get<NodeTree>().Clear();
    };
    void operator()();
};

class NodeBuilder {
    NodeReference node_reference;
    NodeStyle& style { data[node_reference.tree].styles[node_reference.node] };
    NodeProperties& properties { data[node_reference.tree].node_properties[node_reference.node] };

public:
    NodeBuilder(Handle<NodeTree> tree, Layout new_layout, uint2 position);
    NodeBuilder(Handle<NodeTree> tree, Handle<Node> parent, Layout new_layout);
    [[nodiscard]] NodeBuilder& Name(const String& name);
    [[nodiscard]] NodeBuilder& Fill(SDL_Color color);
    [[nodiscard]] NodeBuilder& Texture(Handle<Texture> texture_handle);
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
