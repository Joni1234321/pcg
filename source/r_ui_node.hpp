#pragma once

#include <algorithm>
#include <ranges>
#include <functional>

#include <SDL3/SDL_render.h>

#include <SDL3_ttf/SDL_textengine.h>

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
inline SDL_Color lighten_color(const SDL_Color color, const f32 factor) {
    auto lerp = [] (u8 channel, f32 factor, u8 target) -> u8 { return static_cast<u8>(channel + (target - channel) * factor); };
    return SDL_Color { lerp(color.r, factor, 255), lerp(color.g, factor, 255), lerp(color.b, factor, 255), color.a };
}

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
    struct Handle {
        u32 id { U32_MAX };
        constexpr Handle() noexcept = default;
        explicit constexpr Handle(const u32 value) noexcept : id(value) { }
        [[nodiscard]] bool IsRoot() const noexcept { return id == 0U; }
        [[nodiscard]] bool HasValue() const noexcept { return id != U32_MAX; }
    };
    Node() = default;
    String name { };
    String text { };
    TTF_Text* ttf_text { nullptr };

    SDL_FRect bounding_box { };

    SDL_Color background_color { 0, 0, 0 };
    SDL_Color background_color_hover { 0, 0, 0 };

    uint2 position { U32_MAX, U32_MAX };
    uint2 padding { 0U, 0U };
    u32 gap { 0U };

    LayoutLength width { .resolved = 0U, .layout_type = LayoutLength::child_constraint };
    LayoutLength height { .resolved = 0U, .layout_type = LayoutLength::child_constraint };

    std::function<void(Node*)> on_click { };
    std::function<void(Node*)> on_hover { };
    std::function<void(Node*)> on_hover_out { };

    void OnHover() {
        std::swap(background_color, background_color_hover);
        if (on_hover) { on_hover(this); }
    }
    void OnHoverOut() {
        std::swap(background_color, background_color_hover);
        if (on_hover_out) { on_hover_out(this); }
    }
    void OnClick() { if (on_click) { on_click(this); } }

    [[nodiscard]] constexpr uint2 NonContentSize() const { return padding; }
    [[nodiscard]] constexpr uint2 OuterBoxPosition() const { return position; }
    [[nodiscard]] constexpr uint2 OuterBoxSize() const { return { width.resolved, height.resolved }; }
    [[nodiscard]] constexpr uint2 OuterBoxEndPosition() const { return OuterBoxPosition() + OuterBoxSize(); }
    [[nodiscard]] constexpr uint2 InnerBoxPosition() const { return OuterBoxPosition() + NonContentSize(); }
    [[nodiscard]] constexpr uint2 InnerBoxSize() const { return OuterBoxSize() - NonContentSize() * 2U; }

    [[nodiscard]] constexpr SDL_FRect OuterRect() const { return { .x = static_cast<f32>(OuterBoxPosition().x), .y = static_cast<f32>(OuterBoxPosition().y), .w = static_cast<f32>(OuterBoxSize().x), .h = static_cast<f32>(OuterBoxSize().y) }; };
    [[nodiscard]] constexpr b8 IsInside(uint2 screen_position) const {
        uint2 start { static_cast<u32>(bounding_box.x), static_cast<u32>(bounding_box.y) };
        uint2 relative = screen_position - start;
        return relative.x < static_cast<u32>(bounding_box.w) && relative.y < static_cast<u32>(bounding_box.h);
    }

    friend NodeBuilder;
    friend NodeTree;
};
struct NodeTree {
    TTF_TextEngine* text_engine;
    FontCollection& font_collection;

    List<Node> nodes { };
    List<Node::Handle> parents { };
    List<List<Node::Handle>> children { };

    NodeTree(TTF_TextEngine* text_engine, FontCollection& font) : text_engine(text_engine), font_collection(font) { }

    [[nodiscard]] static constexpr Node::Handle Root() { return Node::Handle { 0U }; }
    Node::Handle SetRoot(const Node& root) {
        ASSERT_DBG(nodes.Empty(), "Setting root non empty tree");
        nodes.PushBack(root);
        parents.PushBack(Node::Handle { });
        children.EmplaceBack();

        return Root();
    }

