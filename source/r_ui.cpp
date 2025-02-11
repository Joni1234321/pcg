#include "r_ui.hpp"

#include "r_ui_node.hpp"
#include "u_types.hpp"

namespace pce::ui {
HandleOptional<Node> HitNode(NodeTree& tree, uint2 screen_position) {
    if (tree.Empty() || !tree.node_styles[tree.Root()].IsInside(screen_position)) { return HandleOptional<Node> { }; }
    Handle<Node> node_handle = tree.Root();
    const auto is_inside_node = [screen_position, &tree] (const Handle<Node> child_handle) -> b8 { return tree.node_styles[child_handle].IsInside(screen_position); };
    while (true) {
        const auto& node_iterator = std::ranges::find_if(tree.children[node_handle], is_inside_node, std::identity { });
        if (node_iterator == tree.children[node_handle].end()) { return HandleOptional<Node> { node_handle.id }; }
        node_handle = *node_iterator;
    }
}
HoveredType GetHovered(const uint2 mouse_position) {
    u32 i = 0;
    for (NodeTree& tree : NodeSystem::node_trees) {
        HandleOptional<Node> node_handle = HitNode(tree, mouse_position);
        if (node_handle.IsValid()) { return std::optional { NodeReference { .tree_handle = Handle<NodeTree> { NodeSystem::node_trees.offset_handle.id + i }, .node_handle = node_handle.GetHandle() } }; }
        i++;
    }
    return std::nullopt;
}
void RecalculateTreeLayout(NodeTree& tree) {
    if (tree.Empty()) { return; }

    auto pixels_gap = [&tree] (const Handle<Node> node_handle, const u32 gap) -> u32 { return tree.children[node_handle].Empty() ? 0U : gap * (tree.children[node_handle].Size() - 1U); };
    auto get_major = [] (const uint2 point, const FlexDirection direction) -> u32 { return direction == horizontal ? point.x : point.y; };
    auto get_minor = [] (const uint2 point, const FlexDirection direction) -> u32 { return direction == horizontal ? point.y : point.x; };
    auto get_major_layout = [] (NodeStyle& node_style, const FlexDirection direction) -> LayoutLength& { return direction == horizontal ? node_style.width : node_style.height; };
    auto get_minor_layout = [] (NodeStyle& node_style, const FlexDirection direction) -> LayoutLength& { return direction == horizontal ? node_style.height : node_style.width; };
    auto get_major_pixels_taken_by_children = [&tree, &get_major] (const Handle<Node> node_handle, const FlexDirection direction) -> u32 {
        auto get_major_outer_box_size = [&tree, &get_major, &direction] (const Handle<Node> child_handle) -> u32 { return get_major(tree.node_styles[child_handle].OuterBoxSize(), direction); };
        return std::ranges::fold_left_first(tree.children[node_handle] | std::views::transform(get_major_outer_box_size), std::plus { }).value_or(0U);
    };

    List<Handle<Node>> node_handles { tree.Root() };
    for (u32 i = 0U; i < node_handles.Size(); ++i) { node_handles.AppendRange(tree.children[node_handles[i]]); }

    // text
    for (const Handle<Node> node_handle : node_handles) {
        NodeStyle& node_style = tree.node_styles[node_handle];
        NodeProperties& node_properties = tree.node_properties[node_handle];
        UniquePointer<TTF_Text, DestroyText>& ttf_text = tree.node_ttf_texts[node_handle];
        if (node_properties.text.Empty()) {
            ttf_text.Reset();
            continue;
        }

        const Font& font = NodeSystem::font.GetFont(node_properties.font_size);
        if (ttf_text.Get() == nullptr) { ttf_text.Reset(TTF_CreateText(Window::text_engine, font.ToSDL(), node_properties.text.CString(), node_properties.text.Size())); } else {
            TTF_SetTextString(ttf_text.Get(), node_properties.text.CString(), node_properties.text.Size());
            TTF_SetTextFont(ttf_text.Get(), font.ToSDL());
        }
        const SDL_Color color = node_style.background_color;
        (void)TTF_SetTextColor(ttf_text.Get(), color.r, color.g, color.b, color.a);
    }

    // hug bottom up
    #define BREAKPOINT 1
    #if BREAKPOINT
    auto reversed_nodes = node_handles | std::views::reverse | std::ranges::to<std::vector>();
    #else
    auto reversed_nodes = node_handles | std::views::reverse;
    #endif
    for (const Handle<Node> node_handle : reversed_nodes) {
        NodeStyle& node_style = tree.node_styles[node_handle];
        NodeProperties& node_properties = tree.node_properties[node_handle];
        if (node_style.width.constraint != LayoutLength::child_constraint && node_style.height.constraint != LayoutLength::child_constraint) { continue; }

        uint2 text_size { 0U, 0U };
        if (!node_properties.text.Empty()) { (void)TTF_GetTextSize(tree.node_ttf_texts[node_handle].Get(), reinterpret_cast<i32*>(&text_size.x), reinterpret_cast<i32*>(&text_size.y)); }
        LayoutLength& major_layout = get_major_layout(node_style, node_style.direction);
        if (major_layout.constraint == LayoutLength::child_constraint) {
            const u32 children_major = get_major_pixels_taken_by_children(node_handle, node_style.direction);
            major_layout.resolved = children_major + pixels_gap(node_handle, node_style.resolved_gap) + get_major(text_size, node_style.direction) + get_major(node_style.NonContentSize2(), node_style.direction);
        }
        LayoutLength& minor_layout = get_minor_layout(node_style, node_style.direction);
        if (minor_layout.constraint == LayoutLength::child_constraint) {
            auto get_minor_outer_box_size = [&tree, get_minor, &node_style] (const Handle<Node> child_handle) -> u32 { return get_minor(tree.node_styles[child_handle].OuterBoxSize(), node_style.direction); };
            const u32 max_minor = tree.children[node_handle].Empty() ? 0U : std::ranges::max(tree.children[node_handle] | std::views::transform(get_minor_outer_box_size));
            minor_layout.resolved = std::max(max_minor, get_minor(text_size, node_style.direction)) + get_minor(node_style.NonContentSize2(), node_style.direction);
        }
    }

    // fill top down
    NodeStyle& root_node = tree.node_styles[tree.Root()];
    if (root_node.width.constraint == LayoutLength::parent_constraint) { root_node.width.resolved = Window::screen_size.x - root_node.position.x; }
    if (root_node.height.constraint == LayoutLength::parent_constraint) { root_node.height.resolved = Window::screen_size.y - root_node.position.y; }
    for (const Handle<Node> node_handle : node_handles) {
        const NodeStyle& node_style = tree.node_styles[node_handle];

        List<Handle<Node>> parent_constrained { };
        u32 pixels_taken_major_axis = pixels_gap(node_handle, node_style.resolved_gap);
        for (const Handle<Node> child_handle : tree.children[node_handle]) {
            NodeStyle& child = tree.node_styles[child_handle];
            LayoutLength& child_major_layout = get_major_layout(child, node_style.direction);
            if (child_major_layout.constraint == LayoutLength::parent_constraint) { parent_constrained.PushBack(child_handle); } else { pixels_taken_major_axis += get_major(child.OuterBoxSize(), node_style.direction); }
        }
        if (parent_constrained.Size() > 0U) {
            if (pixels_taken_major_axis >= get_major(node_style.InnerBoxSize(), node_style.direction)) {
                for (const Handle<Node> child_handle : parent_constrained) {
                    constexpr u32 min_pixel_size = 10U;
                    get_major_layout(tree.node_styles[child_handle], node_style.direction).resolved = min_pixel_size;
                }
                continue;
            }
            const auto [pixels_per, left_over] = math::Div(get_major(node_style.InnerBoxSize(), node_style.direction) - pixels_taken_major_axis, parent_constrained.Size());
            for (const Handle<Node> child_handle : parent_constrained) { get_major_layout(tree.node_styles[child_handle], node_style.direction).resolved = pixels_per; }
            get_major_layout(tree.node_styles[parent_constrained[0U]], node_style.direction).resolved += left_over;
        }

        auto children = tree.children[node_handle] | std::views::filter([&tree, &node_style, &get_minor_layout] (const Handle<Node> child_handle) -> bool {
            return get_minor_layout(tree.node_styles[child_handle], node_style.direction).constraint == LayoutLength::parent_constraint;
        });
        for (const Handle<Node> child_handle : children) { get_minor_layout(tree.node_styles[child_handle], node_style.direction).resolved = get_minor(node_style.InnerBoxSize(), node_style.direction); }
    }

    // position top down
    for (const Handle<Node> node_handle : node_handles) {
        NodeStyle& node_style = tree.node_styles[node_handle];
        const u32 minor_position = get_minor(node_style.InnerBoxPosition(), node_style.direction);
        u32 major_position = get_major(node_style.InnerBoxPosition(), node_style.direction);
        const u32 node_major_size = get_major(node_style.InnerBoxSize(), node_style.direction);
        const u32 children_major_size = get_major_pixels_taken_by_children(node_handle, node_style.direction);
        if (node_style.gap_auto) {
            if (children_major_size < node_major_size && tree.children[node_handle].Size() > 1) { node_style.resolved_gap = (node_major_size - children_major_size) / (tree.children[node_handle].Size() - 1U); } else {
                node_style.resolved_gap = 0U;
            }
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
        const u32 major_pixels_left = node_major_size - children_major_size - node_style.resolved_gap * (tree.children[node_handle].Size() - 1U);
        major_position += major_pixels_left * factor_major;
        for (const Handle<Node> child_handle : tree.children[node_handle]) {
            NodeStyle& child = tree.node_styles[child_handle];
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
    for (const Handle<Node> node_handle : reversed_nodes) {
        NodeStyle& node_style = tree.node_styles[node_handle];
        uint2 start_position = node_style.OuterBoxPosition();
        uint2 end_position = node_style.OuterBoxEndPosition();
        for (const Handle<Node> child_handle : tree.children[node_handle]) {
            NodeStyle& child = tree.node_styles[child_handle];
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
const FrameElements& GetFrameElements(NodeTree& tree) {
    if (tree.dirty) {
        tree.dirty = false;
        RecalculateTreeLayout(tree);
        tree.frame_elements.rectangles.Clear();
        tree.frame_elements.texts.Clear();
        if (!tree.Empty()) {
            Stack<Handle<Node>> nodes;
            nodes.push(tree.Root());
            while (!nodes.empty()) {
                const Handle<Node> node_handle = nodes.top();
                const NodeStyle& node_style = tree.node_styles[node_handle];
                const NodeProperties& node_properties = tree.node_properties[node_handle];
                nodes.pop();
                nodes.push_range(tree.children[node_handle]);

                if (node_properties.text.Empty()) {
                    RectangleElement rectangle { .color = node_style.background_color, .rect = node_style.OuterRect() };
                    tree.frame_elements.rectangles.PushBack(rectangle);
                } else {
                    TextElement text { .text = tree.node_ttf_texts[node_handle].Get(), .position = float2 { static_cast<f32>(node_style.InnerBoxPosition().x), static_cast<f32>(node_style.InnerBoxPosition().y) } };
                    tree.frame_elements.texts.PushBack(text);
                }
            }
        }
    }
    return tree.frame_elements;
}

void Hover(const NodeReference node_reference) {
    Logger().Log("Hover {}", NodeSystem::node_trees[node_reference.tree_handle].node_properties[node_reference.node_handle].text);
    NodeProperties& properties = NodeSystem::node_trees[node_reference.tree_handle].node_properties[node_reference.node_handle];
    if (properties.on_hover) { properties.on_hover(node_reference); }
}
void HoverOut(const NodeReference node_reference) {
    Logger().Log("Hover Out {}", NodeSystem::node_trees[node_reference.tree_handle].node_properties[node_reference.node_handle].text);
    NodeProperties& properties = NodeSystem::node_trees[node_reference.tree_handle].node_properties[node_reference.node_handle];
    if (properties.on_hover_out) { properties.on_hover_out(node_reference); }
}
void Click(const NodeReference node_reference) {
    Logger().Log("Clicked {}", NodeSystem::node_trees[node_reference.tree_handle].node_properties[node_reference.node_handle].text);
    NodeProperties& properties = NodeSystem::node_trees[node_reference.tree_handle].node_properties[node_reference.node_handle];
    if (properties.on_click) { properties.on_click(node_reference); }
}
void Propagate(NodeReference node_reference, const NodeReaction& reaction) {
    Handle<Node> root = NodeSystem::node_trees[node_reference.tree_handle].Root();
    while (true) {
        std::invoke(reaction, node_reference);
        if (node_reference.node_handle.id == root.id) { break; };
        node_reference.node_handle = NodeSystem::node_trees[node_reference.tree_handle].parents[node_reference.node_handle];
    }
}
void NodeSystem::HoverClickEvents() {
    if (InputSystem::input_table.left_mouse_down && hovered.has_value()) { Propagate(hovered.value(), Click); }

    if (hovered.has_value() && !node_trees[hovered->tree_handle].node_styles.ValidHandle(hovered->node_handle)) { hovered = std::nullopt; }
    const HoveredType previous_hovered = hovered;
    hovered = GetHovered(InputSystem::input_table.mouse_position);

    if (hovered.has_value() && previous_hovered.has_value() && hovered->tree_handle.id == previous_hovered->tree_handle.id && hovered->node_handle.id == previous_hovered->node_handle.id) { return; }

    if (previous_hovered.has_value()) {
        Propagate(previous_hovered.value(), HoverOut);
        node_trees[previous_hovered->tree_handle].MarkDirty();
    }
    if (hovered.has_value()) {
        Propagate(hovered.value(), Hover);
        node_trees[hovered->tree_handle].MarkDirty();
    }
}
void NodeSystem::RenderTrees() {
    for (NodeTree& tree : node_trees | std::views::reverse | std::views::filter(&NodeTree::GetDisplay)) {
        const FrameElements& frame_elements = GetFrameElements(tree);
        for (const RectangleElement& element : frame_elements.rectangles) {
            (void)SDL_SetRenderDrawColor(Window::renderer, element.color.r, element.color.g, element.color.b, element.color.a);
            (void)SDL_RenderFillRect(Window::renderer, &element.rect);
        }
        for (const TextElement& text : frame_elements.texts) { (void)TTF_DrawRendererText(text.text, text.position.x, text.position.y); }
    }
}
f32 EasingSin(const f32 t) { return math::Sin(t) * 0.5F + 0.5F; }
f32 EasingCos(const f32 t) { return math::Cos(t) * 0.5F + 0.5F; }
Handle<Animation> AnimationSystem::Register(const AnimationDesc& animation_desc) {
    Logger().Log("Size of action {} and action {} size {}", sizeof(animation_desc), sizeof(animation_desc.action), animations.Size());
    const Animation animation { .action = animation_desc.action, .offset_ms = static_cast<u32>(SDL_GetTicks()), .duration_ms = animation_desc.duration_ms, .state = animation_desc.state };
    for (Handle<Animation> handle { 0U }; handle.id < animations.Size(); ++handle.id) {
        if (animations[handle].state == AnimationState::recycle) {
            animations[handle] = animation;
            return handle;
        }
    }
    return animations.PushBack(animation);
}
void AnimationSystem::StartAnimation(const Handle<Animation> animation_handle) {
    Animation& animation = animations[animation_handle];
    animation.offset_ms = static_cast<u32>(SDL_GetTicks());
    switch (animation.state) {
        case AnimationState::run_once:
            break;
        case AnimationState::recycle:
            break;
        case AnimationState::repeat:
            break;
        case AnimationState::keep_alive:
            break;
        case AnimationState::keep_alive_stopped:
            animation.state = AnimationState::keep_alive;
            break;
    }
}
void AnimationSystem::operator()() {
    const u32 current_ms = SDL_GetTicks();
    for (Animation& animation : animations) {
        f32 t = static_cast<f32>(current_ms - animation.offset_ms) / static_cast<f32>(animation.duration_ms);
        switch (animation.state) {
            case AnimationState::run_once:
                if (t >= 1.0F) {
                    t = 1.0F;
                    animation.state = AnimationState::recycle;
                }
                break;
            case AnimationState::recycle:
                continue;
            case AnimationState::repeat:
                break;
            case AnimationState::keep_alive:
                if (t >= 1.0F) {
                    t = 1.0F;
                    animation.state = AnimationState::keep_alive_stopped;
                }
                break;
            case AnimationState::keep_alive_stopped:
                continue;
        }
        const f32 value = EasingSin(t);
        animation.action(value);
    }
}
}
