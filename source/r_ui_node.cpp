#include "r_ui_node.hpp"

#include <ranges>
#include <r_colors.hpp>
#include <stack>

namespace pce::ui {

const Font& FontCollection::GetFont(const Fonts size) {
    if (!fonts.HasKey(size)) {
        fonts.EmplaceBack(size, Font { font_path, static_cast<FontSize>(size) });
        b8 failed = fonts[size].FailedLoading();
        if (failed) { SDL_Log("Font not loaded (%s)", SDL_GetError()); }
    }
    return fonts[size];
}

// NodeTree
constexpr u32 NodeTree::HandleToIndex(const Node::Handle node_handle) const {
    ASSERT_DBG(ValidHandle(node_handle), "Out of bounds, most likely destroyed");
    return node_handle.id - offset_handle.id;
}
Node::Handle NodeTree::SetRoot(Node&& root) {
    ASSERT_DBG(nodes.Empty(), "Setting root non empty tree");
    nodes.PushBack(root);
    parents.PushBack(Root());
    children.EmplaceBack();
    texts.EmplaceBack(nullptr);

    return Root();
}
Node::Handle NodeTree::AddNode(Node&& node, const Node::Handle parent_handle) {
    ASSERT_DBG(!nodes.Empty(), "Adding node without root");
    const Node::Handle node_handle { offset_handle.id + nodes.Size() };
    ASSERT_DBG(node_handle.id != parent_handle.id, "Assigning node to itself recursion");

    nodes.PushBack(node);
    parents.PushBack(parent_handle);
    children.EmplaceBack();
    texts.EmplaceBack(nullptr);

    Children(parent_handle).PushBack(node_handle);
    return node_handle;
}
void NodeTree::Clear() {
    offset_handle.id += nodes.Size();

    nodes.Clear();
    parents.Clear();
    children.Clear();
    texts.Clear();
}
void NodeTree::Propagate(Node::Handle node_handle, const std::function<void(Node&)>& proj) {
    while (true) {
        std::invoke(proj, GetNode(node_handle));
        if (node_handle.id == Root().id) { break; };
        node_handle = Parent(node_handle);
    }
}
[[nodiscard]] constexpr LayoutLength::Constraint ToConstraint(const RelativeConstraint related_constraint) {
    return related_constraint == hug ? LayoutLength::child_constraint : LayoutLength::parent_constraint;
}
NodeBuilder::NodeBuilder(const uint2 size) {
    node.width = { .resolved = size.x, .constraint = LayoutLength::fixed};
    node.height = { .resolved = size.y, .constraint = LayoutLength::fixed};
}
NodeBuilder::NodeBuilder(const RelativeConstraint relative_constraint) {
    LayoutLength::Constraint constraint = ToConstraint(relative_constraint);
    node.width = { .resolved = -1U, .constraint = constraint};
    node.height = { .resolved = -1U, .constraint = constraint};
}
NodeBuilder::NodeBuilder(const u32 width, const RelativeConstraint height_constraint) {
    node.width = { .resolved = width, .constraint = LayoutLength::fixed};
    node.height = { .resolved = -1U, .constraint = ToConstraint(height_constraint)};
}
NodeBuilder::NodeBuilder(const RelativeConstraint width_constraint, const u32 height) {
    node.width = { .resolved = -1U, .constraint = ToConstraint(width_constraint)};
    node.height = { .resolved = height, .constraint = LayoutLength::fixed};
}
NodeBuilder::NodeBuilder(const RelativeConstraint width_constraint, const RelativeConstraint height_constraint) {
    node.width = { .resolved = -1U, .constraint = ToConstraint(width_constraint)};
    node.height = { .resolved = -1U, .constraint = ToConstraint(height_constraint)};
}
NodeBuilder& NodeBuilder::Name(const String& name) {
    node.name = name;
    return *this;
}
NodeBuilder& NodeBuilder::Fill(const SDL_Color color) {
    node.background_color = color;
    return *this;
}
NodeBuilder& NodeBuilder::Padding(const uint2 padding) {
    node.padding = uint4 { padding.x, padding.y, padding.x, padding.y };
    return *this;
}
NodeBuilder& NodeBuilder::Padding4(const uint4 padding) {
    node.padding = padding;
    return *this;
}
NodeBuilder& NodeBuilder::Gap(const u32 gap) {
    node.gap = gap;
    return *this;
}
NodeBuilder& NodeBuilder::Direction(FlexDirection direction) {
    node.direction = direction;
    return *this;
}
constexpr SDL_Color DEFAULT_TEXT_COLOR = colors::black;
NodeBuilder& NodeBuilder::Text(const String& string) {
    if (node.background_color.a == 0U) { node.background_color = DEFAULT_TEXT_COLOR; }
    node.text = string;
    return *this;
}
NodeBuilder& NodeBuilder::Text(String&& string) {
    if (node.background_color.a == 0U) { node.background_color = DEFAULT_TEXT_COLOR; }
    node.text = string;
    return *this;
}
NodeBuilder& NodeBuilder::Text(const String& string, const Fonts font_size) {
    if (node.background_color.a == 0U) { node.background_color = DEFAULT_TEXT_COLOR; }
    node.text = string;
    node.font_size = font_size;
    return *this;
}
NodeBuilder& NodeBuilder::Text(String&& string, const Fonts font_size) {
    if (node.background_color.a == 0U) { node.background_color = DEFAULT_TEXT_COLOR; }
    node.text = string;
    node.font_size = font_size;
    return *this;
}
NodeBuilder& NodeBuilder::Alignment(ui::Alignment alignment) {
    node.alignment = alignment;
    return *this;
}
NodeBuilder& NodeBuilder::Right() { return Alignment(right); }
NodeBuilder& NodeBuilder::Center() { return Alignment(center); }
NodeBuilder& NodeBuilder::Left() { return Alignment(left); }
inline SDL_Color lighten_color(const SDL_Color color, const f32 factor) {
    auto lerp = [] (u8 channel, f32 factor, u8 target) -> u8 { return static_cast<u8>(channel + (target - channel) * factor); };
    return SDL_Color { lerp(color.r, factor, 255), lerp(color.g, factor, 255), lerp(color.b, factor, 255), color.a };
}
void NodeBuilder::Finalize(NodeTree& node_tree) {
    node_tree.MarkDirty();

    constexpr f32 factor = 0.5F;
    node.background_color_hover = lighten_color(node.background_color, factor);

    node.on_click = [&] (Node* node) -> void { Logger().Log("Clicked"); };
    node.on_hover = [&] (Node* node) -> void { Logger().Log("Hover"); };
    node.on_hover_out = [&] (Node* node) -> void { Logger().Log("On Hover Out"); };
}
Node::Handle NodeBuilder::BuildRoot(NodeTree& node_tree, const uint2 position) {
    Finalize(node_tree);
    node.position = position;
    return node_tree.SetRoot(std::move(node));
}
Node::Handle NodeBuilder::Build(NodeTree& node_tree, Node::Handle parent_handle) {
    Finalize(node_tree);
    return node_tree.AddNode(std::move(node), parent_handle);
}
}
