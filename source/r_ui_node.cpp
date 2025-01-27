#include "r_ui_node.hpp"

#include <ranges>
#include <stack>

namespace pce::ui {
// Assume node is leaf with fixed or child_constraint
void NodeTree::RecalculateLayout() {
    auto pixels_gap = [this] (Node::Handle node_handle, u32 gap) -> u32 { return Children(node_handle).Empty() ? 0U : gap * (Children(node_handle).Size() - 1U); };
    List<Node::Handle> node_handles { Root() };
    for (u32 i = 0U; i < node_handles.Size(); ++i) { node_handles.AppendRange(Children(node_handles[i])); }

    TTF_Font* font = font_collection.large;
    for (const Node::Handle node_handle : node_handles) {
        Node& node = GetNode(node_handle);

        if (node.text.Empty()) {
            if (node.ttf_text != nullptr) {
                TTF_DestroyText(node.ttf_text);
                node.ttf_text = nullptr;
            }
            continue;
        }

        if (node.ttf_text == nullptr) { node.ttf_text = TTF_CreateText(text_engine, font, node.text.CString(), node.text.Size()); } else {
            TTF_SetTextString(node.ttf_text, node.text.CString(), node.text.Size());
            TTF_SetTextFont(node.ttf_text, font);
        }
        constexpr SDL_Color color = colors::black;
        (void)TTF_SetTextColor(node.ttf_text, color.r, color.g, color.b, color.a);
    }

    // hug bottom up
    auto reversed_nodes = node_handles | std::views::reverse;
    for (const Node::Handle node_handle : reversed_nodes) {
        Node& node = GetNode(node_handle);
        if (node.width.layout_type != LayoutLength::child_constraint && node.height.layout_type != LayoutLength::child_constraint) { continue; }
        auto children = Children(node_handle) | std::views::transform([this] (const Node::Handle handle) -> Node *{ return &GetNode(handle); });
        const uint2 children_outer_size = std::ranges::fold_left(children | std::views::transform(&Node::OuterBoxSize), node.NonContentSize() * 2U, std::plus { });

        uint2 text_size { 0U, 0U };
        if (node.ttf_text != nullptr) { (void)TTF_GetTextSize(node.ttf_text, reinterpret_cast<i32*>(&text_size.x), reinterpret_cast<i32*>(&text_size.y)); }
        // major
        if (node.width.layout_type == LayoutLength::child_constraint) { node.width.resolved = children_outer_size.x + pixels_gap(node_handle, node.gap) + text_size.x; }
        // minor
        if (node.height.layout_type == LayoutLength::child_constraint) { node.height.resolved = children_outer_size.y + text_size.y; }
    }

    // fill top down
    for (const Node::Handle node_handle : node_handles) {
        Node& node = GetNode(node_handle);

        // major axis
        List<Node::Handle> parent_constrained { };
        uint2 pixels_taken { pixels_gap(node_handle, node.gap), 0U };
        for (const Node::Handle child_handle : Children(node_handle)) {
            if (Node& child = GetNode(child_handle); child.width.layout_type == LayoutLength::parent_constraint) { parent_constrained.PushBack(child_handle); } else { pixels_taken.x += child.OuterBoxSize().x; }
        }
        if (parent_constrained.Size() > 0U) {
            if (pixels_taken.x >= node.InnerBoxSize().x) {
                for (const Node::Handle child_handle : parent_constrained) { GetNode(child_handle).width.resolved = 10U; }
                continue;
            }
            const auto [pixels_per, left_over] = math::Div(node.InnerBoxSize().x - pixels_taken.x, parent_constrained.Size());
            for (const Node::Handle child_handle : parent_constrained) {
                Node& child = GetNode(child_handle);
                child.width.resolved = pixels_per - child.NonContentSize().x * 2U;
            }
            GetNode(parent_constrained[0U]).width.resolved += left_over;
        }

        // minor axis
        auto children = Children(node_handle) | std::views::filter([this] (const Node::Handle child_handle) -> bool { return GetNode(child_handle).height.layout_type == LayoutLength::parent_constraint; });
        for (const Node::Handle child_handle : children) { GetNode(child_handle).height.resolved = node.InnerBoxSize().y; }
    }

    // position top down
    for (const Node::Handle parent_handle : node_handles) {
        const Node& parent = GetNode(parent_handle);
        uint2 position = parent.InnerBoxPosition();
        for (const Node::Handle child_handle : Children(parent_handle)) {
            Node& child = GetNode(child_handle);
            child.position = position;
            // major axis
            position.x += child.OuterBoxSize().x + parent.gap;
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

TextElement NodeTree::CreateTextAligned(const String& string, const SDL_Color color, f32 x, const f32 y, const TextAlign alignment, const u32 parent_width) const {
    TTF_Text* text = TTF_CreateText(text_engine, font_collection.small, string.CString(), string.size());
    (void)TTF_SetTextColor(text, color.r, color.g, color.b, color.a);
    i32 text_width;
    (void)TTF_GetTextSize(text, &text_width, nullptr);
    switch (alignment) {
        case TextAlign::left:
            break;
        case TextAlign::center:
            x += (parent_width - static_cast<f32>(text_width)) * 0.5F;
            break;
        case TextAlign::right:
            x += parent_width - static_cast<f32>(text_width);
            break;
    }
    return TextElement { .text = text, .x = x, .y = y };
}

FrameElements NodeTree::CreateFrameElements() {
    FrameElements elements;
    std::stack<Node::Handle> nodes;
    nodes.push(Root());
    while (!nodes.empty()) {
        const Node::Handle node_handle = nodes.top();
        const Node& node = GetNode(node_handle);
        nodes.pop();

        RectangleElement rectangle { .color = node.background_color, .rect = node.OuterRect(), .on_click = nullptr };
        elements.rectangles.PushBack(rectangle);
        if (node.ttf_text != nullptr) {
            TextElement text { .text = node.ttf_text, .x = rectangle.rect.x, .y = rectangle.rect.y };
            elements.texts.PushBack(text);
        }
        nodes.push_range(Children(node_handle));
    }

    return elements;
}
NodeBuilder::NodeBuilder(const uint2 size) { Fixed(size); }
NodeBuilder::NodeBuilder(const LayoutLength::RelatedConstraint constraint) {
    if (constraint == LayoutLength::hug) {
        HugWidth();
        HugHeight();
    } else {
        FillWidth();
        FillHeight();
    }
}
NodeBuilder::NodeBuilder(const u32 width, const LayoutLength::RelatedConstraint height_constraint) {
    FixedWidth(width);
    if (height_constraint == LayoutLength::hug) { HugHeight(); } else { FillHeight(); }
}
NodeBuilder::NodeBuilder(const LayoutLength::RelatedConstraint width_constraint, const u32 height) {
    if (width_constraint == LayoutLength::hug) { HugWidth(); } else { FillWidth(); }
    FixedHeight(height);
}
NodeBuilder::NodeBuilder(const LayoutLength::RelatedConstraint width_constraint, const LayoutLength::RelatedConstraint height_constraint) {
    if (width_constraint == LayoutLength::hug) { HugWidth(); } else { FillWidth(); }
    if (height_constraint == LayoutLength::hug) { HugHeight(); } else { FillHeight(); }
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
    node.padding = padding;
    return *this;
}
NodeBuilder& NodeBuilder::Gap(u32 gap) {
    node.gap = gap;
    return *this;
}
NodeBuilder& NodeBuilder::Text(const String& string) {
    node.text = string;
    return *this;
}
NodeBuilder& NodeBuilder::Text(String&& string) {
    node.text = string;
    return *this;
}
}
