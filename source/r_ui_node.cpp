#include "r_ui_node.hpp"

#include <ranges>
#include <r_colors.hpp>
#include <stack>

namespace pce::ui {
NodeBuilder::NodeBuilder(const uint2 size) { Fixed(size); }
NodeBuilder::NodeBuilder(const RelatedConstraint constraint) {
    if (constraint == hug) {
        HugWidth();
        HugHeight();
    } else {
        FillWidth();
        FillHeight();
    }
}
NodeBuilder::NodeBuilder(const u32 width, const RelatedConstraint height_constraint) {
    FixedWidth(width);
    if (height_constraint == hug) { HugHeight(); } else { FillHeight(); }
}
NodeBuilder::NodeBuilder(const RelatedConstraint width_constraint, const u32 height) {
    if (width_constraint == hug) { HugWidth(); } else { FillWidth(); }
    FixedHeight(height);
}
NodeBuilder::NodeBuilder(const RelatedConstraint width_constraint, const RelatedConstraint height_constraint) {
    if (width_constraint == hug) { HugWidth(); } else { FillWidth(); }
    if (height_constraint == hug) { HugHeight(); } else { FillHeight(); }
}

NodeBuilder& NodeBuilder::Name(const String& name) {
    node.name = name;
    return *this;
}
NodeBuilder& NodeBuilder::Fill(const SDL_Color color) {
    node.background_color = color;
    return *this;
}
NodeBuilder& NodeBuilder::Fixed(const uint2 size) {
    node.width = { .resolved = size.x, .layout_type = LayoutLength::fixed };
    node.height = { .resolved = size.y, .layout_type = LayoutLength::fixed };
    return *this;
}
NodeBuilder& NodeBuilder::FixedWidth(const u32 width) {
    node.width = { .resolved = width, .layout_type = LayoutLength::fixed };
    return *this;
}
NodeBuilder& NodeBuilder::FixedHeight(const u32 height) {
    node.height = { .resolved = height, .layout_type = LayoutLength::fixed };
    return *this;
}
NodeBuilder& NodeBuilder::HugWidth() {
    node.width = { .resolved = -1U, .layout_type = LayoutLength::child_constraint };
    return *this;
}
NodeBuilder& NodeBuilder::HugHeight() {
    node.height = { .resolved = -1U, .layout_type = LayoutLength::child_constraint };
    return *this;
}
NodeBuilder& NodeBuilder::FillWidth() {
    node.width = { .resolved = -1U, .layout_type = LayoutLength::parent_constraint };
    return *this;
}
NodeBuilder& NodeBuilder::FillHeight() {
    node.height = { .resolved = -1U, .layout_type = LayoutLength::parent_constraint };
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
}
