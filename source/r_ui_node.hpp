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
enum class Fonts : FontSize { body = 16U, h1 = 34U, h2 = 30U, h3 = 24U, h4 = 20U, h5 = 18U, h6 = 16U, small = 14U, tiny = 12U, title = 52U };
class Font {
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
    FlatMap<Fonts, Font> fonts { };

public:
    explicit FontCollection(const AbsolutePath& path) : font_path { path } { }
    [[nodiscard]] const Font& GetFont(Fonts size);
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
struct Node : LogDestroyWithCount<Node> {
    struct Handle {
        u32 id;
        explicit constexpr Handle(const u32 value) noexcept : id(value) { }
    };
    struct OptionalHandle {
        [[nodiscard]] constexpr OptionalHandle() noexcept : id(U32_MAX) { }
        [[nodiscard]] constexpr explicit OptionalHandle(const u32 value) noexcept : id(value) { }
        [[nodiscard]] constexpr OptionalHandle(const Handle handle) noexcept : id(handle.id) { }
        [[nodiscard]] constexpr bool IsValid() const noexcept { return id != U32_MAX; }
        [[nodiscard]] constexpr Handle GetHandle() const noexcept { return Handle { id }; }
        u32 id;
    };

    String name { };
    String text { };
    Fonts font_size { Fonts::body };

    SDL_FRect bounding_box { };

    SDL_Color background_color { 0, 0, 0 };
    SDL_Color background_color_hover { 0, 0, 0 };

    uint2 position { U32_MAX, U32_MAX };
    uint4 padding { 0U, 0U, 0U, 0U };
    u32 gap { 0U };

    LayoutLength width { .resolved = 0U, .constraint = LayoutLength::child_constraint };
    LayoutLength height { .resolved = 0U, .constraint = LayoutLength::child_constraint };
    FlexDirection direction { horizontal };
    Alignment alignment { top_left };

    std::function<void(Node*)> on_click { };
    std::function<void(Node*)> on_hover { };
    std::function<void(Node*)> on_hover_out { };

    Node() = default;
    ~Node() { }

    [[nodiscard]] constexpr uint4 NonContentSize4() const { return padding; }
    [[nodiscard]] constexpr uint2 NonContentSize2() const { return { padding.x + padding.z, padding.y + padding.w }; }
    [[nodiscard]] constexpr uint2 OuterBoxPosition() const { return position; }
    [[nodiscard]] constexpr uint2 OuterBoxSize() const { return { width.resolved, height.resolved }; }
    [[nodiscard]] constexpr uint2 OuterBoxEndPosition() const { return OuterBoxPosition() + OuterBoxSize(); }
    [[nodiscard]] constexpr uint2 InnerBoxPosition() const { return OuterBoxPosition() + uint2 { NonContentSize4().x, NonContentSize4().y }; }
    [[nodiscard]] constexpr uint2 InnerBoxSize() const { return OuterBoxSize() - NonContentSize2(); }
    [[nodiscard]] constexpr b8 IsText() const { return !text.Empty(); }
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

    void OnHover() {
        std::swap(background_color, background_color_hover);
        if (on_hover) { on_hover(this); }
    }
    void OnHoverOut() {
        std::swap(background_color, background_color_hover);
        if (on_hover_out) { on_hover_out(this); }
    }
    void OnClick() { if (on_click) { on_click(this); } }
};
class NodeTree {
    static constexpr u32 default_count = 32U;
    List<Node> nodes { default_count };
    List<Node::Handle> parents { default_count };
    List<List<Node::Handle>> children { default_count };
    List<UniquePointer<TTF_Text, DestroyText>> texts { default_count };
    Node::Handle offset_handle { 0U };
    b8 display { true };

public:
    bool dirty { true };
    FrameElements frame_elements { };

    [[nodiscard]] constexpr Node::Handle Root() const { return offset_handle; }
    [[nodiscard]] constexpr Node::Handle Parent(const Node::Handle node_handle) { return parents[HandleToIndex(node_handle)]; }
    [[nodiscard]] constexpr const List<Node::Handle>& Children(const Node::Handle node_handle) const { return children[HandleToIndex(node_handle)]; }
    [[nodiscard]] constexpr List<Node::Handle>& Children(const Node::Handle node_handle) { return children[HandleToIndex(node_handle)]; }
    [[nodiscard]] constexpr UniquePointer<TTF_Text, DestroyText>& Text(const Node::Handle node_handle) { return texts[HandleToIndex(node_handle)]; }
    [[nodiscard]] constexpr const Node& GetNode(const Node::Handle node_handle) const { return nodes[HandleToIndex(node_handle)]; }
    [[nodiscard]] constexpr Node& GetNode(const Node::Handle node_handle) { return nodes[HandleToIndex(node_handle)]; }
    [[nodiscard]] constexpr b8 Empty() const { return nodes.Empty(); }
    [[nodiscard]] Node::Handle AddRoot();
    [[nodiscard]] Node::Handle AddRoot(Node&& root);
    [[nodiscard]] Node::Handle AddNode(Node::Handle parent_handle);
    [[nodiscard]] Node::Handle AddNode(Node&& node, Node::Handle parent_handle);

    void Clear();
    void Propagate(Node::Handle node_handle, const std::function<void(Node&)>& proj);
    constexpr void MarkDirty() noexcept { dirty = true; }
    constexpr void SetDisplay(const b8 value) noexcept { display = value; }

    [[nodiscard]] constexpr b8 GetDisplay() const noexcept { return display; }
    [[nodiscard]] constexpr b8 ValidHandle(const Node::Handle node_handle) const { return (node_handle.id - offset_handle.id) < nodes.Size(); }

private:
    [[nodiscard]] constexpr u32 HandleToIndex(Node::Handle node_handle) const;
};

struct NodeReference {
    std::reference_wrapper<NodeTree> tree;
    Node::Handle node_handle;
    [[nodiscard]] b8 operator==(const NodeReference other) const { return &tree.get() == &other.tree.get() && node_handle.id == other.node_handle.id; }
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
    Node& node { node_reference.tree.get().GetNode(node_reference.node_handle) };
public:
    NodeBuilder(NodeTree& node_tree, Layout layout, uint2 position);
    NodeBuilder(NodeTree& node_tree, Layout layout, Node::Handle parent_handle);
    [[nodiscard]] NodeBuilder& Name(const String& name);
    [[nodiscard]] NodeBuilder& Fill(SDL_Color color);
    [[nodiscard]] NodeBuilder& Padding(uint2 padding);
    [[nodiscard]] NodeBuilder& Padding4(uint4 padding);
    [[nodiscard]] NodeBuilder& Gap(u32 gap);
    [[nodiscard]] NodeBuilder& Direction(FlexDirection direction);
    [[nodiscard]] NodeBuilder& Text(const String& string);
    [[nodiscard]] NodeBuilder& Text(String&& string);
    [[nodiscard]] NodeBuilder& Text(const String& string, Fonts font_size);
    [[nodiscard]] NodeBuilder& Text(String&& string, Fonts font_size);
    [[nodiscard]] NodeBuilder& Alignment(Alignment alignment);
    [[nodiscard]] NodeBuilder& Right();
    [[nodiscard]] NodeBuilder& Center();
    [[nodiscard]] NodeBuilder& Left();
    Node::Handle Build();
};
using B = NodeBuilder;
} // pce::ui
