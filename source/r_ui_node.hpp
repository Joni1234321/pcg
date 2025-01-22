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

struct NodeGenerator;
struct NodeBuilder;

class Node {
    String name { };
    String text { };
    SDL_Color background_color { 0, 0, 0 };
    uint2 position { };
    uint2 padding { };
    uint2 size { };

    LayoutLength width { .resolved = 0U, .layout_type = LayoutLength::child_constraint };
    LayoutLength height { .resolved = 0U, .layout_type = LayoutLength::child_constraint };
    Node* parent = nullptr;
    List<Node> children { };

    friend NodeGenerator;
    friend NodeBuilder;

public:
    Node() { }
    void SetWidth(LayoutLength new_width);
    [[nodiscard]] Node *GetParent() const { return parent; }
    [[nodiscard]] SDL_Color GetBackgroundColor() const { return background_color; }
    [[nodiscard]] uint2 GetResolvedPosition() const { return { width.resolved, height.resolved }; }
    void AddChild(Node& child) {
        child.parent = this;
        children.PushBack(child);
    }
    void AddChild(Node&& child) {
        child.parent = this;
        children.PushBack(child);
    }
    [[nodiscard]] const List<Node>& GetChildren() const { return children; }
};

class TextNode : public Node {
    TextNode() : Node() { };
    void SetText(String& string);
    String& GetText();

private:
    Color color { 0, 0, 0 };
    String text { };
};
struct NodeBuilder {
    NodeBuilder() : node() { }
    explicit NodeBuilder(const Node& node) : node(node) { }
    NodeBuilder& Color(SDL_Color color);
    NodeBuilder& Size(LayoutLength width, LayoutLength height);
    NodeBuilder& AbsolutePosition(uint2 pos);
    NodeBuilder& Layout(LayoutLength width, LayoutLength height);
    NodeBuilder& Fixed(uint2 size);
    NodeBuilder& FixedWidth(u32 width);
    NodeBuilder& FixedHeight(u32 height);
    NodeBuilder& HugWidth();
    NodeBuilder& HugHeight();
    NodeBuilder& FillWidth();
    NodeBuilder& FillHeight();
    NodeBuilder& Text(String& string);
    Node Build() { return node; };

private:
    Node node;
};
struct NodeGenerator {
    void RecalculateLayout(Node* root);
    FrameElements CreateFrameElements(Node* root);
};
void RenderFrameElements(SDL_Renderer* renderer, FrameElements& frame_elements);
}
