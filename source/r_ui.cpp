#include "r_ui.hpp"

namespace pce::ui {
NodeHandleOptional HitNode(NodeTree& tree, uint2 screen_position) {
    if (tree.Empty() || !tree.GetStyle(tree.Root()).IsInside(screen_position)) { return NodeHandleOptional { }; }
    NodeHandle node_handle = tree.Root();
    const auto is_inside_node = [screen_position, &tree] (const NodeHandle child_handle) -> b8 { return tree.GetStyle(child_handle).IsInside(screen_position); };
    while (true) {
        auto node_iterator = std::ranges::find_if(tree.Children(node_handle), is_inside_node, std::identity { });
        if (node_iterator == tree.Children(node_handle).end()) { return tree.GetStyle(node_handle).background_color.a == 0 ? NodeHandleOptional { } : NodeHandleOptional { node_handle.id }; }
        node_handle = *node_iterator;
    }
}
HoveredType GetHovered(NodeRenderSystem& render_system, const uint2 mouse_position) {
    for (NodeTree& tree : render_system.GetNodeTrees()) {
        NodeHandleOptional node_handle = HitNode(tree, mouse_position);
        if (node_handle.IsValid()) { return std::optional { WeakNodeReference { .tree = tree, .node_handle = node_handle.GetHandle() } }; }
    }
    return std::nullopt;
}
void RecalculateTreeLayout(NodeRenderSystem& node_render_system, NodeTree& tree, FontCollection& font) {
    if (tree.Empty()) { return; }

    auto pixels_gap = [&tree] (const NodeHandle node_handle, const u32 gap) -> u32 { return tree.Children(node_handle).Empty() ? 0U : gap * (tree.Children(node_handle).Size() - 1U); };
    auto get_major = [] (const uint2 point, const FlexDirection direction) -> u32 { return direction == horizontal ? point.x : point.y; };
    auto get_minor = [] (const uint2 point, const FlexDirection direction) -> u32 { return direction == horizontal ? point.y : point.x; };
    auto get_major_layout = [] (NodeStyle& node_style, const FlexDirection direction) -> LayoutLength& { return direction == horizontal ? node_style.width : node_style.height; };
    auto get_minor_layout = [] (NodeStyle& node_style, const FlexDirection direction) -> LayoutLength& { return direction == horizontal ? node_style.height : node_style.width; };
    auto get_major_pixels_taken_by_children = [&tree, &get_major] (const NodeHandle node_handle, const FlexDirection direction) -> u32 {
        auto get_major_outer_box_size = [&tree, &get_major, &direction] (const NodeHandle child_handle) -> u32 { return get_major(tree.GetStyle(child_handle).OuterBoxSize(), direction); };
        return std::ranges::fold_left_first(tree.Children(node_handle) | std::views::transform(get_major_outer_box_size), std::plus { }).value_or(0U);
    };

    List<NodeHandle> node_handles { tree.Root() };
    for (u32 i = 0U; i < node_handles.Size(); ++i) { node_handles.AppendRange(tree.Children(node_handles[i])); }

    // text
    for (const NodeHandle node_handle : node_handles) {
        NodeStyle& node_style = tree.GetStyle(node_handle);
        NodeProperties& node_properties = tree.GetProperties(node_handle);

        if (node_properties.text.Empty()) {
            node_properties.ttf_text.Reset();
            continue;
        }

        const Font& f = font.GetFont(node_properties.font_size);
        if (node_properties.ttf_text.Get() == nullptr) { node_properties.ttf_text.Reset(TTF_CreateText(node_render_system.text_engine.Get(), f.ToSDL(), node_properties.text.CString(), node_properties.text.Size())); } else {
            TTF_SetTextString(node_properties.ttf_text.Get(), node_properties.text.CString(), node_properties.text.Size());
            TTF_SetTextFont(node_properties.ttf_text.Get(), f.ToSDL());
        }
        const SDL_Color color = node_style.background_color;
        (void)TTF_SetTextColor(node_properties.ttf_text.Get(), color.r, color.g, color.b, color.a);
    }

    // hug bottom up
    #define BREAKPOINT 1
    #if BREAKPOINT
    auto reversed_nodes = node_handles | std::views::reverse | std::ranges::to<std::vector>();
    #else
    auto reversed_nodes = node_handles | std::views::reverse;
    #endif
    for (const NodeHandle node_handle : reversed_nodes) {
        NodeStyle& node_style = tree.GetStyle(node_handle);
        NodeProperties& node_properties = tree.GetProperties(node_handle);
        if (node_style.width.constraint != LayoutLength::child_constraint && node_style.height.constraint != LayoutLength::child_constraint) { continue; }

        uint2 text_size { 0U, 0U };
        if (!node_properties.text.Empty()) { (void)TTF_GetTextSize(node_properties.ttf_text.Get(), reinterpret_cast<i32*>(&text_size.x), reinterpret_cast<i32*>(&text_size.y)); }
        LayoutLength& major_layout = get_major_layout(node_style, node_style.direction);
        if (major_layout.constraint == LayoutLength::child_constraint) {
            const u32 children_major = get_major_pixels_taken_by_children(node_handle, node_style.direction);
            major_layout.resolved = children_major + pixels_gap(node_handle, node_style.resolved_gap) + get_major(text_size, node_style.direction) + get_major(node_style.NonContentSize2(), node_style.direction);
        }
        LayoutLength& minor_layout = get_minor_layout(node_style, node_style.direction);
        if (minor_layout.constraint == LayoutLength::child_constraint) {
            auto get_minor_outer_box_size = [&tree, get_minor, &node_style] (const NodeHandle child_handle) -> u32 { return get_minor(tree.GetStyle(child_handle).OuterBoxSize(), node_style.direction); };
            const u32 max_minor = tree.Children(node_handle).Empty() ? 0U : std::ranges::max(tree.Children(node_handle) | std::views::transform(get_minor_outer_box_size));
            minor_layout.resolved = std::max(max_minor, get_minor(text_size, node_style.direction)) + get_minor(node_style.NonContentSize2(), node_style.direction);
        }
    }

    // fill top down
    NodeStyle& root_node = tree.GetStyle(tree.Root());
    if (root_node.width.constraint == LayoutLength::parent_constraint) { root_node.width.resolved = node_render_system.render_system.screen_size.x - root_node.position.x; }
    if (root_node.height.constraint == LayoutLength::parent_constraint) { root_node.height.resolved = node_render_system.render_system.screen_size.y - root_node.position.y; }
    for (const NodeHandle node_handle : node_handles) {
        const NodeStyle& node_style = tree.GetStyle(node_handle);

        List<NodeHandle> parent_constrained { };
        u32 pixels_taken_major_axis = pixels_gap(node_handle, node_style.resolved_gap);
        for (const NodeHandle child_handle : tree.Children(node_handle)) {
            NodeStyle& child = tree.GetStyle(child_handle);
            LayoutLength& child_major_layout = get_major_layout(child, node_style.direction);
            if (child_major_layout.constraint == LayoutLength::parent_constraint) { parent_constrained.PushBack(child_handle); } else { pixels_taken_major_axis += get_major(child.OuterBoxSize(), node_style.direction); }
        }
        if (parent_constrained.Size() > 0U) {
            if (pixels_taken_major_axis >= get_major(node_style.InnerBoxSize(), node_style.direction)) {
                for (const NodeHandle child_handle : parent_constrained) {
                    constexpr u32 min_pixel_size = 10U;
                    get_major_layout(tree.GetStyle(child_handle), node_style.direction).resolved = min_pixel_size;
                }
                continue;
            }
            const auto [pixels_per, left_over] = math::Div(get_major(node_style.InnerBoxSize(), node_style.direction) - pixels_taken_major_axis, parent_constrained.Size());
            for (const NodeHandle child_handle : parent_constrained) { get_major_layout(tree.GetStyle(child_handle), node_style.direction).resolved = pixels_per; }
            get_major_layout(tree.GetStyle(parent_constrained[0U]), node_style.direction).resolved += left_over;
        }

        auto children = tree.Children(node_handle) | std::views::filter([&tree, &node_style, &get_minor_layout] (const NodeHandle child_handle) -> bool {
            return get_minor_layout(tree.GetStyle(child_handle), node_style.direction).constraint == LayoutLength::parent_constraint;
        });
        for (const NodeHandle child_handle : children) { get_minor_layout(tree.GetStyle(child_handle), node_style.direction).resolved = get_minor(node_style.InnerBoxSize(), node_style.direction); }
    }

    // position top down
    for (const NodeHandle node_handle : node_handles) {
        NodeStyle& node_style = tree.GetStyle(node_handle);
        const u32 minor_position = get_minor(node_style.InnerBoxPosition(), node_style.direction);
        u32 major_position = get_major(node_style.InnerBoxPosition(), node_style.direction);
        const u32 node_major_size = get_major(node_style.InnerBoxSize(), node_style.direction);
        const u32 children_major_size = get_major_pixels_taken_by_children(node_handle, node_style.direction);
        if (node_style.gap_auto) {
            if (children_major_size < node_major_size) {
                const auto [gap, left_over] = math::Div(node_major_size - children_major_size, tree.Children(node_handle).Size() - 1U);
                node_style.resolved_gap = gap;
            } else { node_style.resolved_gap = 0U; }
        }
        float2 factor;
        switch (node_style.alignment) {
            case top_left:
                factor = float2 { 0.0F, 0.0F };
                break;
            case top_center:
                factor = float2 { 0.5F, 0.0F };
                break;
            case top_right:
                factor = float2 { 1.0F, 0.0F };
                break;
            case left:
                factor = float2 { 0.0F, 0.5F };
                break;
            case center:
                factor = float2 { 0.5F, 0.5F };
                break;
            case right:
                factor = float2 { 1.0F, 0.5F };
                break;
            case bottom_left:
                factor = float2 { 0.0F, 1.0F };
                break;
            case bottom_center:
                factor = float2 { 0.5F, 1.0F };
                break;
            case bottom_right:
                factor = float2 { 1.0F, 1.0F };
                break;
        }
        const f32 factor_major = node_style.direction == horizontal ? factor.x : factor.y;
        const f32 factor_minor = node_style.direction == horizontal ? factor.y : factor.x;
        const u32 major_pixels_left = node_major_size - children_major_size - node_style.resolved_gap * (tree.Children(node_handle).Size() - 1U);
        major_position += major_pixels_left * factor_major;
        for (const NodeHandle child_handle : tree.Children(node_handle)) {
            NodeStyle& child = tree.GetStyle(child_handle);
            if (node_style.direction == horizontal) {
                const u32 child_minor_position = minor_position + static_cast<u32>((node_style.InnerBoxSize().y - child.OuterBoxSize().y) * factor_minor);
                child.position = uint2 { major_position, child_minor_position };
                major_position += child.OuterBoxSize().x + node_style.resolved_gap;
            } else {
                const u32 child_minor_position = minor_position + static_cast<u32>((node_style.InnerBoxSize().x - child.OuterBoxSize().x) * factor_minor);
                child.position = uint2 { child_minor_position, major_position };
                major_position += child.OuterBoxSize().y + node_style.resolved_gap;
            }
        }
    }

    // bounding box
    for (const NodeHandle node_handle : reversed_nodes) {
        NodeStyle& node_style = tree.GetStyle(node_handle);
        uint2 start_position = node_style.OuterBoxPosition();
        uint2 end_position = node_style.OuterBoxEndPosition();
        for (const NodeHandle child_handle : tree.Children(node_handle)) {
            NodeStyle& child = tree.GetStyle(child_handle);
            uint2 child_start_position = child.OuterBoxPosition();
            uint2 child_end_position = child.OuterBoxEndPosition();
            start_position.x = std::min(child_start_position.x, start_position.x);
            start_position.y = std::min(child_start_position.y, start_position.y);
            end_position.x = std::max(child_end_position.x, end_position.x);
            end_position.y = std::max(child_end_position.y, end_position.y);
        }
        const uint2 size = end_position - start_position;
        node_style.bounding_box = { .x = static_cast<f32>(start_position.x), .y = static_cast<f32>(start_position.y), .w = static_cast<f32>(size.x), .h = static_cast<f32>(size.y) };
    }
}
const FrameElements& GetFrameElements(NodeRenderSystem& node_render_system, NodeTree& tree) {
    if (tree.dirty) {
        tree.dirty = false;
        RecalculateTreeLayout(node_render_system, tree, node_render_system.font);
        tree.frame_elements.rectangles.Clear();
        tree.frame_elements.texts.Clear();
        if (!tree.Empty()) {
            Stack<NodeHandle> nodes;
            nodes.push(tree.Root());
            while (!nodes.empty()) {
                const NodeHandle node_handle = nodes.top();
                const NodeStyle& node_style = tree.GetStyle(node_handle);
                const NodeProperties& node_properties = tree.GetProperties(node_handle);
                nodes.pop();
                nodes.push_range(tree.Children(node_handle));

                if (node_properties.text.Empty()) {
                    RectangleElement rectangle { .color = node_style.background_color, .rect = node_style.OuterRect() };
                    tree.frame_elements.rectangles.PushBack(rectangle);
                } else {
                    TextElement text { .text = node_properties.ttf_text.Get(), .position = float2 { static_cast<f32>(node_style.InnerBoxPosition().x), static_cast<f32>(node_style.InnerBoxPosition().y) } };
                    tree.frame_elements.texts.PushBack(text);
                }
            }
        }
    }
    return tree.frame_elements;
}

void Hover(const NodeReference& node_reference) {
    Logger().Log("Hover");
    NodeStyle& node_style = node_reference.tree.GetStyle(node_reference.node_handle);
    std::swap(node_style.background_color, node_style.background_color_hover);
    NodeProperties& details = node_reference.tree.GetProperties(node_reference.node_handle);
    if (details.on_hover) { details.on_hover(node_reference); }
}
void HoverOut(const NodeReference& node_reference) {
    Logger().Log("Hover Out");
    NodeStyle& node_style = node_reference.tree.GetStyle(node_reference.node_handle);
    std::swap(node_style.background_color, node_style.background_color_hover);
    NodeProperties& details = node_reference.tree.GetProperties(node_reference.node_handle);
    if (details.on_hover_out) { details.on_hover_out(node_reference); }
}
void Click(const NodeReference& node_reference) {
    Logger().Log("Clicked");
    NodeProperties& properties = node_reference.tree.GetProperties(node_reference.node_handle);
    if (properties.on_click) { properties.on_click(node_reference); }
}
void NodeRenderSystem::HoverClickEvents(const InputSystem& input_system) {
    if (input_system.LeftMouseDown() && hovered.has_value()) {
        NodeTree& hovered_tree = hovered.value().tree;
        const NodeHandle hovered_node = hovered.value().node_handle;
        hovered_tree.Propagate(hovered_node, Click);
    }

    if (hovered.has_value() && !hovered.value().tree.get().ValidHandle(hovered.value().node_handle)) { hovered = std::nullopt; }
    const HoveredType previous_hovered = hovered;
    hovered = GetHovered(*this, input_system.MousePosition());

    if (hovered.has_value() && previous_hovered.has_value() && hovered == previous_hovered) { return; }

    if (previous_hovered.has_value()) {
        NodeTree& hovered_tree = previous_hovered.value().tree;
        const NodeHandle hovered_node = previous_hovered.value().node_handle;
        hovered_tree.Propagate(hovered_node, HoverOut);
        hovered_tree.MarkDirty();
    }
    if (hovered.has_value()) {
        NodeTree& hovered_tree = hovered.value().tree;
        const NodeHandle hovered_node = hovered.value().node_handle;
        hovered_tree.Propagate(hovered_node, Hover);
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
