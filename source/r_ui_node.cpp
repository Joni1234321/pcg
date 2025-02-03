#include "r_ui_node.hpp"

#include <ranges>
#include <r_colors.hpp>
#include <stack>

namespace pce::ui {
// Assume node is leaf with fixed or child_constraint
void NodeTree::RecalculateLayout() {
    if (Empty()) { return; }

    auto pixels_gap = [this] (const Node::Handle node_handle, const u32 gap) -> u32 { return Children(node_handle).Empty() ? 0U : gap * (Children(node_handle).Size() - 1U); };
    auto get_major = [] (const uint2 point, const FlexDirection direction) -> u32 { return direction == horizontal ? point.x : point.y; };
    auto get_minor = [] (const uint2 point, const FlexDirection direction) -> u32 { return direction == horizontal ? point.y : point.x; };
    auto get_major_layout = [] (Node& node, const FlexDirection direction) -> LayoutLength& { return direction == horizontal ? node.width : node.height; };
    auto get_minor_layout = [] (Node& node, const FlexDirection direction) -> LayoutLength& { return direction == horizontal ? node.height : node.width; };
    auto get_major_pixels_taken_by_children = [this, &get_major] (const Node::Handle node_handle, const FlexDirection direction) -> u32 {
        auto get_major_outer_box_size = [this, &get_major, &direction] (const Node::Handle child_handle) -> u32 { return get_major(GetNode(child_handle).OuterBoxSize(), direction); };
        return std::ranges::fold_left_first(Children(node_handle) | std::views::transform(get_major_outer_box_size), std::plus { }).value_or(0U);
    };
    auto get_minor_pixels_taken_by_children = [this, &get_minor] (const Node::Handle node_handle, const FlexDirection direction) -> u32 {
        auto get_minor_outer_box_size = [this, &get_minor, &direction] (const Node::Handle child_handle) -> u32 { return get_minor(GetNode(child_handle).OuterBoxSize(), direction); };
        return std::ranges::fold_left_first(Children(node_handle) | std::views::transform(get_minor_outer_box_size), std::plus { }).value_or(0U);
    };

    List<Node::Handle> node_handles { Root() };
    for (u32 i = 0U; i < node_handles.Size(); ++i) { node_handles.AppendRange(Children(node_handles[i])); }

    // text
    for (const Node::Handle node_handle : node_handles) {
        Node& node = GetNode(node_handle);

        if (!node.IsText()) {
            if (node.ttf_text != nullptr) {
                TTF_DestroyText(node.ttf_text);
                node.ttf_text = nullptr;
            }
            continue;
        }

        const Font& font = font_collection.GetFont(node.font_size);
        if (node.ttf_text == nullptr) { node.ttf_text = TTF_CreateText(text_engine, font.ToSDL(), node.text.CString(), node.text.Size()); } else {
            TTF_SetTextString(node.ttf_text, node.text.CString(), node.text.Size());
            TTF_SetTextFont(node.ttf_text, font.ToSDL());
        }
        const SDL_Color color = node.background_color;
        (void)TTF_SetTextColor(node.ttf_text, color.r, color.g, color.b, color.a);
    }

    // hug bottom up
    #define BREAKPOINT 1
    #if BREAKPOINT
    auto reversed_nodes = node_handles | std::views::reverse | std::ranges::to<std::vector>();
    #else
    auto reversed_nodes = node_handles | std::views::reverse;
    #endif
    for (const Node::Handle node_handle : reversed_nodes) {
        Node& node = GetNode(node_handle);
        if (node.width.layout_type != LayoutLength::child_constraint && node.height.layout_type != LayoutLength::child_constraint) { continue; }

        uint2 text_size { 0U, 0U };
        if (node.IsText()) { (void)TTF_GetTextSize(node.ttf_text, reinterpret_cast<i32*>(&text_size.x), reinterpret_cast<i32*>(&text_size.y)); }
        LayoutLength& major_layout = get_major_layout(node, node.direction);
        if (major_layout.layout_type == LayoutLength::child_constraint) {
            const u32 children_major = get_major_pixels_taken_by_children(node_handle, node.direction);
            major_layout.resolved = children_major + pixels_gap(node_handle, node.gap) + get_major(text_size, node.direction) + get_major(node.NonContentSize2(), node.direction);
        }
        LayoutLength& minor_layout = get_minor_layout(node, node.direction);
        if (minor_layout.layout_type == LayoutLength::child_constraint) {
            auto get_minor_outer_box_size = [this, get_minor, &node] (const Node::Handle child_handle) -> u32 { return get_minor(GetNode(child_handle).OuterBoxSize(), node.direction); };
            const u32 max_minor = Children(node_handle).Empty() ? 0U : std::ranges::max(Children(node_handle) | std::views::transform(get_minor_outer_box_size));
            minor_layout.resolved = std::max(max_minor, get_minor(text_size, node.direction)) + get_minor(node.NonContentSize2(), node.direction);
        }
    }

    // fill top down
    for (const Node::Handle node_handle : node_handles) {
        const Node& node = GetNode(node_handle);

        List<Node::Handle> parent_constrained { };
        u32 pixels_taken_major_axis = pixels_gap(node_handle, node.gap);
        for (const Node::Handle child_handle : Children(node_handle)) {
            Node& child = GetNode(child_handle);
            LayoutLength& child_major_layout = get_major_layout(child, node.direction);
            if (child_major_layout.layout_type == LayoutLength::parent_constraint) { parent_constrained.PushBack(child_handle); } else { pixels_taken_major_axis += get_major(child.OuterBoxSize(), node.direction); }
        }
        if (parent_constrained.Size() > 0U) {
            if (pixels_taken_major_axis >= get_major(node.InnerBoxSize(), node.direction)) {
                for (const Node::Handle child_handle : parent_constrained) {
                    constexpr u32 min_pixel_size = 10U;
                    get_major_layout(GetNode(child_handle), node.direction).resolved = min_pixel_size;
                }
                continue;
            }
            const auto [pixels_per, left_over] = math::Div(get_major(node.InnerBoxSize(), node.direction) - pixels_taken_major_axis, parent_constrained.Size());
            for (const Node::Handle child_handle : parent_constrained) { get_major_layout(GetNode(child_handle), node.direction).resolved = pixels_per; }
            get_major_layout(GetNode(parent_constrained[0U]), node.direction).resolved += left_over;
        }

        auto children = Children(node_handle) | std::views::filter([this, &node, &get_minor_layout] (const Node::Handle child_handle) -> bool {
            return get_minor_layout(GetNode(child_handle), node.direction).layout_type == LayoutLength::parent_constraint;
        });
        for (const Node::Handle child_handle : children) { get_minor_layout(GetNode(child_handle), node.direction).resolved = get_minor(node.InnerBoxSize(), node.direction); }
    }

    // position top down
    for (const Node::Handle node_handle : node_handles) {
        const Node& node = GetNode(node_handle);
        u32 major_position = get_major(node.InnerBoxPosition(), node.direction);

        const u32 children_major = get_major_pixels_taken_by_children(node_handle, node.direction);
        for (const Node::Handle child_handle : Children(node_handle)) {
            Node& child = GetNode(child_handle);

            if (node.direction == horizontal) {
                u32 minor_position = node.InnerBoxPosition().y;
                switch (node.alignment) {
                    case top_left:
                    case top_center:
                    case top_right:
                        break;
                    case left:
                    case center:
                    case right:
                        minor_position += (node.InnerBoxSize().y - child.OuterBoxSize().y) / 2U;
                        break;
                    case bottom_left:
                    case bottom_center:
                    case bottom_right:
                        minor_position += node.InnerBoxSize().y - child.OuterBoxSize().y;
                        break;
                }
                child.position = uint2 { major_position, minor_position };
                major_position += child.OuterBoxSize().x + node.gap;
            } else {
                u32 minor_position = node.InnerBoxPosition().x;
                switch (node.alignment) {
                    case top_left:
                    case left:
                    case bottom_left:
                        break;
                    case top_center:
                    case center:
                    case bottom_center:
                        minor_position += (node.InnerBoxSize().x - child.OuterBoxSize().x) / 2U;
                        break;
                    case top_right:
                    case right:
                    case bottom_right:
                        minor_position += node.InnerBoxSize().x - child.OuterBoxSize().x;
                        break;
                }
                child.position = uint2 { minor_position, major_position };
                major_position += child.OuterBoxSize().y + node.gap;
            }
        }
    }

    // bounding box
    for (const Node::Handle node_handle : reversed_nodes) {
        Node& node = GetNode(node_handle);
        uint2 start_position = node.OuterBoxPosition();
        uint2 end_position = node.OuterBoxEndPosition();
        for (const Node::Handle child_handle : Children(node_handle)) {
            Node& child = GetNode(child_handle);
            uint2 child_start_position = child.OuterBoxPosition();
            uint2 child_end_position = child.OuterBoxEndPosition();
            start_position.x = std::min(child_start_position.x, start_position.x);
            start_position.y = std::min(child_start_position.y, start_position.y);
            end_position.x = std::max(child_end_position.x, end_position.x);
            end_position.y = std::max(child_end_position.y, end_position.y);
        }
        const uint2 size = end_position - start_position;
        node.bounding_box = { .x = static_cast<f32>(start_position.x), .y = static_cast<f32>(start_position.y), .w = static_cast<f32>(size.x), .h = static_cast<f32>(size.y) };
    }
}
FrameElements NodeTree::CreateFrameElements() {
    FrameElements elements;
    if (Empty()) { return elements; }
    Stack<Node::Handle> nodes;
    nodes.push(Root());
    while (!nodes.empty()) {
        const Node::Handle node_handle = nodes.top();
        const Node& node = GetNode(node_handle);
        nodes.pop();
        nodes.push_range(Children(node_handle));

        if (node.IsText()) {
            TextElement text { .text = node.ttf_text, .position = float2 { static_cast<f32>(node.InnerBoxPosition().x), static_cast<f32>(node.InnerBoxPosition().y) } };
            elements.texts.PushBack(text);
        } else {
            RectangleElement rectangle { .color = node.background_color, .rect = node.OuterRect() };
            elements.rectangles.PushBack(rectangle);
        }
    }
    return elements;
}
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
