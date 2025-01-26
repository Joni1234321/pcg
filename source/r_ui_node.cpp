#include "r_ui_node.hpp"

#include <ranges>
#include <stack>

namespace pce::ui {
// Assume node is leaf with fixed or child_constraint
void NodeTree::RecalculateLayout() {
    auto handle_to_node = handle_to_node_generator();
    List<Node*> parents { handle_to_node(Root()) };
    for (u32 i = 0U; i < parents.Size(); ++i) {
        auto children = parents[i]->children | std::views::transform(handle_to_node);
        for (Node* child : children) { parents.PushBack(child); }
    }

    // hug bottom up
    auto reversedParents = parents | std::views::reverse;
    for (Node* parent : reversedParents) {
        auto children = parent->children | std::views::transform(handle_to_node);
        const uint2 children_content_size = std::ranges::fold_left(children | std::views::transform(&Node::OuterBoxSize), parent->NonContentSize() * 2U, std::plus { });
        if (parent->width.layout_type == LayoutLength::child_constraint) { parent->width.resolved = children_content_size.x; }
        if (parent->height.layout_type == LayoutLength::child_constraint) { parent->height.resolved = children_content_size.y; }
    }

    // fill top down
    for (Node* parent : parents) {
        // major axis
        List<Node::Handle> parent_constrained { };
        uint2 pixels_taken { 0U, 0U };

        for (const Node::Handle child_handle : parent->children) {
            Node& child = GetNode(child_handle);
            if (child.width.layout_type == LayoutLength::parent_constraint) { parent_constrained.PushBack(child_handle); } else { pixels_taken.x += child.OuterBoxSize().x; }
        }
        u32 n_children = parent_constrained.Size();
        if (n_children > 0U) {
            if (pixels_taken.x >= parent->InnerBoxSize().x) {
                for (const Node::Handle child_handle : parent_constrained) { GetNode(child_handle).width.resolved = 0U; }
                continue;
            }
            const auto [pixels_per, left_over] = math::Div(parent->InnerBoxSize().x - pixels_taken.x, n_children);
            for (const Node::Handle child_handle : parent_constrained) {
                Node& child = GetNode(child_handle);
                child.width.resolved = pixels_per - child.NonContentSize().x * 2U;
            }
            GetNode(parent_constrained[0U]).width.resolved += left_over;
        }

        // minor axis
         auto children = parent->children | std::views::filter([this] (const Node::Handle child_handle) -> bool { return GetNode(child_handle).height.layout_type == LayoutLength::parent_constraint; });
        for (const Node::Handle child_handle : children) { GetNode(child_handle).height.resolved = parent->InnerBoxSize().y; }
    }

    // position top down
    for (Node* parent : parents) {
        uint2 position = parent->InnerBoxPosition();
        for (const Node::Handle child_handle : parent->children) {
            Node& child = GetNode(child_handle);
            child.position = position;
            // major axis
            position.x += child.OuterBoxSize().x;
        }
    }

    // bounding box
    for (Node* node : reversedParents) {
        uint2 start_position = node->OuterBoxPosition();
        uint2 end_position = node->OuterBoxEndPosition();
        for (const Node::Handle child_handle : node->children) {
            Node& child = GetNode(child_handle);
            uint2 child_start_position = child.OuterBoxPosition();
            uint2 child_end_position = child.OuterBoxEndPosition();
            start_position.x = std::min(child_start_position.x, start_position.x);
            start_position.y = std::min(child_start_position.y, start_position.y);
            end_position.x = std::max(child_end_position.x, end_position.x);
            end_position.y = std::max(child_end_position.y, end_position.y);
        }
        uint2 size = end_position - start_position;
        node->bounding_box = { .x = static_cast<f32>(start_position.x), .y = static_cast<f32>(start_position.y), .w = static_cast<f32>(size.x), .h = static_cast<f32>(size.y) };
    }
}

FrameElements NodeTree::CreateFrameElements() {
    FrameElements elements;
    std::stack<Node::Handle> nodes;
    nodes.push(Root());
    while (!nodes.empty()) {
        const Node& node = GetNode(nodes.top());
        nodes.pop();
        RectangleElement rectangle { .color = node.background_color, .rect = node.OuterRect(), .on_click = nullptr };
        elements.rectangles.PushBack(rectangle);
        for (const Node::Handle child : node.children) { nodes.push(child); }
    }

    return elements;
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

NodeBuilder& NodeBuilder::Name(const String& name) {
    node.name = name;
    return *this;
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
