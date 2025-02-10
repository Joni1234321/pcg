#include "r_ui_node.hpp"

#include "u_types.hpp"

#include <ranges>
#include <r_colors.hpp>
#include <stack>

namespace pce::ui {
const Font& FontCollection::GetFont(const FontSizes size) {
    if (!fonts.HasKey(size)) {
        fonts.EmplaceBack(size, font_path, static_cast<FontSize>(size));
        b8 failed = fonts[size].FailedLoading();
        if (failed) { SDL_Log("Font not loaded (%s)", SDL_GetError()); }
    }
    return fonts[size];
}

// NodeTree
constexpr u32 NodeTree::HandleToIndex(const Handle<NodeTree> node_handle) const {
    ASSERT_DBG(ValidHandle(node_handle), "Out of bounds, most likely destroyed");
    return node_handle.id - offset_handle.id;
}
Handle<NodeTree> NodeTree::AddRoot() {
    ASSERT_DBG(node_styles.Empty(), "Setting root non empty tree");
    node_styles.EmplaceBack();
    parents.EmplaceBack(Root());
    children.EmplaceBack();
    node_properties.EmplaceBack();
    node_ttf_texts.emplace_back();

    return Root();
}
Handle<NodeTree> NodeTree::AddRoot(NodeStyle&& root) {
    const Handle<NodeTree> node_handle = AddRoot();
    node_styles[HandleToIndex(node_handle)] = std::move(root);
    return node_handle;
}
Handle<NodeTree> NodeTree::AddNode(const Handle<NodeTree> parent_handle) {
    ASSERT_DBG(!node_styles.Empty(), "Adding node without root");
    const Handle<NodeTree> node_handle { offset_handle.id + node_styles.Size() };
    ASSERT_DBG(node_handle.id != parent_handle.id, "Assigning node to itself recursion");
    node_styles.EmplaceBack();
    parents.PushBack(parent_handle);
    children.EmplaceBack();
    node_properties.EmplaceBack();
    node_ttf_texts.emplace_back();

    Children(parent_handle).PushBack(node_handle);
    return node_handle;
}
Handle<NodeTree> NodeTree::AddNode(NodeStyle&& node, const Handle<NodeTree> parent_handle) {
    const Handle<NodeTree> node_handle = AddNode(parent_handle);
    node_styles[HandleToIndex(node_handle)] = std::move(node);
    return node_handle;
}
void NodeTree::Clear() {
    offset_handle.id += node_styles.Size();

    node_styles.Clear();
    parents.Clear();
    children.Clear();
    node_properties.Clear();
    node_ttf_texts.clear();
}
void NodeTree::Propagate(Handle<NodeTree> node_handle, const NodeReaction& reaction) {
    while (true) {
        std::invoke(reaction, NodeReference { *this, node_handle });
        if (node_handle.id == Root().id) { break; };
        node_handle = Parent(node_handle);
    }
}
NodeBuilder::NodeBuilder(NodeTree &node_tree, const Layout new_layout, const uint2 position) : node_reference{ node_tree, node_tree.AddRoot() } {
    style.position = position;
    style.width = new_layout.width;
    style.height = new_layout.height;
}
NodeBuilder::NodeBuilder(NodeTree &node_tree, const Handle<NodeTree> parent_handle, const Layout new_layout) : node_reference{ node_tree, node_tree.AddNode(parent_handle) } {
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
    style.resolved_gap = gap;
    style.gap_auto = false;
    return *this;
}
NodeBuilder& NodeBuilder::GapAuto() {
    style.resolved_gap = 0U;
    style.gap_auto = true;
    return *this;
}
NodeBuilder &NodeBuilder::Direction(const FlexDirection direction) {
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
NodeBuilder &NodeBuilder::Alignment(const ui::Alignment alignment) {
    style.alignment = alignment;
    return *this;
}
NodeBuilder &NodeBuilder::Right() { return Alignment(right); }
NodeBuilder &NodeBuilder::Center() { return Alignment(center); }
NodeBuilder &NodeBuilder::Left() { return Alignment(left); }
SDL_Color LightenColor(const SDL_Color color, const f32 factor) {
    auto lerp = [] (const u8 channel, const f32 factor, const u8 target) -> u8 { return static_cast<u8>(channel + (target - channel) * factor); };
    return SDL_Color { lerp(color.r, factor, 255), lerp(color.g, factor, 255), lerp(color.b, factor, 255), color.a };
}
Handle<NodeTree> NodeBuilder::Build() const {
    node_reference.tree.MarkDirty();
    return node_reference.node_handle;
}
}
