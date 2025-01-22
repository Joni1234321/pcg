#include "r_ui_node.hpp"

#include <ranges>
#include <stack>

namespace pce::ui {
// Assume node is leaf with fixed or child_constraint
void NodeGenerator::RecalculateLayout(Node* root) {
    // Create tree
    List parents { root };
    // Possibly make this a cooler for loop
    for (u32 i = 0U; i < parents.Size(); ++i) {
        Node* parent = parents[i];
        for (Node& child : parent->children) { (void)parents.EmplaceBack(&child); }
    }

    // From bottom up create children constrained
    // Remember child_constraint only consist of fixed and child_constrained
    auto isChildConstrained = [&] (const Node* parent) -> bool { return parent->width.layout_type == LayoutLength::child_constraint; };
    auto bottomUpChildConstrainedParents = parents | std::views::reverse | std::ranges::views::filter(isChildConstrained);
    for (Node* parent : bottomUpChildConstrainedParents) {
        parent->width.resolved = std::accumulate(parent->children.begin(), parent->children.end(), 0U, [&] (const u32 sum, const Node& child) -> u32 { return sum + child.width.resolved; });
    }

    // From top down create parents constrained
    for (Node* parent : parents) {
        List<Node*> parent_constrained_children { };
        u32 pixels_taken = 0U;
        for (Node& child : parent->children) { if (child.width.layout_type == LayoutLength::parent_constraint) { (void)parent_constrained_children.EmplaceBack(&child); } else { pixels_taken += child.width.resolved; } }

        u32 n_children = parent_constrained_children.Size();
        if (n_children > 0U) {
            u32 pixels_per = 0U;
            u32 left_over = 0U;
            if (pixels_taken < parent->width.resolved) {
                pixels_per = (pixels_taken - parent->width.resolved) / n_children;
                left_over = (pixels_taken - parent->width.resolved) % n_children;
            }
            for (Node* child : parent_constrained_children) { child->width.resolved = pixels_per; }
            parent_constrained_children[0U]->width.resolved += left_over;
        }
    }
}
FrameElements NodeGenerator::CreateFrameElements(Node* root) {
    FrameElements elements;
    std::stack<const Node*> nodes;
    nodes.emplace(root);
    while (!nodes.empty()) {
        const Node* node = nodes.top();
        nodes.pop();
        const SDL_FRect rect { .x = static_cast<f32>(node->position.x), .y = static_cast<f32>(node->position.y), .w = static_cast<f32>(node->width.resolved), .h = static_cast<f32>(node->height.resolved) };
        const SDL_Color color { .r = node->background_color.r, .g = node->background_color.g, .b = node->background_color.b, .a = node->background_color.a };
        RectangleElement rectangle { .color = color, .rect = rect, .on_click = nullptr };
        elements.rectangles.PushBack(rectangle);

        for (const Node& child : node->children) { nodes.emplace(&child); }
    }

    return elements;
}

void RenderFrameElements(SDL_Renderer* renderer, FrameElements& frame_elements) {
    for (const RectangleElement& element : frame_elements.rectangles) {
        (void)SDL_SetRenderDrawColor(renderer, element.color.r, element.color.g, element.color.b, element.color.a);
        (void)SDL_RenderFillRect(renderer, &element.rect);
    }
    for (const TextElement& text : frame_elements.texts) { (void)TTF_DrawRendererText(text.text, text.x, text.y); }
}

void Node::SetWidth(LayoutLength new_width) {
    width = new_width;
    Node* root = this;
    while (root->width.layout_type != LayoutLength::fixed) { root = root->parent; } // NOLINT(*-id-dependent-backward-branch)
    NodeGenerator().RecalculateLayout(root);
}
NodeBuilder& NodeBuilder::Color(SDL_Color color) {
    node.background_color = color;
    return *this;
}
NodeBuilder& NodeBuilder::AbsolutePosition(uint2 pos) {
    node.position = pos;
    return *this;
}
NodeBuilder& NodeBuilder::Fixed(uint2 size) {
    node.width = { .resolved = size.x, .layout_type = LayoutLength::fixed };
    node.height = { .resolved = size.y, .layout_type = LayoutLength::fixed };
    return *this;
}
}
