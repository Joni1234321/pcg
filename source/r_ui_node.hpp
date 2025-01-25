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
    enum Constraint { child_constraint, parent_constraint, fixed };
    enum RelatedConstraint { hug, fill };
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

    SDL_FRect bounding_box { };

    [[nodiscard]] constexpr uint2 NonContentSize() const { return padding; }
    [[nodiscard]] constexpr uint2 OuterBoxPosition() const { return position; }
    [[nodiscard]] constexpr uint2 OuterBoxSize() const { return { width.resolved, height.resolved }; }
    [[nodiscard]] constexpr uint2 OuterBoxEndPosition() const { return OuterBoxPosition() + OuterBoxSize(); }
    [[nodiscard]] constexpr uint2 InnerBoxPosition() const { return OuterBoxPosition() + NonContentSize(); }
    [[nodiscard]] constexpr uint2 InnerBoxSize() const { return OuterBoxSize() - NonContentSize() * 2U; }

    [[nodiscard]] constexpr SDL_FRect OuterRect() const { return { .x = static_cast<f32>(position.x), .y = static_cast<f32>(position.y), .w = static_cast<f32>(width.resolved), .h = static_cast<f32>(height.resolved) }; };
    [[nodiscard]] constexpr b8 IsInside(uint2 screen_position) const;

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

    Node *HitNode(uint2 screen_position) {
        if (!root.IsInside(screen_position)) { return nullptr; }
        Node* node = &root;
        const auto is_inside_node = [screen_position] (const Node& child) -> b8 { return child.IsInside(screen_position); };
        while (true) {
            auto node_iterator = std::ranges::find_if(node->children, is_inside_node);
            if (node_iterator == node->children.end()) { break; }
            node = &*node_iterator;
        }
        return node;
    }

private:
    bool dirty { true };
    FrameElements frame_elements { };
    FrameElements CreateFrameElements();
    void RecalculateLayout();
};
struct NodeBuilder {
    explicit NodeBuilder(uint2 size);
    explicit NodeBuilder(LayoutLength::RelatedConstraint constraint);
    NodeBuilder(u32 width, LayoutLength::RelatedConstraint height_constraint);
    NodeBuilder(LayoutLength::RelatedConstraint width_constraint, u32 height);
    NodeBuilder(LayoutLength::RelatedConstraint width_constraint, LayoutLength::RelatedConstraint height_constraint);
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
    NodeBuilder& Padding(uint2 padding);
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
