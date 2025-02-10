#pragma once

#include <stacktrace>
#include <algorithm>
#include <functional>

#include <SDL3/SDL_render.h>
#include <SDL3_ttf/SDL_ttf.h>

#include "u_collections.hpp"
#include "u_logger.hpp"
#include "u_types.hpp"

namespace pce::ui {
struct CloseFont {
    void operator()(TTF_Font* font) const {
        Logger().Destroyed("TTF_Font");
        TTF_CloseFont(font);
    }
};
struct DestroyText {
    void operator()(TTF_Text* text) const {
        Logger().Destroyed("TTF_Text");
        TTF_DestroyText(text);
    }
};
using FontSize = u8;
enum class FontSizes : FontSize { body = 16U, h1 = 34U, h2 = 30U, h3 = 24U, h4 = 20U, h5 = 18U, h6 = 16U, small = 14U, tiny = 12U, title = 52U, massive = 72U };
class Font : LogLifetimeWithCount<Font> {
    UniquePointer<TTF_Font, CloseFont> font;

public:
    Font(const AbsolutePath& path, const FontSize size) : font(TTF_OpenFont(path.string().c_str(), size)) { Logger().Created("Font {} {}", size, path.string()); }
    Font(const Font&) = delete;
    Font& operator=(const Font&) = delete;
    Font(Font&&) noexcept = default;
    Font& operator=(Font&&) noexcept = default;

    [[nodiscard]] b8 FailedLoading() const { return font.Get() == nullptr; }
    [[nodiscard]] constexpr TTF_Font *ToSDL() const { return font.Get(); }
    [[nodiscard]] FontSize GetSize() const { return TTF_GetFontSize(font.Get()); }
};
class FontCollection {
    AbsolutePath font_path;
    FlatMap<FontSizes, Font> fonts { 16U };

public:
    explicit FontCollection(const AbsolutePath& path) : font_path { path } { }
    [[nodiscard]] const Font& GetFont(FontSizes size);
};
struct TextElement {
    TTF_Text* text;
    float2 position;
};
struct RectangleElement {
    SDL_Color color;
    SDL_FRect rect;
};
struct FrameElements {
    List<RectangleElement> rectangles { };
    List<TextElement> texts { };
};
enum class TextAlign { left, center, right };
enum RelativeConstraint : u8 { hug, fill };
enum FlexDirection : u8 { horizontal, vertical };
enum Alignment : u8 { top_left, top_center, top_right, left, center, right, bottom_left, bottom_center, bottom_right };
struct LayoutLength {
    enum Constraint : u8 { child_constraint, parent_constraint, fixed };
    u32 resolved;
    Constraint constraint;
};
class NodeTree;
class NodeBuilder;
struct Node;
struct NodeStyle : LogDestroyWithCount<NodeStyle> {
    SDL_FRect bounding_box { };
    SDL_Color background_color { 0, 0, 0 };

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

    friend NodeBuilder;
    friend NodeTree;
};
struct NodeReference {
    NodeTree& tree;
    Handle<Node> node_handle;
};
struct WeakNodeReference {
    std::reference_wrapper<NodeTree> tree;
    Handle<Node> node_handle;
    [[nodiscard]] b8 operator==(const WeakNodeReference other) const { return &tree.get() == &other.tree.get() && node_handle.id == other.node_handle.id; }
};
using NodeReaction = std::function<void(NodeReference)>;
struct NodeProperties {
    String name { };
    String text { };
    FontSizes font_size { FontSizes::body };

    NodeReaction on_click { };
    NodeReaction on_hover { };
    NodeReaction on_hover_out { };
};
class NodeTree {
    static constexpr u32 DEFAULT_COUNT = 64U;
    List<NodeStyle> node_styles { DEFAULT_COUNT };
    List<NodeProperties> node_properties { DEFAULT_COUNT };
    std::vector<std::unique_ptr<TTF_Text, DestroyText>> node_ttf_texts { };
    List<Handle<Node>> parents { DEFAULT_COUNT };
    List<List<Handle<Node>>> children { DEFAULT_COUNT };

    Handle<Node> offset_handle { 0U };
    b8 display { true };

public:
    bool dirty { true };
    FrameElements frame_elements { };

    NodeTree() = default;
    NodeTree(const NodeTree&) = delete;
    NodeTree& operator=(const NodeTree&) = delete;
    NodeTree(NodeTree&&) noexcept = default;
    NodeTree& operator=(NodeTree&&) noexcept = default;

