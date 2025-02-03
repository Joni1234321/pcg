#include "r_ui.hpp"

namespace pce::ui {
NodeRenderSystem::HoveredType NodeRenderSystem::GetHovered(const uint2 mouse_position) const {
    for (NodeTree& tree : GetNodeTrees()) {
        Node::OptionalHandle node_handle = tree.HitNode(mouse_position);
        if (node_handle.IsValid()) { return std::optional { NodeReference { .tree = tree, .node_handle = node_handle.GetHandle() } }; }
    }
    return std::nullopt;
}
void NodeRenderSystem::HoverClickEvents(const InputSystem& input_system) {
    if (input_system.LeftMouseDown()) {
        if (hovered.has_value()) {
            NodeTree& hovered_tree = hovered.value().tree;
            const Node::Handle hovered_node = hovered.value().node_handle;
            hovered_tree.Propagate(hovered_node, &Node::OnClick);
        }
    }

    if (hovered.has_value() && !hovered.value().tree.get().ValidHandle(hovered.value().node_handle)) { hovered = std::nullopt; }
    const HoveredType previous_hovered = hovered;
    hovered = GetHovered(input_system.MousePosition());

    if (hovered.has_value() && previous_hovered.has_value() && hovered == previous_hovered) { return; }

    if (previous_hovered.has_value()) {
        NodeTree& hovered_tree = previous_hovered.value().tree;
        const Node::Handle hovered_node = previous_hovered.value().node_handle;
        hovered_tree.Propagate(hovered_node, &Node::OnHoverOut);
        hovered_tree.MarkDirty();
    }
    if (hovered.has_value()) {
        NodeTree& hovered_tree = hovered.value().tree;
        const Node::Handle hovered_node = hovered.value().node_handle;
        hovered_tree.Propagate(hovered_node, &Node::OnHover);
        hovered_tree.MarkDirty();
    }
}
void NodeRenderSystem::RenderTrees(SDL_Renderer* renderer) const {
    for (const FrameElements& frame_elements : GetNodeTrees() | std::views::reverse | std::views::filter(&NodeTree::GetDisplay) | std::views::transform(&NodeTree::GetFrameElements)) {
        for (const RectangleElement& element : frame_elements.rectangles) {
            (void)SDL_SetRenderDrawColor(renderer, element.color.r, element.color.g, element.color.b, element.color.a);
            (void)SDL_RenderFillRect(renderer, &element.rect);
        }
        for (const TextElement& text : frame_elements.texts) { (void)TTF_DrawRendererText(text.text, text.position.x, text.position.y); }
    }
}
}
