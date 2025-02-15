#include "r_ui_node.hpp"

#include <ranges>

#include "orchestra.hpp"
#include "t_tick_system.hpp"

#include "engine/r_window.hpp"
#include "engine/u_colors.hpp"
#include "engine/u_types.hpp"

namespace pce::ui {
const Font& FontCollection::GetFont(const FontSizes size) {
    if (!fonts.HasKey(size)) {
        fonts.EmplaceBack(size, font_path, static_cast<FontSize>(size));
        b8 failed = fonts[size].FailedLoading();
        if (failed) { SDL_Log("Font not loaded (%s)", SDL_GetError()); }
    }
    return fonts[size];
}

// NodeTree
Handle<Node> NodeTree::AddRoot() {
    ASSERT_DBG(node_styles.Empty(), "Setting root non empty tree");
    node_styles.EmplaceBack();
    parents.EmplaceBack(Root());
    children.EmplaceBack();
    node_properties.EmplaceBack();
    node_ttf_texts.EmplaceBack(nullptr);

    return Root();
}
Handle<Node> NodeTree::AddRoot(NodeStyle&& root) {
    const Handle<Node> node_handle = AddRoot();
    node_styles[node_handle] = std::move(root);
    return node_handle;
}
Handle<Node> NodeTree::AddNode(const Handle<Node> parent_handle) {
    ASSERT_DBG(!node_styles.Empty(), "Adding node without root");
    const Handle<Node> node_handle = node_styles.EmplaceBack();
    parents.PushBack(parent_handle);
    children.EmplaceBack();
    node_properties.EmplaceBack();
    node_ttf_texts.EmplaceBack(nullptr);

    ASSERT_DBG(node_handle.id != parent_handle.id, "Assigning node to itself recursion");

    children[parent_handle].PushBack(node_handle);
    return node_handle;
}
Handle<Node> NodeTree::AddNode(NodeStyle&& node, const Handle<Node> parent_handle) {
    const Handle<Node> node_handle = AddNode(parent_handle);
    node_styles[node_handle] = std::move(node);
    return node_handle;
}
void NodeTree::Clear() {
    node_styles.Clear();
    parents.Clear();
    children.Clear();
    node_properties.Clear();
    node_ttf_texts.Clear();
}
NodeBuilder::NodeBuilder(const Handle<NodeTree> tree_handle, const Layout new_layout, const uint2 position) : node_reference { tree_handle, NodeRenderSystem::node_trees[tree_handle].AddRoot() } {
    style.position = position;
    style.width = new_layout.width;
    style.height = new_layout.height;
}
NodeBuilder::NodeBuilder(const Handle<NodeTree> tree_handle, const Handle<Node> parent_handle, const Layout new_layout) : node_reference { tree_handle, NodeRenderSystem::node_trees[tree_handle].AddNode(parent_handle) } {
    style.width = new_layout.width;
    style.height = new_layout.height;
}
NodeBuilder& NodeBuilder::Name(const String& name) {
    properties.name = name;
    return *this;
}
NodeBuilder& NodeBuilder::Fill(const SDL_Color color) {
    style.background_color = color;
    return *this;
}
NodeBuilder& NodeBuilder::Padding(const u32 padding) {
    style.padding = uint4 { padding, padding, padding, padding };
    return *this;
}
NodeBuilder& NodeBuilder::Padding2(const uint2 padding) {
    style.padding = uint4 { padding.x, padding.y, padding.x, padding.y };
    return *this;
}
NodeBuilder& NodeBuilder::Padding4(const uint4 padding) {
    style.padding = padding;
    return *this;
}
NodeBuilder& NodeBuilder::Gap(const u32 gap) {
    style.resolved_gap = gap;
    style.gap_auto = false;
    return *this;
}
NodeBuilder& NodeBuilder::GapAuto() {
    style.resolved_gap = 0U;
    style.gap_auto = true;
    return *this;
}
NodeBuilder& NodeBuilder::Direction(const FlexDirection direction) {
    style.direction = direction;
    return *this;
}
constexpr SDL_Color DEFAULT_TEXT_COLOR = colors::black;
NodeBuilder& NodeBuilder::Text(const String& string) {
    if (style.background_color.a == 0U) { style.background_color = DEFAULT_TEXT_COLOR; }
    properties.text = string;
    return *this;
}
NodeBuilder& NodeBuilder::Text(String&& string) {
    if (style.background_color.a == 0U) { style.background_color = DEFAULT_TEXT_COLOR; }
    properties.text = string;
    return *this;
}
NodeBuilder& NodeBuilder::Text(const String& string, const FontSizes font_size) {
    if (style.background_color.a == 0U) { style.background_color = DEFAULT_TEXT_COLOR; }
    properties.text = string;
    properties.font_size = font_size;
    return *this;
}
NodeBuilder& NodeBuilder::Text(String&& string, const FontSizes font_size) {
    if (style.background_color.a == 0U) { style.background_color = DEFAULT_TEXT_COLOR; }
    properties.text = string;
    properties.font_size = font_size;
    return *this;
}
NodeBuilder& NodeBuilder::Alignment(const ui::Alignment alignment) {
    style.alignment = alignment;
    return *this;
}
NodeBuilder& NodeBuilder::Right() { return Alignment(right); }
NodeBuilder& NodeBuilder::Center() { return Alignment(center); }
NodeBuilder& NodeBuilder::Left() { return Alignment(left); }
SDL_Color LightenColor(const SDL_Color color, const f32 factor) {
    auto lerp = [] (const u8 channel, const f32 factor, const u8 target) -> u8 { return static_cast<u8>(channel + (target - channel) * factor); };
    return SDL_Color { lerp(color.r, factor, 255), lerp(color.g, factor, 255), lerp(color.b, factor, 255), color.a };
}
Handle<Node> NodeBuilder::Build() const {
    NodeRenderSystem::node_trees[node_reference.tree_handle].MarkDirty();
    return node_reference.node_handle;
}

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
    for (NodeTree& tree : NodeRenderSystem::node_trees) {
        HandleOptional<Node> node_handle = HitNode(tree, mouse_position);
        if (node_handle.IsValid()) { return std::optional { NodeReference { .tree_handle = Handle<NodeTree> { NodeRenderSystem::node_trees.offset_handle.id + i }, .node_handle = node_handle.GetHandle() } }; }
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

        const Font& font = NodeRenderSystem::font.GetFont(node_properties.font_size);
        if (ttf_text.Get() == nullptr) { ttf_text.Reset(TTF_CreateText(Window::window_config.text_engine, font.ToSDL(), node_properties.text.CString(), node_properties.text.Size())); } else {
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
    if (root_node.width.constraint == LayoutLength::parent_constraint) { root_node.width.resolved = Window::window_config.screen_size.x - root_node.position.x; }
    if (root_node.height.constraint == LayoutLength::parent_constraint) { root_node.height.resolved = Window::window_config.screen_size.y - root_node.position.y; }
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
    // Logger().Log("Hover {}", NodeSystem::node_trees[node_reference.tree_handle].node_properties[node_reference.node_handle].text);
    NodeProperties& properties = NodeRenderSystem::node_trees[node_reference.tree_handle].node_properties[node_reference.node_handle];
    if (properties.on_hover) { properties.on_hover(node_reference); }
}
void HoverOut(const NodeReference node_reference) {
    // Logger().Log("Hover Out {}", NodeSystem::node_trees[node_reference.tree_handle].node_properties[node_reference.node_handle].text);
    NodeProperties& properties = NodeRenderSystem::node_trees[node_reference.tree_handle].node_properties[node_reference.node_handle];
    if (properties.on_hover_out) { properties.on_hover_out(node_reference); }
}
void Click(const NodeReference node_reference) {
    // Logger().Log("Clicked {}", NodeSystem::node_trees[node_reference.tree_handle].node_properties[node_reference.node_handle].text);
    NodeProperties& properties = NodeRenderSystem::node_trees[node_reference.tree_handle].node_properties[node_reference.node_handle];
    if (properties.on_click) { properties.on_click(node_reference); }
}
void Propagate(NodeReference node_reference, const NodeReaction& reaction) {
    Handle<Node> root = NodeRenderSystem::node_trees[node_reference.tree_handle].Root();
    while (true) {
        std::invoke(reaction, node_reference);
        if (node_reference.node_handle.id == root.id) { break; };
        node_reference.node_handle = NodeRenderSystem::node_trees[node_reference.tree_handle].parents[node_reference.node_handle];
    }
}
void NodeInputSystem::operator()() {
    if (InputSystem::input_table.left_mouse_down && hovered.has_value()) { Propagate(hovered.value(), Click); }

    if (hovered.has_value() && !NodeRenderSystem::node_trees[hovered->tree_handle].node_styles.ValidHandle(hovered->node_handle)) { hovered = std::nullopt; }
    const HoveredType previous_hovered = hovered;
    hovered = GetHovered(InputSystem::input_table.mouse_position);

    if (hovered.has_value() && previous_hovered.has_value() && hovered->tree_handle.id == previous_hovered->tree_handle.id && hovered->node_handle.id == previous_hovered->node_handle.id) { return; }

    if (previous_hovered.has_value()) {
        Propagate(previous_hovered.value(), HoverOut);
        NodeRenderSystem::node_trees[previous_hovered->tree_handle].MarkDirty();
    }
    if (hovered.has_value()) {
        Propagate(hovered.value(), Hover);
        NodeRenderSystem::node_trees[hovered->tree_handle].MarkDirty();
    }
}
void NodeRenderSystem::operator()() {
    for (NodeTree& tree : node_trees | std::views::reverse | std::views::filter(&NodeTree::GetDisplay)) {
        const FrameElements& frame_elements = GetFrameElements(tree);
        for (const RectangleElement& element : frame_elements.rectangles) {
            (void)SDL_SetRenderDrawColor(Window::window_config.renderer, element.color.r, element.color.g, element.color.b, element.color.a);
            (void)SDL_RenderFillRect(Window::window_config.renderer, &element.rect);
        }
        for (const TextElement& text : frame_elements.texts) { (void)TTF_DrawRendererText(text.text, text.position.x, text.position.y); }
    }
}
}