    [[nodiscard]] constexpr Handle<Node> Root() const { return offset_handle; }
    [[nodiscard]] constexpr Handle<Node> Parent(const Handle<Node> node_handle) { return parents[HandleToIndex(node_handle)]; }
    [[nodiscard]] constexpr const List<Handle<Node>>& Children(const Handle<Node> node_handle) const { return children[HandleToIndex(node_handle)]; }
    [[nodiscard]] constexpr List<Handle<Node>>& Children(const Handle<Node> node_handle) { return children[HandleToIndex(node_handle)]; }
    [[nodiscard]] constexpr const NodeStyle& GetStyle(const Handle<Node> node_handle) const { return node_styles[HandleToIndex(node_handle)]; }
    [[nodiscard]] constexpr NodeStyle& GetStyle(const Handle<Node> node_handle) { return node_styles[HandleToIndex(node_handle)]; }
    [[nodiscard]] constexpr const NodeProperties& GetProperties(const Handle<Node> node_handle) const { return node_properties[HandleToIndex(node_handle)]; }
    [[nodiscard]] constexpr NodeProperties& GetProperties(const Handle<Node> node_handle) { return node_properties[HandleToIndex(node_handle)]; }
    [[nodiscard]] constexpr const std::unique_ptr<TTF_Text, DestroyText>& GetTTFText(const Handle<Node> node_handle) const { return node_ttf_texts[HandleToIndex(node_handle)]; }
    [[nodiscard]] constexpr std::unique_ptr<TTF_Text, DestroyText>& GetTTFText(const Handle<Node> node_handle) { return node_ttf_texts[HandleToIndex(node_handle)]; }
    [[nodiscard]] constexpr b8 Empty() const { return node_styles.Empty(); }
    [[nodiscard]] Handle<Node> AddRoot();
    [[nodiscard]] Handle<Node> AddRoot(NodeStyle&& root);
    [[nodiscard]] Handle<Node> AddNode(Handle<Node> parent_handle);
    [[nodiscard]] Handle<Node> AddNode(NodeStyle&& node, Handle<Node> parent_handle);

    void Clear();
    void Propagate(Handle<Node> node_handle, const NodeReaction& reaction);
    constexpr void MarkDirty() noexcept { dirty = true; }
    constexpr void SetDisplay(const b8 value) noexcept { display = value; }

    [[nodiscard]] constexpr b8 GetDisplay() const noexcept { return display; }
    [[nodiscard]] constexpr b8 ValidHandle(const Handle<Node> node_handle) const { return (node_handle.id - offset_handle.id) < node_styles.Size(); }

private:
    [[nodiscard]] constexpr u32 HandleToIndex(Handle<Node> node_handle) const;
};
struct Layout {
    LayoutLength width;
    LayoutLength height;
    [[nodiscard]] constexpr LayoutLength::Constraint ToConstraint(const RelativeConstraint related_constraint) { return related_constraint == hug ? LayoutLength::child_constraint : LayoutLength::parent_constraint; }
    Layout(uint2 size) : width { size.x, LayoutLength::fixed }, height { size.y, LayoutLength::fixed } { }
    Layout(RelativeConstraint relative_constraint) : width { -1U, ToConstraint(relative_constraint) }, height { -1U, ToConstraint(relative_constraint) } { }
    Layout(u32 width, RelativeConstraint height_constraint) : width { width, LayoutLength::fixed }, height { -1U, ToConstraint(height_constraint) } { }
    Layout(RelativeConstraint width_constraint, u32 height) : width { -1U, ToConstraint(width_constraint) }, height { height, LayoutLength::fixed } { }
    Layout(RelativeConstraint width_constraint, RelativeConstraint height_constraint) : width { -1U, ToConstraint(width_constraint) }, height { -1U, ToConstraint(height_constraint) } { }
};
class NodeBuilder {
    NodeReference node_reference;
    NodeStyle& style { node_reference.tree.GetStyle(node_reference.node_handle) };
    NodeProperties& properties { node_reference.tree.GetProperties(node_reference.node_handle) };

public:
    NodeBuilder(NodeTree& node_tree, Layout new_layout, uint2 position);
    NodeBuilder(NodeTree& node_tree, Handle<Node> parent_handle, Layout new_layout);
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
SDL_Color LightenColor(SDL_Color color, f32 factor);

} // pce::ui
