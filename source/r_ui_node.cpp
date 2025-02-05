#include "r_ui_node.hpp"

#include <ranges>
#include <r_colors.hpp>
#include <stack>

namespace pce::ui {
const Font& FontCollection::GetFont(const FontSizes size) {
    if (!fonts.HasKey(size)) {
        fonts.EmplaceBack(size, Font { font_path, static_cast<FontSize>(size) });
        b8 failed = fonts[size].FailedLoading();
        if (failed) { SDL_Log("Font not loaded (%s)", SDL_GetError()); }
    }
    return fonts[size];
}

// NodeTree
constexpr u32 NodeTree::HandleToIndex(const NodeStyle::NodeHandle node_handle) const {
    ASSERT_DBG(ValidHandle(node_handle), "Out of bounds, most likely destroyed");
    return node_handle.id - offset_handle.id;
}
NodeStyle::NodeHandle NodeTree::AddRoot() {
    ASSERT_DBG(nodes.Empty(), "Setting root non empty tree");
    nodes.EmplaceBack();
    parents.EmplaceBack(Root());
    children.EmplaceBack();
    node_properties.EmplaceBack();

    return Root();
}
NodeStyle::NodeHandle NodeTree::AddRoot(NodeStyle&& root) {
    const NodeStyle::NodeHandle node_handle = AddRoot();
    nodes[HandleToIndex(node_handle)] = std::move(root);
    return node_handle;
}
NodeStyle::NodeHandle NodeTree::AddNode(NodeStyle::NodeHandle parent_handle) {
    ASSERT_DBG(!nodes.Empty(), "Adding node without root");
    const NodeStyle::NodeHandle node_handle { offset_handle.id + nodes.Size() };
    ASSERT_DBG(node_handle.id != parent_handle.id, "Assigning node to itself recursion");
    nodes.EmplaceBack();
    parents.PushBack(parent_handle);
    children.EmplaceBack();
    node_properties.EmplaceBack();

    Children(parent_handle).PushBack(node_handle);
    return node_handle;
}
NodeStyle::NodeHandle NodeTree::AddNode(NodeStyle&& node, const NodeStyle::NodeHandle parent_handle) {
    const NodeStyle::NodeHandle node_handle = AddNode(parent_handle);
    nodes[HandleToIndex(node_handle)] = std::move(node);
    return node_handle;
}
void NodeTree::Clear() {
    offset_handle.id += nodes.Size();

    nodes.Clear();
    parents.Clear();
    children.Clear();
    node_properties.Clear();
}
void NodeTree::Propagate(NodeStyle::NodeHandle node_handle, const NodeReaction& reaction) {
    while (true) {
        std::invoke(reaction, NodeReference { *this, node_handle });
        if (node_handle.id == Root().id) { break; };
        node_handle = Parent(node_handle);
    }
}
NodeBuilder::NodeBuilder(NodeTree &node_tree, Layout new_layout, uint2 position) : node_reference{ node_tree, node_tree.AddRoot() } {
    style.position = position;
    style.width = new_layout.width;
    style.height = new_layout.height;
}
NodeBuilder::NodeBuilder(NodeTree &node_tree, NodeStyle::NodeHandle parent_handle, Layout new_layout) : node_reference{ node_tree, node_tree.AddNode(parent_handle) } {
    style.width = new_layout.width;
    style.height = new_layout.height;
}
NodeBuilder &NodeBuilder::Name(const String &name) {
    properties.name = name;
    return *this;
}
NodeBuilder &NodeBuilder::Fill(const SDL_Color color) {
    style.background_color = color;
    return *this;
}
NodeBuilder &NodeBuilder::Padding(const u32 padding) {
    style.padding = uint4 { padding, padding, padding, padding };
    return *this;
}
NodeBuilder &NodeBuilder::Padding2(const uint2 padding) {
    style.padding = uint4 { padding.x, padding.y, padding.x, padding.y };
    return *this;
}
NodeBuilder &NodeBuilder::Padding4(const uint4 padding) {
    style.padding = padding;
    return *this;
}
NodeBuilder &NodeBuilder::Gap(const u32 gap) {
    style.gap = gap;
    return *this;
}
NodeBuilder &NodeBuilder::Direction(FlexDirection direction) {
    style.direction = direction;
    return *this;
}
constexpr SDL_Color DEFAULT_TEXT_COLOR = colors::black;
NodeBuilder &NodeBuilder::Text(const String &string) {
    if (style.background_color.a == 0U) { style.background_color = DEFAULT_TEXT_COLOR; }
    properties.text = string;
    return *this;
}
NodeBuilder &NodeBuilder::Text(String &&string) {
    if (style.background_color.a == 0U) { style.background_color = DEFAULT_TEXT_COLOR; }
    properties.text = string;
    return *this;
}
NodeBuilder &NodeBuilder::Text(const String &string, const FontSizes font_size) {
    if (style.background_color.a == 0U) { style.background_color = DEFAULT_TEXT_COLOR; }
    properties.text = string;
    properties.font_size = font_size;
    return *this;
}
NodeBuilder &NodeBuilder::Text(String &&string, const FontSizes font_size) {
    if (style.background_color.a == 0U) { style.background_color = DEFAULT_TEXT_COLOR; }
    properties.text = string;
    properties.font_size = font_size;
    return *this;
}
NodeBuilder &NodeBuilder::Alignment(ui::Alignment alignment) {
    style.alignment = alignment;
    return *this;
}
NodeBuilder &NodeBuilder::Right() { return Alignment(right); }
NodeBuilder &NodeBuilder::Center() { return Alignment(center); }
NodeBuilder &NodeBuilder::Left() { return Alignment(left); }
inline SDL_Color lighten_color(const SDL_Color color, const f32 factor) {
    auto lerp = [] (u8 channel, f32 factor, u8 target) -> u8 { return static_cast<u8>(channel + (target - channel) * factor); };
    return SDL_Color { lerp(color.r, factor, 255), lerp(color.g, factor, 255), lerp(color.b, factor, 255), color.a };
}
NodeStyle::NodeHandle NodeBuilder::Build() {
    node_reference.tree.MarkDirty();
    constexpr f32 factor = 0.5F;
    style.background_color_hover = lighten_color(style.background_color, factor);
    return node_reference.node_handle;
}
}