    [[nodiscard]] Node& GetNode(const Node::Handle node) { return nodes[node.id]; }
    [[nodiscard]] Node::Handle Parent(const Node::Handle node) { return parents[node.id]; }
    [[nodiscard]] const List<Node::Handle>& Children(const Node::Handle node) { return children[node.id]; }
    [[nodiscard]] Node::Handle AddNode(const Node& node, const Node::Handle parent_handle) {
        nodes.PushBack(node);
        parents.PushBack(parent_handle);
        children.EmplaceBack();

        Node::Handle handle { nodes.Size() - 1 };
        children[parent_handle.id].PushBack(handle);
        return handle;
    }

    void Propagate(Node::Handle node_handle, const std::function<void(Node&)>& proj) {
        while (node_handle.HasValue()) {
            std::invoke(proj, GetNode(node_handle));
            node_handle = Parent(node_handle);
        }
    }

    void MarkDirty() { dirty = true; }
    const FrameElements& GetFrameElements() {
        if (dirty) {
            dirty = false;
            RecalculateLayout();
            frame_elements = CreateFrameElements();
        }
        return frame_elements;
    };

    Node::Handle HitNode(uint2 screen_position) {
        if (!GetNode(Root()).IsInside(screen_position)) { return Node::Handle { }; }
        Node::Handle node_handle { 0U };
        const auto is_inside_node = [screen_position, this] (const Node::Handle child_handle) -> b8 { return this->GetNode(child_handle).IsInside(screen_position); };
        while (true) {
            auto node_iterator = std::ranges::find_if(Children(node_handle), is_inside_node, std::identity { });
            if (node_iterator == Children(node_handle).end()) { break; }
            node_handle = *node_iterator;
        }
        return node_handle;
    }

private:
    bool dirty { true };
    FrameElements frame_elements { };
    FrameElements CreateFrameElements();
    void RecalculateLayout();
    TextElement CreateTextAligned(const String& string, SDL_Color color, f32 x, f32 y, TextAlign alignment, u32 parent_width) const;
};

struct NodeBuilder {
    explicit NodeBuilder(uint2 size);
    explicit NodeBuilder(LayoutLength::RelatedConstraint constraint);
    NodeBuilder(u32 width, LayoutLength::RelatedConstraint height_constraint);
    NodeBuilder(LayoutLength::RelatedConstraint width_constraint, u32 height);
    NodeBuilder(LayoutLength::RelatedConstraint width_constraint, LayoutLength::RelatedConstraint height_constraint);
    NodeBuilder& Name(const String& name);
    NodeBuilder& Fill(SDL_Color color);
    // NodeBuilder& Absolute(uint2 pos);
    NodeBuilder& Layout(LayoutLength width, LayoutLength height);
    NodeBuilder& Fixed(uint2 size);
    NodeBuilder& FixedWidth(u32 width);
    NodeBuilder& FixedHeight(u32 height);
    NodeBuilder& HugWidth();
    NodeBuilder& HugHeight();
    NodeBuilder& FillWidth();
    NodeBuilder& FillHeight();
    NodeBuilder& Padding(uint2 padding);
    NodeBuilder& Gap(u32 gap);
    NodeBuilder& Text(const String& string);
    NodeBuilder& Text(String&& string);
    void Finalize(NodeTree& node_tree) {
        node_tree.MarkDirty();

        const f32 factor = 0.5F;
        node.background_color_hover = lighten_color(node.background_color, factor);

        node.on_click = [&] (Node* node) -> void { Logger().Log("Clicked"); };
        node.on_hover = [&] (Node* node) -> void { Logger().Log("Hover"); };
        node.on_hover_out = [&] (Node* node) -> void { Logger().Log("On Hover Out"); };
    }
    Node::Handle BuildRoot(NodeTree& node_tree, const uint2 position) {
        Finalize(node_tree);
        node.position = position;
        node_tree.SetRoot(node);
        return Node::Handle { 0U };
    };
    Node::Handle Build(NodeTree& node_tree, Node::Handle parent_handle) {
        Finalize(node_tree);
        return node_tree.AddNode(node, parent_handle);
    }

private:
    Node node { };
};
} // pce::ui
