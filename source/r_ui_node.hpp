#pragma once

#include <algorithm>
#include <functional>

#include <SDL3/SDL_render.h>
#include <SDL3_ttf/SDL_ttf.h>

#include "u_collections.hpp"
#include "u_logger.hpp"
#include "u_types.hpp"

namespace pce::ui {
using FontSize = u8;
enum class Fonts : FontSize { body = 16U, h1 = 34U, h2 = 30U, h3 = 24U, h4 = 20U, h5 = 18U, h6 = 16U, small = 14U, tiny = 12U, title = 52U };
class Font {
    std::unique_ptr<TTF_Font, decltype(&TTF_CloseFont)> font;
public:
    Font(const AbsolutePath& path, const FontSize size) : font(TTF_OpenFont(path.string().c_str(), size), &TTF_CloseFont) { Logger().Log("Loading Font {} {}", size, path.string()); }
    Font(const Font&) = delete;
    Font& operator=(const Font&) = delete;
    Font(Font&&) noexcept = default;
    Font& operator=(Font&&) noexcept = default;

    b8 FailedLoading() const { return font.get() == nullptr; }
    constexpr TTF_Font *ToSDL() const { return font.get(); }
    FontSize GetSize() const { return TTF_GetFontSize(font.get()); }
};
struct FontCollection {
    const AbsolutePath font_path;

    explicit FontCollection(const AbsolutePath& path) : font_path { path } { }

    FlatMap<Fonts, Font> fonts { };
    [[nodiscard]] const Font& GetFont(const Fonts size) {
        if (!fonts.HasKey(size)) {
            fonts.EmplaceBack(size, Font { font_path, static_cast<FontSize>(size) });
            b8 failed = fonts[size].FailedLoading();
            if (failed) { SDL_Log("Font not loaded (%s)", SDL_GetError()); }
        }
        return fonts[size];
    }
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
inline SDL_Color lighten_color(const SDL_Color color, const f32 factor) {
    auto lerp = [] (u8 channel, f32 factor, u8 target) -> u8 { return static_cast<u8>(channel + (target - channel) * factor); };
    return SDL_Color { lerp(color.r, factor, 255), lerp(color.g, factor, 255), lerp(color.b, factor, 255), color.a };
}

enum class TextAlign { left, center, right };
enum RelatedConstraint : u8 { hug, fill };
enum FlexDirection : u8 { horizontal, vertical }; // default based on max width or height
enum Alignment : u8 {  top_left, top_center, top_right, left, center, right, bottom_left, bottom_center, bottom_right };
struct LayoutLength {
    enum Constraint : u8 { child_constraint, parent_constraint, fixed };
    u32 resolved;
    Constraint layout_type;
};

struct NodeTree;
struct NodeBuilder;

struct Node {
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
    Node() = default;
    ~Node() {
        if (ttf_text != nullptr) {
            Logger().Log("[Destroy] node text {}", text);
            TTF_DestroyText(ttf_text);
        }
    }

    String name { };
    String text { };
    TTF_Text* ttf_text { nullptr };
    Fonts font_size { Fonts::body };

    SDL_FRect bounding_box { };

    SDL_Color background_color { 0, 0, 0 };
    SDL_Color background_color_hover { 0, 0, 0 };

    uint2 position { U32_MAX, U32_MAX };
    uint4 padding { 0U, 0U, 0U, 0U };
    u32 gap { 0U };

    LayoutLength width { .resolved = 0U, .layout_type = LayoutLength::child_constraint };
    LayoutLength height { .resolved = 0U, .layout_type = LayoutLength::child_constraint };
    FlexDirection direction { horizontal };
    Alignment alignment { top_left };

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
};
struct NodeTree {
    [[nodiscard]] constexpr Node::Handle Root() const { return offset_handle; }
    Node::Handle SetRoot(const Node& root) {
        ASSERT_DBG(nodes.Empty(), "Setting root non empty tree");
        nodes.PushBack(root);
        parents.PushBack(Root());
        children.EmplaceBack();

        return Root();
    }

