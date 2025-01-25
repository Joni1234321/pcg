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

    auto reversedParents = parents | std::views::reverse;
    for (Node* parent : reversedParents) {
        const uint2 children_content_size = std::ranges::fold_left(parent->children, parent->padding * 2U, [&] (const uint2 sum, const Node& child) -> uint2 { return sum + child.OuterBoxSize(); });
        if (parent->width.layout_type == LayoutLength::child_constraint) { parent->width.resolved = children_content_size.x; }
        if (parent->height.layout_type == LayoutLength::child_constraint) { parent->height.resolved = children_content_size.y; }
    }

    // From top down create parents constrained (fill)
    for (Node* parent : parents) {
        List<Node*> parent_constrained { };
        u32 pixels_taken = 0U;
        for (Node& child : parent->children) { if (child.width.layout_type == LayoutLength::parent_constraint) { (void)parent_constrained.EmplaceBack(&child); } else { pixels_taken += child.OuterBoxSize().x; } }

        u32 n_children = parent_constrained.Size();
        if (n_children > 0U) {
            u32 pixels_per = 0U;
            u32 left_over = 0U;
            if (pixels_taken < parent->InnerBoxSize().x) {
                pixels_per = (parent->InnerBoxSize().x - pixels_taken) / n_children;
                left_over = (parent->InnerBoxSize().x - pixels_taken) % n_children;
            }
            for (Node* child : parent_constrained) { child->width.resolved = pixels_per - child->padding.x * 2U; }
            parent_constrained[0U]->width.resolved += left_over;
        }
    }

    // From top down position
    for (Node* parent : parents) {
        uint2 position = parent->InnerBoxPosition();
        for (Node& child : parent->children) {
            child.position = position;
            position.x += child.OuterBoxSize().x;
        }
    }
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
