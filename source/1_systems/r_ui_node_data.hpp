#pragma once

#include <functional>

#include <SDL3_ttf/SDL_ttf.h>

#include "0_engine/u_assets.hpp"
#include "0_engine/u_collections.hpp"
#include "0_engine/u_fonts.hpp"
#include "0_engine/u_logger.hpp"
#include "0_engine/u_types.hpp"

namespace pce::ui {
enum class TextAlign { left, center, right };
enum RelativeConstraint : u8 { hug, fill };
enum FlexDirection : u8 { horizontal, vertical };
enum Alignment : u8 { top_left, top_center, top_right, left, center, right, bottom_left, bottom_center, bottom_right };

struct TextElement {
    TTF_Text* text;
    float2 position;
};
struct RectangleElement {
    SDL_Color color;
    SDL_FRect rect;
};
struct TextureElement {
    SDL_FRect rect;
    SDL_Texture* texture;
};
struct FrameElements {
    List<TextureElement> textures { };
    List<RectangleElement> rectangles { };
    List<TextElement> texts { };
};

// Building
struct LayoutLength {
    enum Constraint : u8 { child_constraint, parent_constraint, fixed };
    u32 resolved;
    Constraint constraint;
};
struct Layout {
    LayoutLength width;
    LayoutLength height;
    [[nodiscard]] static constexpr LayoutLength::Constraint ToConstraint(const RelativeConstraint related_constraint) {
        return related_constraint == hug ? LayoutLength::child_constraint : LayoutLength::parent_constraint;
    }
    Layout(const uint2 size) : width { size.x, LayoutLength::fixed }, height { size.y, LayoutLength::fixed } { }
    Layout(const RelativeConstraint relative_constraint) : width { U32_MAX, ToConstraint(relative_constraint) }, height { U32_MAX, ToConstraint(relative_constraint) } { }
    Layout(const u32 width, const RelativeConstraint height_constraint) : width { width, LayoutLength::fixed }, height { U32_MAX, ToConstraint(height_constraint) } { }
    Layout(const RelativeConstraint width_constraint, const u32 height) : width { U32_MAX, ToConstraint(width_constraint) }, height { height, LayoutLength::fixed } { }
    Layout(const RelativeConstraint width_constraint, const RelativeConstraint height_constraint) : width { U32_MAX, ToConstraint(width_constraint) }, height { U32_MAX, ToConstraint(height_constraint) } { }
};

// Forward
struct Node;
struct NodeTree;
struct NodeReference {
    Handle<NodeTree> tree_handle;
    Handle<Node> node_handle;
};
using NodeReaction = std::function<void(NodeReference)>;
using HoveredType = std::optional<NodeReference>;

// Nodes
struct NodeStyle : LogDestroyWithCount<NodeStyle> {
    SDL_FRect bounding_box { };
    SDL_Color background_color { 0, 0, 0, 0 };
    HandleOptional<Texture> texture { };

    uint2 position { U32_MAX, U32_MAX };
    uint4 padding { 0U, 0U, 0U, 0U };
    u32 resolved_gap { 0U };
    b8 gap_auto { false };

    LayoutLength width { .resolved = 0U, .constraint = LayoutLength::child_constraint };
    LayoutLength height { .resolved = 0U, .constraint = LayoutLength::child_constraint };
    FlexDirection direction { horizontal };
    Alignment alignment { top_left };

    [[nodiscard]] constexpr uint4 NonContentSize4() const { return padding; }
    [[nodiscard]] constexpr uint2 NonContentSize2() const { return { padding.x + padding.z, padding.y + padding.w }; }
    [[nodiscard]] constexpr uint2 OuterBoxPosition() const { return position; }
    [[nodiscard]] constexpr uint2 OuterBoxSize() const { return { width.resolved, height.resolved }; }
    [[nodiscard]] constexpr uint2 OuterBoxEndPosition() const { return OuterBoxPosition() + OuterBoxSize(); }
    [[nodiscard]] constexpr uint2 InnerBoxPosition() const { return OuterBoxPosition() + uint2 { NonContentSize4().x, NonContentSize4().y }; }
    [[nodiscard]] constexpr uint2 InnerBoxSize() const { return OuterBoxSize() - NonContentSize2(); }
    [[nodiscard]] constexpr SDL_FRect OuterRect() const {
        return { .x = static_cast<f32>(OuterBoxPosition().x), .y = static_cast<f32>(OuterBoxPosition().y), .w = static_cast<f32>(OuterBoxSize().x), .h = static_cast<f32>(OuterBoxSize().y) };
    };
    [[nodiscard]] constexpr b8 IsInside(const uint2 screen_position) const {
        const uint2 start { static_cast<u32>(bounding_box.x), static_cast<u32>(bounding_box.y) };
        const uint2 relative = screen_position - start;
        return relative.x < static_cast<u32>(bounding_box.w) && relative.y < static_cast<u32>(bounding_box.h);
    }
};
struct NodeProperties {
    String name { };
    String text { };
    FontSizes font_size { FontSizes::body };

    NodeReaction on_click { };
    NodeReaction on_hover { };
    NodeReaction on_hover_out { };
};
struct NodeTree {
    struct DestroyText {
        void operator()(TTF_Text* text) const {
            Logger().Destroyed("TTF_Text");
            TTF_DestroyText(text);
        }
    };
    template <class T> using HandleList = HandleList<T, Handle<Node>>;
    static constexpr u32 DEFAULT_COUNT = 64U;
    HandleList<NodeStyle> node_styles { DEFAULT_COUNT };
    HandleList<NodeProperties> node_properties { DEFAULT_COUNT };
    HandleList<UniquePointer<TTF_Text, DestroyText>> node_ttf_texts { DEFAULT_COUNT };
    HandleList<Handle<Node>> parents { DEFAULT_COUNT };
    HandleList<List<Handle<Node>>> children { DEFAULT_COUNT };

    b8 display { true };
    b8 dirty { true };
    FrameElements frame_elements { };

    NodeTree() = default;
    NodeTree(const NodeTree&) = delete;
    NodeTree& operator=(const NodeTree&) = delete;
    NodeTree(NodeTree&&) noexcept = default;
    NodeTree& operator=(NodeTree&&) noexcept = default;

    [[nodiscard]] constexpr Handle<Node> Root() const { return node_styles.First(); }
    [[nodiscard]] constexpr b8 Empty() const { return node_styles.Empty(); }
    [[nodiscard]] Handle<Node> AddRoot();
    [[nodiscard]] Handle<Node> AddRoot(NodeStyle&& root);
    [[nodiscard]] Handle<Node> AddNode(Handle<Node> parent_handle);
    [[nodiscard]] Handle<Node> AddNode(NodeStyle&& node, Handle<Node> parent_handle);

    void Clear();
    constexpr void MarkDirty() noexcept { dirty = true; }
    constexpr void SetDisplay(const b8 value) noexcept { display = value; }

    [[nodiscard]] constexpr b8 GetDisplay() const noexcept { return display; }
};
} // namespace pce::ui