    [[nodiscard]] const Node& GetNode(const Node::Handle node_handle) const { return nodes[HandleToIndex(node_handle)]; }
    [[nodiscard]] constexpr Node& GetNode(const Node::Handle node_handle) { return nodes[HandleToIndex(node_handle)]; }
    [[nodiscard]] Node::Handle Parent(const Node::Handle node_handle) { return parents[HandleToIndex(node_handle)]; }
    [[nodiscard]] const List<Node::Handle>& Children(const Node::Handle node_handle) const { return children[HandleToIndex(node_handle)]; }
    [[nodiscard]] List<Node::Handle>& Children(const Node::Handle node_handle) { return children[HandleToIndex(node_handle)]; }
    [[nodiscard]] b8 Empty() const { return nodes.Empty(); }
    [[nodiscard]] Node::Handle AddNode(const Node& node, const Node::Handle parent_handle) {
        ASSERT_DBG(!nodes.Empty(), "Adding node without root");
        const Node::Handle node_handle { offset_handle.id + nodes.Size() };
        ASSERT_DBG(node_handle.id != parent_handle.id, "Assigning node to itself recursion");

        nodes.PushBack(node);
        parents.PushBack(parent_handle);
        children.EmplaceBack();

        Children(parent_handle).PushBack(node_handle);
        return node_handle;
    }
    void Clear() {
        offset_handle.id += nodes.Size();

        nodes.Clear();
        parents.Clear();
        children.Clear();
    };
    void Propagate(Node::Handle node_handle, const std::function<void(Node&)>& proj) {
        while (true) {
            std::invoke(proj, GetNode(node_handle));
            if (node_handle.id == Root().id) { break; };
            node_handle = Parent(node_handle);
        }
    }
    constexpr void SetDisplay(const b8 value) noexcept { display = value; }
    [[nodiscard]] constexpr b8 GetDisplay() const noexcept { return display; }
    constexpr void MarkDirty() noexcept { dirty = true; }
    Node::OptionalHandle HitNode(uint2 screen_position) const {
        if (nodes.Empty() || !GetNode(Root()).IsInside(screen_position)) { return Node::OptionalHandle { }; }
        Node::Handle node_handle = Root();
        const auto is_inside_node = [screen_position, this] (const Node::Handle child_handle) -> b8 { return this->GetNode(child_handle).IsInside(screen_position); };
        while (true) {
            auto node_iterator = std::ranges::find_if(Children(node_handle), is_inside_node, std::identity { });
            if (node_iterator == Children(node_handle).end()) { return GetNode(node_handle).background_color.a == 0 ? Node::OptionalHandle { } : Node::OptionalHandle { node_handle.id }; }
            node_handle = *node_iterator;
        }
    }
    [[nodiscard]] constexpr b8 ValidHandle(const Node::Handle node_handle) const { return (node_handle.id - offset_handle.id) < nodes.Size(); }
    bool dirty { true };
    FrameElements frame_elements { };

private:
    List<Node> nodes { };
    List<Node::Handle> parents { };
    List<List<Node::Handle>> children { };
    Node::Handle offset_handle { 0U };
    b8 display { true };

    [[nodiscard]] constexpr u32 HandleToIndex(const Node::Handle node_handle) const {
        ASSERT_DBG(ValidHandle(node_handle), "Out of bounds, most likely destroyed");
        return node_handle.id - offset_handle.id;
    }
};
struct NodeReference {
    std::reference_wrapper<NodeTree> tree;
    Node::Handle node_handle;
    [[nodiscard]] b8 operator==(const NodeReference other) const { return &tree.get() == &other.tree.get() && node_handle.id == other.node_handle.id; }
};
struct NodeBuilder {
    [[nodiscard]] explicit NodeBuilder(uint2 size);
    [[nodiscard]] explicit NodeBuilder(RelatedConstraint constraint);
    [[nodiscard]] NodeBuilder(u32 width, RelatedConstraint height_constraint);
    [[nodiscard]] NodeBuilder(RelatedConstraint width_constraint, u32 height);
    [[nodiscard]] NodeBuilder(RelatedConstraint width_constraint, RelatedConstraint height_constraint);
    [[nodiscard]] NodeBuilder& Name(const String& name);
    [[nodiscard]] NodeBuilder& Fill(SDL_Color color);
    [[nodiscard]] NodeBuilder& Layout(LayoutLength width, LayoutLength height);
    NodeBuilder& Fixed(uint2 size);
    NodeBuilder& FixedWidth(u32 width);
    NodeBuilder& FixedHeight(u32 height);
    NodeBuilder& HugWidth();
    NodeBuilder& HugHeight();
    NodeBuilder& FillWidth();
    NodeBuilder& FillHeight();
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
    void Finalize(NodeTree& node_tree) {
        node_tree.MarkDirty();

        constexpr f32 factor = 0.5F;
        node.background_color_hover = lighten_color(node.background_color, factor);

        node.on_click = [&] (Node* node) -> void { Logger().Log("Clicked"); };
        node.on_hover = [&] (Node* node) -> void { Logger().Log("Hover"); };
        node.on_hover_out = [&] (Node* node) -> void { Logger().Log("On Hover Out"); };
    }
    Node::Handle BuildRoot(NodeTree& node_tree, const uint2 position) {
        Finalize(node_tree);
        node.position = position;
        return node_tree.SetRoot(node);
    };
    Node::Handle Build(NodeTree& node_tree, Node::Handle parent_handle) {
        Finalize(node_tree);
        return node_tree.AddNode(node, parent_handle);
    }


private:
    Node node { };
};
} // pce::ui
