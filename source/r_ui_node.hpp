#pragma once

#include <SDL3/SDL_render.h>

#include "r_ui_element.hpp"
#include "u_collections.hpp"
#include "u_types.hpp"

namespace pce::ui {
struct FrameElements {
    List<RectangleElement> rectangles { };
    List<TextElement> texts { };
};
class Color {
public:
    u8 r;
    u8 g;
    u8 b;
    u8 a;
    Color(u8 r, u8 g, u8 b) : r(r), g(g), b(b), a(255) { }
    Color(u8 r, u8 g, u8 b, u8 a) : r(r), g(g), b(b), a(a) { }
    //Color(f32 r, f32 g, f32 b) : r(r * 255U), g(g * 255U), b(b * 255U), a(255) { }
    //Color(f32 r, f32 g, f32 b, f32 a) : r(r * 255U), g(g * 255U), b(b * 255U), a(a * 255U) { }
    explicit Color(const SDL_Color color) : Color(color.r, color.g, color.b, color.a) { }
    explicit Color(const SDL_FColor color) : Color(color.r, color.g, color.b, color.a) { }
};

struct ResolvedLayout {
    SDL_FRect layout;
};
struct LayoutLength {
    enum Constraint { fixed, child_constraint, parent_constraint };
    u32 resolved;
    Constraint layout_type;
};

struct NodeTree;
struct NodeBuilder;

struct Node {
    Node* parent { nullptr };
    List<Node> children { };

    String name { };
    String text { };
    SDL_Color background_color { 0, 0, 0 };
    uint2 position { };
    uint2 padding { };

    LayoutLength width { .resolved = 0U, .layout_type = LayoutLength::child_constraint };
    LayoutLength height { .resolved = 0U, .layout_type = LayoutLength::child_constraint };

    friend NodeBuilder;
    friend NodeTree;
};

struct NodeTree {
    Node root { };
    void MarkDirty() { dirty = true; }
    const FrameElements& GetFrameElements() {
        if (dirty) {
            dirty = false;
            RecalculateLayout();
            frame_elements = CreateFrameElements();
        }
        return frame_elements;
    };

private:
    bool dirty { true };
    FrameElements frame_elements { };
    FrameElements CreateFrameElements();
    void RecalculateLayout();
};
struct NodeBuilder {
    NodeBuilder() { }
    explicit NodeBuilder(const Node& node) : node(node) { }
    NodeBuilder& Fill(SDL_Color color);
    NodeBuilder& Absolute(uint2 pos);
    NodeBuilder& Layout(LayoutLength width, LayoutLength height);
    NodeBuilder& Fixed(uint2 size);
    NodeBuilder& FixedWidth(u32 width);
    NodeBuilder& FixedHeight(u32 height);
    NodeBuilder& HugWidth();
    NodeBuilder& HugHeight();
    NodeBuilder& FillWidth();
    NodeBuilder& FillHeight();
    NodeBuilder& Text(String& string);
    Node Build(NodeTree& node_tree) {
        node.parent = nullptr;
        node_tree.root = node;
        return node;
    };
    Node& Build(Node& parent) {
        node.parent = &parent;
        parent.children.push_back(node);
        return parent.children.Back();
    }

private:
    Node node { };
};

void RenderFrameElements(SDL_Renderer* renderer, NodeTree& node_tree);
}
