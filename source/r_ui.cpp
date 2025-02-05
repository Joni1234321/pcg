#include "r_ui.hpp"

namespace pce::ui {
Node::OptionalHandle HitNode(NodeTree& tree, uint2 screen_position) {
    if (tree.Empty() || !tree.GetNode(tree.Root()).IsInside(screen_position)) { return Node::OptionalHandle { }; }
    Node::Handle node_handle = tree.Root();
    const auto is_inside_node = [screen_position, &tree] (const Node::Handle child_handle) -> b8 { return tree.GetNode(child_handle).IsInside(screen_position); };
    while (true) {
        auto node_iterator = std::ranges::find_if(tree.Children(node_handle), is_inside_node, std::identity { });
        if (node_iterator == tree.Children(node_handle).end()) { return tree.GetNode(node_handle).background_color.a == 0 ? Node::OptionalHandle { } : Node::OptionalHandle { node_handle.id }; }
        node_handle = *node_iterator;
    }
}
HoveredType GetHovered(NodeRenderSystem& render_system, const uint2 mouse_position) {
    for (NodeTree& tree : render_system.GetNodeTrees()) {
        Node::OptionalHandle node_handle = HitNode(tree, mouse_position);
        if (node_handle.IsValid()) { return std::optional { NodeReference { .tree = tree, .node_handle = node_handle.GetHandle() } }; }
    }
    return std::nullopt;
}
void RecalculateTreeLayout(NodeRenderSystem& node_render_system, NodeTree& tree, FontCollection& font) {
    if (tree.Empty()) { return; }

    auto pixels_gap = [&tree] (const Node::Handle node_handle, const u32 gap) -> u32 { return tree.Children(node_handle).Empty() ? 0U : gap * (tree.Children(node_handle).Size() - 1U); };
    auto get_major = [] (const uint2 point, const FlexDirection direction) -> u32 { return direction == horizontal ? point.x : point.y; };
    auto get_minor = [] (const uint2 point, const FlexDirection direction) -> u32 { return direction == horizontal ? point.y : point.x; };
    auto get_major_layout = [] (Node& node, const FlexDirection direction) -> LayoutLength& { return direction == horizontal ? node.width : node.height; };
    auto get_minor_layout = [] (Node& node, const FlexDirection direction) -> LayoutLength& { return direction == horizontal ? node.height : node.width; };
    auto get_major_pixels_taken_by_children = [&tree, &get_major] (const Node::Handle node_handle, const FlexDirection direction) -> u32 {
        auto get_major_outer_box_size = [&tree, &get_major, &direction] (const Node::Handle child_handle) -> u32 { return get_major(tree.GetNode(child_handle).OuterBoxSize(), direction); };
        return std::ranges::fold_left_first(tree.Children(node_handle) | std::views::transform(get_major_outer_box_size), std::plus { }).value_or(0U);
    };

    List<Node::Handle> node_handles { tree.Root() };
    for (u32 i = 0U; i < node_handles.Size(); ++i) { node_handles.AppendRange(tree.Children(node_handles[i])); }

    // text
    for (const Node::Handle node_handle : node_handles) {
        Node& node = tree.GetNode(node_handle);

        UniquePointer<TTF_Text, DestroyText>& text = tree.Text(node_handle);
        if (!node.IsText()) {
            text.Reset();
            continue;
        }

        const Font& f = font.GetFont(node.font_size);
        if (text.Get() == nullptr) { text.Reset(TTF_CreateText(node_render_system.text_engine.Get(), f.ToSDL(), node.text.CString(), node.text.Size())); } else {
            TTF_SetTextString(text.Get(), node.text.CString(), node.text.Size());
            TTF_SetTextFont(text.Get(), f.ToSDL());
        }
        const SDL_Color color = node.background_color;
        (void)TTF_SetTextColor(text.Get(), color.r, color.g, color.b, color.a);
    }

    // hug bottom up
    #define BREAKPOINT 1
    #if BREAKPOINT
    auto reversed_nodes = node_handles | std::views::reverse | std::ranges::to<std::vector>();
    #else
    auto reversed_nodes = node_handles | std::views::reverse;
    #endif
    for (const Node::Handle node_handle : reversed_nodes) {
        Node& node = tree.GetNode(node_handle);
        if (node.width.constraint != LayoutLength::child_constraint && node.height.constraint != LayoutLength::child_constraint) { continue; }

        uint2 text_size { 0U, 0U };
        if (node.IsText()) { (void)TTF_GetTextSize(tree.Text(node_handle).Get(), reinterpret_cast<i32*>(&text_size.x), reinterpret_cast<i32*>(&text_size.y)); }
        LayoutLength& major_layout = get_major_layout(node, node.direction);
        if (major_layout.constraint == LayoutLength::child_constraint) {
            const u32 children_major = get_major_pixels_taken_by_children(node_handle, node.direction);
            major_layout.resolved = children_major + pixels_gap(node_handle, node.gap) + get_major(text_size, node.direction) + get_major(node.NonContentSize2(), node.direction);
        }
        LayoutLength& minor_layout = get_minor_layout(node, node.direction);
        if (minor_layout.constraint == LayoutLength::child_constraint) {
            auto get_minor_outer_box_size = [&tree, get_minor, &node] (const Node::Handle child_handle) -> u32 { return get_minor(tree.GetNode(child_handle).OuterBoxSize(), node.direction); };
            const u32 max_minor = tree.Children(node_handle).Empty() ? 0U : std::ranges::max(tree.Children(node_handle) | std::views::transform(get_minor_outer_box_size));
            minor_layout.resolved = std::max(max_minor, get_minor(text_size, node.direction)) + get_minor(node.NonContentSize2(), node.direction);
        }
    }

    // fill top down
    Node& root_node = tree.GetNode(tree.Root());
    if (root_node.width.constraint == LayoutLength::parent_constraint) { root_node.width.resolved = node_render_system.render_system.screen_size.x - root_node.position.x; }
    if (root_node.height.constraint == LayoutLength::parent_constraint) { root_node.height.resolved = node_render_system.render_system.screen_size.y - root_node.position.y; }
    for (const Node::Handle node_handle : node_handles) {
        const Node& node = tree.GetNode(node_handle);

        List<Node::Handle> parent_constrained { };
        u32 pixels_taken_major_axis = pixels_gap(node_handle, node.gap);
        for (const Node::Handle child_handle : tree.Children(node_handle)) {
            Node& child = tree.GetNode(child_handle);
            LayoutLength& child_major_layout = get_major_layout(child, node.direction);
            if (child_major_layout.constraint == LayoutLength::parent_constraint) { parent_constrained.PushBack(child_handle); } else { pixels_taken_major_axis += get_major(child.OuterBoxSize(), node.direction); }
        }
        if (parent_constrained.Size() > 0U) {
            if (pixels_taken_major_axis >= get_major(node.InnerBoxSize(), node.direction)) {
                for (const Node::Handle child_handle : parent_constrained) {
                    constexpr u32 min_pixel_size = 10U;
                    get_major_layout(tree.GetNode(child_handle), node.direction).resolved = min_pixel_size;
                }
                continue;
            }
            const auto [pixels_per, left_over] = math::Div(get_major(node.InnerBoxSize(), node.direction) - pixels_taken_major_axis, parent_constrained.Size());
            for (const Node::Handle child_handle : parent_constrained) { get_major_layout(tree.GetNode(child_handle), node.direction).resolved = pixels_per; }
            get_major_layout(tree.GetNode(parent_constrained[0U]), node.direction).resolved += left_over;
        }

        auto children = tree.Children(node_handle) | std::views::filter([&tree, &node, &get_minor_layout] (const Node::Handle child_handle) -> bool {
            return get_minor_layout(tree.GetNode(child_handle), node.direction).constraint == LayoutLength::parent_constraint;
        });
        for (const Node::Handle child_handle : children) { get_minor_layout(tree.GetNode(child_handle), node.direction).resolved = get_minor(node.InnerBoxSize(), node.direction); }
    }

    // position top down
    for (const Node::Handle node_handle : node_handles) {
        const Node& node = tree.GetNode(node_handle);
        u32 major_position = get_major(node.InnerBoxPosition(), node.direction);
        const u32 children_major = get_major_pixels_taken_by_children(node_handle, node.direction);
        if (node.direction == horizontal) {
            switch (node.alignment) {
                case top_left:
                case left:
                case bottom_left:
                    break;
                case top_center:
                case center:
                case bottom_center:
                    major_position += (node.InnerBoxSize().x - children_major) / 2U;
                break;
                case top_right:
                case right:
                case bottom_right:
                    major_position += node.InnerBoxSize().x - children_major;
                break;
            }
        }
        else {
            switch (node.alignment) {
                case top_left:
                case top_center:
                case top_right:
                    break;
                case left:
                case center:
                case right:
                    major_position += (node.InnerBoxSize().y - children_major) / 2U;
                break;
                case bottom_left:
                case bottom_center:
                case bottom_right:
                    major_position += node.InnerBoxSize().y - children_major;
                break;
            }
        }
        for (const Node::Handle child_handle : tree.Children(node_handle)) {
            Node& child = tree.GetNode(child_handle);

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
        Node& node = tree.GetNode(node_handle);
        uint2 start_position = node.OuterBoxPosition();
        uint2 end_position = node.OuterBoxEndPosition();
        for (const Node::Handle child_handle : tree.Children(node_handle)) {
            Node& child = tree.GetNode(child_handle);
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
const FrameElements& GetFrameElements(NodeRenderSystem& node_render_system, NodeTree& tree) {
    if (tree.dirty) {
        tree.dirty = false;
        RecalculateTreeLayout(node_render_system, tree, node_render_system.font);
        tree.frame_elements.rectangles.Clear();
        tree.frame_elements.texts.Clear();
        if (!tree.Empty()) {
            Stack<Node::Handle> nodes;
            nodes.push(tree.Root());
            while (!nodes.empty()) {
                const Node::Handle node_handle = nodes.top();
                const Node& node = tree.GetNode(node_handle);
                nodes.pop();
                nodes.push_range(tree.Children(node_handle));

                if (node.IsText()) {
                    TextElement text { .text = tree.Text(node_handle).Get(), .position = float2 { static_cast<f32>(node.InnerBoxPosition().x), static_cast<f32>(node.InnerBoxPosition().y) } };
                    tree.frame_elements.texts.PushBack(text);
                } else {
                    RectangleElement rectangle { .color = node.background_color, .rect = node.OuterRect() };
                    tree.frame_elements.rectangles.PushBack(rectangle);
                }
            }
        }
    }
    return tree.frame_elements;
}

void NodeRenderSystem::HoverClickEvents(const InputSystem& input_system) {
    if (input_system.LeftMouseDown() && hovered.has_value()) {
        NodeTree& hovered_tree = hovered.value().tree;
        const Node::Handle hovered_node = hovered.value().node_handle;
        hovered_tree.Propagate(hovered_node, &Node::OnClick);
    }

    if (hovered.has_value() && !hovered.value().tree.get().ValidHandle(hovered.value().node_handle)) { hovered = std::nullopt; }
    const HoveredType previous_hovered = hovered;
    hovered = GetHovered(*this, input_system.MousePosition());

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
void NodeRenderSystem::RenderTrees(SDL_Renderer* renderer) {
    for (NodeTree& tree : GetNodeTrees() | std::views::reverse | std::views::filter(&NodeTree::GetDisplay)) {
        const FrameElements& frame_elements = GetFrameElements(*this, tree);
        for (const RectangleElement& element : frame_elements.rectangles) {
            (void)SDL_SetRenderDrawColor(renderer, element.color.r, element.color.g, element.color.b, element.color.a);
            (void)SDL_RenderFillRect(renderer, &element.rect);
        }
        for (const TextElement& text : frame_elements.texts) { (void)TTF_DrawRendererText(text.text, text.position.x, text.position.y); }
    }
}
}
