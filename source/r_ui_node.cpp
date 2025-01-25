#include "r_ui_node.hpp"

#include <ranges>
#include <stack>

namespace pce::ui {
// Assume node is leaf with fixed or child_constraint
void NodeTree::RecalculateLayout() {
    List parents { &root };
    for (u32 i = 0U; i < parents.Size(); ++i) {
        Node* parent = parents[i];
        for (Node& child : parent->children) { (void)parents.EmplaceBack(&child); }
    }

    // hug bottom up
    auto reversedParents = parents | std::views::reverse;
    for (Node* parent : reversedParents) {
        const uint2 children_content_size = std::ranges::fold_left(parent->children | std::views::transform(&Node::OuterBoxSize), parent->NonContentSize() * 2U, std::plus { });
        if (parent->width.layout_type == LayoutLength::child_constraint) { parent->width.resolved = children_content_size.x; }
        if (parent->height.layout_type == LayoutLength::child_constraint) { parent->height.resolved = children_content_size.y; }
    }

    // fill top down
    for (Node* parent : parents) {
        // major axis
        List<Node*> parent_constrained { };
        uint2 pixels_taken { 0U, 0U };
        for (Node& child : parent->children) { if (child.width.layout_type == LayoutLength::parent_constraint) { (void)parent_constrained.EmplaceBack(&child); } else { pixels_taken.x += child.OuterBoxSize().x; } }
        u32 n_children = parent_constrained.Size();
        if (n_children > 0U) {
            if (pixels_taken.x >= parent->InnerBoxSize().x) {
                for (Node* child : parent_constrained) { child->width.resolved = 0U; }
                continue;
            }
            u32 pixels_per = (parent->InnerBoxSize().x - pixels_taken.x) / n_children;
            u32 left_over = (parent->InnerBoxSize().x - pixels_taken.x) % n_children;
            for (Node* child : parent_constrained) { child->width.resolved = pixels_per - child->NonContentSize().x * 2U; }
            parent_constrained[0U]->width.resolved += left_over;
        }

        // minor axis
        auto children = parent->children | std::views::filter([&] (const Node& child) -> bool { return child.height.layout_type == LayoutLength::parent_constraint; });
        for (Node& child : children) { child.height.resolved = parent->InnerBoxSize().y; }
    }

    // position top down
    for (Node* parent : parents) {
        uint2 position = parent->InnerBoxPosition();
        for (Node& child : parent->children) {
            child.position = position;
            // major axis
            position.x += child.OuterBoxSize().x;
        }
    }

    // bounding box
    for (Node* node : reversedParents) {
        uint2 start_position = node->OuterBoxPosition();
        uint2 end_position = node->OuterBoxEndPosition();
        for (const Node& child : node->children) {
            uint2 child_start_position = child.OuterBoxPosition();
            uint2 child_end_position = child.OuterBoxEndPosition();
            start_position.x = std::min(child_start_position.x, start_position.x);
            start_position.y = std::min(child_start_position.y, start_position.y);
            end_position.x = std::max(child_end_position.x, end_position.x);
            end_position.y = std::max(child_end_position.y, end_position.y);
        }
        uint2 size = start_position - end_position;
        node->bounding_box = { .x = static_cast<f32>(start_position.x), .y = static_cast<f32>(start_position.y), .w = static_cast<f32>(size.x), .h = static_cast<f32>(size.y) };
    }
}

constexpr b8 Node::IsInside(uint2 screen_position) const {
    uint2 start { static_cast<u32>(bounding_box.x), static_cast<u32>(bounding_box.y) };
    uint2 relative = screen_position - start;
    return relative.x < static_cast<u32>(bounding_box.w) && relative.y < static_cast<u32>(bounding_box.h);
}

FrameElements NodeTree::CreateFrameElements() {
    FrameElements elements;
    std::stack<const Node*> nodes;
    (void)nodes.emplace(&root);
    while (!nodes.empty()) {
        const Node* node = nodes.top();
        nodes.pop();
        RectangleElement rectangle { .color = node->background_color, .rect = node->OuterRect(), .on_click = nullptr };
        elements.rectangles.PushBack(rectangle);

        for (const Node& child : node->children) { (void)nodes.emplace(&child); }
    }

    return elements;
}

void RenderFrameElements(SDL_Renderer* renderer, NodeTree& node_tree) {
    const FrameElements& frame_elements = node_tree.GetFrameElements();
    for (const RectangleElement& element : frame_elements.rectangles) {
        (void)SDL_SetRenderDrawColor(renderer, element.color.r, element.color.g, element.color.b, element.color.a);
        (void)SDL_RenderFillRect(renderer, &element.rect);
    }
    for (const TextElement& text : frame_elements.texts) { (void)TTF_DrawRendererText(text.text, text.x, text.y); }
}

NodeBuilder::NodeBuilder(uint2 size) { Fixed(size); }
NodeBuilder::NodeBuilder(LayoutLength::RelatedConstraint constraint) {
    if (constraint == LayoutLength::hug) {
        HugWidth();
        HugHeight();
    } else {
        FillWidth();
        FillHeight();
    }
}
NodeBuilder::NodeBuilder(u32 width, LayoutLength::RelatedConstraint height_constraint) {
    FixedWidth(width);
    if (height_constraint == LayoutLength::hug) { HugHeight(); } else { FillHeight(); }
}
NodeBuilder::NodeBuilder(LayoutLength::RelatedConstraint width_constraint, u32 height) {
    if (width_constraint == LayoutLength::hug) { HugWidth(); } else { FillWidth(); }
    FixedHeight(height);
}
NodeBuilder::NodeBuilder(LayoutLength::RelatedConstraint width_constraint, LayoutLength::RelatedConstraint height_constraint) {
    if (width_constraint == LayoutLength::hug) { HugWidth(); } else { FillWidth(); }
    if (height_constraint == LayoutLength::hug) { HugHeight(); } else { FillHeight(); }
}

NodeBuilder& NodeBuilder::Fill(SDL_Color color) {
    node.background_color = color;
    return *this;
}
NodeBuilder& NodeBuilder::Absolute(uint2 pos) {
    node.position = pos;
    return *this;
}
NodeBuilder& NodeBuilder::Fixed(uint2 size) {
    node.width = { .resolved = size.x, .layout_type = LayoutLength::fixed };
    node.height = { .resolved = size.y, .layout_type = LayoutLength::fixed };
    return *this;
}
NodeBuilder& NodeBuilder::FixedWidth(u32 width) {
    node.width = { .resolved = width, .layout_type = LayoutLength::fixed };
    return *this;
}
NodeBuilder& NodeBuilder::FixedHeight(u32 height) {
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
NodeBuilder& NodeBuilder::Padding(uint2 padding) {
    node.padding = padding;
    return *this;
}
}
