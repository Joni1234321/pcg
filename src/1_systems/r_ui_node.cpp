#include "r_ui_node.hpp"

#include <ranges>

#include "0_engine/r_window.hpp"
#include "0_engine/u_colors.hpp"
#include "0_engine/u_types.hpp"

#include "1_systems/i_input_system.hpp"
#include "1_systems/t_tick_system.hpp"

namespace pce::ui {
const Font& FontCollection::GetFont(const FontSizes size) {
    if (!fonts.HasKey(size) && font_path != "") {
        fonts.EmplaceBack(size, font_path, static_cast<FontSize>(size));
        if (fonts[size].FailedLoading()) {
            SDL_Log("ERROR Failed Font not loaded (%s)", SDL_GetError());
            fonts.Erase(size);
        }
    }
    return fonts[size];
}

// NodeTree
Handle<Node> NodeTree::AddRoot() {
    STL_ASSERT(styles.empty(), "Setting root non empty tree");
    (void)styles.EmplaceBack();
    (void)node_properties.EmplaceBack();
    (void)node_ttf_texts.EmplaceBack(nullptr);
    (void)parents.EmplaceBack(Root());
    (void)children.EmplaceBack();
    (void)subtree_roots.EmplaceBack(Root().id);
    return Root();
}
Handle<Node> NodeTree::AddNode(const Handle<Node> parent) {
    STL_ASSERT(!styles.empty(), "Adding node before root");
    const Handle<Node> node = styles.EmplaceBack();
    (void)node_properties.EmplaceBack();
    (void)node_ttf_texts.EmplaceBack(nullptr);
    (void)children[parent].EmplaceBack(node);
    (void)parents.PushBack(parent);
    (void)children.EmplaceBack();
    (void)subtree_roots.EmplaceBack(Root().id);
    STL_ASSERT(node.id != parent.id, "Assigning node to itself. Recursion!");
    return node;
}
Handle<Node> NodeTree::CloneNode(const Handle<Node> clone) {
    const Handle<Node> node = AddNode(parents[clone]);
    styles[node] = styles[clone];
    node_properties[node] = node_properties[clone];
    return node;
}
void NodeTree::DetachNode(const Handle<Node> node) {
    STL_ASSERT(node.id != Root().id, "trying to detach root");
    const Handle parent = parents[node];
    children[parent].erase_value(node);
}
void NodeTree::AttachNode(const Handle<Node> node, const Handle<Node> parent) {
    assert(node.id != Root().id);
    children[parent].push_back(node);
    parents[node] = parent;
}

void NodeTree::Clear() {
    styles.clear();
    node_properties.clear();
    node_ttf_texts.clear();
    parents.clear();
    children.clear();
    subtree_roots.clear();
}
NodeBuilder::NodeBuilder(const Handle<NodeTree> tree, const Layout new_layout, const uint2 position) : node_reference { tree, data[tree].AddRoot() } {
    style.position = position;
    style.width = new_layout.width;
    style.height = new_layout.height;
}
NodeBuilder::NodeBuilder(const Handle<NodeTree> tree, const Handle<Node> parent, const Layout new_layout) : node_reference { tree, data[tree].AddNode(parent) } {
    style.width = new_layout.width;
    style.height = new_layout.height;
}
NodeBuilder::NodeBuilder(const NodeReference parent, const Layout new_layout) : node_reference { parent.tree, data[parent.tree].AddNode(parent.node) } {
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
NodeBuilder& NodeBuilder::Texture(const Handle<pce::Texture> texture) {
    style.texture = OptionalHandle { texture };
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
NodeBuilder& NodeBuilder::Text(const String& string, const SDL_Color color) {
    properties.text = string;
    style.background_color = color;
    return *this;
}
NodeBuilder& NodeBuilder::Text(String&& string, const SDL_Color color) {
    properties.text = string;
    style.background_color = color;
    return *this;
}
NodeBuilder& NodeBuilder::Text(const String& string, const FontSizes font_size, const SDL_Color color) {
    properties.text = string;
    properties.font_size = font_size;
    style.background_color = color;
    return *this;
}
NodeBuilder& NodeBuilder::Text(String&& string, const FontSizes font_size, const SDL_Color color) {
    properties.text = string;
    properties.font_size = font_size;
    style.background_color = color;
    return *this;
}
NodeBuilder& NodeBuilder::Text(const FontSizes font_size, const SDL_Color color) {
    properties.font_size = font_size;
    style.background_color = color;
    return *this;
}
NodeBuilder& NodeBuilder::Alignment(const ui::Alignment alignment) {
    style.alignment = alignment;
    return *this;
}
NodeBuilder& NodeBuilder::Right() { return Alignment(right); }
NodeBuilder& NodeBuilder::Center() { return Alignment(center); }
NodeBuilder& NodeBuilder::Left() { return Alignment(left); }
NodeBuilder& NodeBuilder::OnClick(NodeReaction&& reaction) {
    properties.on_click = reaction;
    return *this;
}
Handle<Node> NodeBuilder::Build() const {
    data[node_reference.tree].MarkDirty();
    return node_reference.node;
}

OptionalHandle<Node> NodeAt(const NodeTree& tree, const uint2 screen_position) {
    const auto position_inside_node = [screen_position, &tree](const Handle<Node> child) -> b8 { return tree.styles[child].IsInside(screen_position); };
    if (tree.Empty() || !tree.display || !tree.styles[tree.Root()].IsInside(screen_position)) { return OptionalHandle<Node> { }; }
    Handle<Node> node = tree.Root();
    while (true) {
        const List<Handle<Node>>& children = tree.children[node];
        const List<Handle<Node>>::const_iterator node_iterator = std::ranges::find_if(children, position_inside_node, std::identity { });
        if (node_iterator == std::end(children)) { break; }
        node = *node_iterator;
    }
    while (tree.styles[node].background_color.a == 0U && !tree.styles[node].texture.IsValid()) {
        if (node == tree.Root()) { return OptionalHandle<Node> { }; }
        node = tree.parents[node];
    }
    return OptionalHandle<Node> { node.id };
}
HoveredType NodeAt(const uint2 mouse_position) {
    for (const auto [i, tree] : std::views::zip(std::views::iota(0u), data.Get<NodeTree>())) {
        const OptionalHandle<Node> node = NodeAt(tree, mouse_position);
        if (node.IsValid()) { return NodeReference { .tree = Handle { data.Get<NodeTree>().IndexToHandle(static_cast<u32>(i)) }, .node = node.GetHandle() }; }
    }
    return std::nullopt;
}
void RecalculateTreeLayout(NodeTree& tree, Handle<SubtreeRoot> subtree_root) {
    auto pixels_gap = [&tree](const Handle<Node> node, const u32 gap) -> u32 { return tree.children[node].empty() ? 0U : gap * (tree.children[node].size() - 1U); };
    auto get_major = [](const uint2 point, const FlexDirection direction) -> u32 { return direction == horizontal ? point.x : point.y; };
    auto get_minor = [](const uint2 point, const FlexDirection direction) -> u32 { return direction == horizontal ? point.y : point.x; };
    auto get_major_layout = [](NodeStyle& node_style, const FlexDirection direction) -> LayoutLength& { return direction == horizontal ? node_style.width : node_style.height; };
    auto get_minor_layout = [](NodeStyle& node_style, const FlexDirection direction) -> LayoutLength& { return direction == horizontal ? node_style.height : node_style.width; };
    auto get_major_pixels_taken_by_children = [&tree, &get_major](const Handle<Node> node, const FlexDirection direction) -> u32 {
        auto get_major_outer_box_size = [&tree, &get_major, &direction](const Handle<Node> child) -> u32 { return get_major(tree.styles[child].OuterBoxSize(), direction); };
        return std::ranges::fold_left(tree.children[node] | std::views::transform(get_major_outer_box_size), 0U, std::plus { });
    };

    // root
    Handle<Node> root { subtree_root.id };
    if (root == tree.Root()) {
        NodeStyle& root_style = tree.styles[root];
        if (root_style.width.constraint == LayoutLength::parent_constraint) { root_style.width.resolved = singleton.Get<WindowState>().screen_size.x - root_style.position.x; }
        if (root_style.height.constraint == LayoutLength::parent_constraint) { root_style.height.resolved = singleton.Get<WindowState>().screen_size.y - root_style.position.y; }
    }

    // nodes ordered
    List nodes { root };
    for (u32 i = 0U; i < nodes.size(); ++i) { nodes.append_range(tree.children[nodes[i]]); }

    // text
    for (const Handle node : nodes) {
        NodeStyle& node_style = tree.styles[node];
        NodeProperties& node_properties = tree.node_properties[node];
        UniquePointer<TTF_Text, DestroyText>& ttf_text = tree.node_ttf_texts[node];
        if (node_properties.text.empty()) {
            ttf_text.Reset();
            continue;
        }

        const Font& font = singleton.Get<FontCollection>().GetFont(node_properties.font_size);
        if (ttf_text.Get() == nullptr) {
            ttf_text.Reset(TTF_CreateText(singleton.Get<WindowState>().text_engine, font.ToSDL(), node_properties.text.c_str(), node_properties.text.size()));
        } else {
            TTF_SetTextString(ttf_text.Get(), node_properties.text.c_str(), node_properties.text.size());
            TTF_SetTextFont(ttf_text.Get(), font.ToSDL());
        }
        const SDL_Color color = node_style.background_color;
        (void)TTF_SetTextColor(ttf_text.Get(), color.r, color.g, color.b, color.a);
    }

// hug bottom up
#define BREAKPOINT 1
#if BREAKPOINT
    auto reversed_nodes = nodes | std::views::reverse | std::ranges::to<std::vector>();
#else
    auto reversed_nodes = nodes | std::views::reverse;
#endif
    for (const Handle node : reversed_nodes) {
        NodeStyle& node_style = tree.styles[node];
        NodeProperties& node_properties = tree.node_properties[node];
        if (node_style.width.constraint != LayoutLength::child_constraint && node_style.height.constraint != LayoutLength::child_constraint) { continue; }

        uint2 text_size { 0U, 0U };
        if (!node_properties.text.empty()) { (void)TTF_GetTextSize(tree.node_ttf_texts[node].Get(), reinterpret_cast<i32*>(&text_size.x), reinterpret_cast<i32*>(&text_size.y)); }
        LayoutLength& major_layout = get_major_layout(node_style, node_style.direction);
        if (major_layout.constraint == LayoutLength::child_constraint) {
            const u32 children_major = get_major_pixels_taken_by_children(node, node_style.direction);
            major_layout.resolved = children_major + pixels_gap(node, node_style.resolved_gap) + get_major(text_size, node_style.direction) + get_major(node_style.NonContentSize2(), node_style.direction);
        }
        LayoutLength& minor_layout = get_minor_layout(node_style, node_style.direction);
        if (minor_layout.constraint == LayoutLength::child_constraint) {
            auto get_minor_outer_box_size = [&tree, get_minor, &node_style](const Handle<Node> child) -> u32 { return get_minor(tree.styles[child].OuterBoxSize(), node_style.direction); };
            const u32 max_minor = tree.children[node].empty() ? 0U : std::ranges::max(tree.children[node] | std::views::transform(get_minor_outer_box_size));
            minor_layout.resolved = std::max(max_minor, get_minor(text_size, node_style.direction)) + get_minor(node_style.NonContentSize2(), node_style.direction);
        }
    }

    // fill top down
    List<Handle<Node>> parent_constrained { };
    for (const Handle node : nodes) {
        parent_constrained.clear();
        const NodeStyle& node_style = tree.styles[node];

        u32 pixels_taken_major_axis = pixels_gap(node, node_style.resolved_gap);
        for (const Handle child : tree.children[node]) {
            NodeStyle& child_style = tree.styles[child];
            LayoutLength& child_major_layout = get_major_layout(child_style, node_style.direction);
            if (child_major_layout.constraint == LayoutLength::parent_constraint) {
                parent_constrained.push_back(child);
            } else {
                pixels_taken_major_axis += get_major(child_style.OuterBoxSize(), node_style.direction);
            }
        }
        if (parent_constrained.size() > 0U) {
            if (pixels_taken_major_axis >= get_major(node_style.InnerBoxSize(), node_style.direction)) {
                for (const Handle child : parent_constrained) {
                    constexpr u32 min_pixel_size = 10U;
                    get_major_layout(tree.styles[child], node_style.direction).resolved = min_pixel_size;
                }
                continue;
            }
            const auto [pixels_per, left_over] = math::Div(get_major(node_style.InnerBoxSize(), node_style.direction) - pixels_taken_major_axis, parent_constrained.size());
            for (const Handle child : parent_constrained) { get_major_layout(tree.styles[child], node_style.direction).resolved = pixels_per; }
            get_major_layout(tree.styles[parent_constrained[0U]], node_style.direction).resolved += left_over;
        }

        auto children =
            tree.children[node] | std::views::filter([&tree, &node_style, &get_minor_layout](const Handle<Node> child) -> bool { return get_minor_layout(tree.styles[child], node_style.direction).constraint == LayoutLength::parent_constraint; });
        for (const Handle child : children) { get_minor_layout(tree.styles[child], node_style.direction).resolved = get_minor(node_style.InnerBoxSize(), node_style.direction); }
    }

    // position top down
    for (const Handle node : nodes) {
        NodeStyle& style = tree.styles[node];
        const u32 minor_position = get_minor(style.InnerBoxPosition(), style.direction);
        u32 major_position = get_major(style.InnerBoxPosition(), style.direction);
        const u32 node_major_size = get_major(style.InnerBoxSize(), style.direction);
        const u32 children_major_size = get_major_pixels_taken_by_children(node, style.direction);
        if (style.gap_auto) {
            if (children_major_size < node_major_size && tree.children[node].size() > 1) {
                style.resolved_gap = (node_major_size - children_major_size) / (tree.children[node].size() - 1U);
            } else {
                style.resolved_gap = 0U;
            }
        }
        float2 factor;
        switch (style.alignment) {
            case top_left: factor = float2 { 0.0F, 0.0F }; break;
            case top_center: factor = float2 { 0.5F, 0.0F }; break;
            case top_right: factor = float2 { 1.0F, 0.0F }; break;
            case left: factor = float2 { 0.0F, 0.5F }; break;
            case center: factor = float2 { 0.5F, 0.5F }; break;
            case right: factor = float2 { 1.0F, 0.5F }; break;
            case bottom_left: factor = float2 { 0.0F, 1.0F }; break;
            case bottom_center: factor = float2 { 0.5F, 1.0F }; break;
            case bottom_right: factor = float2 { 1.0F, 1.0F }; break;
        }
        const f32 factor_major = style.direction == horizontal ? factor.x : factor.y;
        const f32 factor_minor = style.direction == horizontal ? factor.y : factor.x;
        const u32 major_pixels_left = node_major_size - children_major_size - style.resolved_gap * (tree.children[node].size() - 1U);
        major_position += static_cast<u32>(major_pixels_left * factor_major);
        for (const Handle child : tree.children[node]) {
            NodeStyle& child_style = tree.styles[child];
            if (style.direction == horizontal) {
                const u32 child_minor_position = minor_position + static_cast<u32>((style.InnerBoxSize().y - child_style.OuterBoxSize().y) * factor_minor);
                child_style.position = uint2 { major_position, child_minor_position };
                major_position += child_style.OuterBoxSize().x + style.resolved_gap;
            } else {
                const u32 child_minor_position = minor_position + static_cast<u32>((style.InnerBoxSize().x - child_style.OuterBoxSize().x) * factor_minor);
                child_style.position = uint2 { child_minor_position, major_position };
                major_position += child_style.OuterBoxSize().y + style.resolved_gap;
            }
        }
    }

    // bounding box
    for (const Handle node : reversed_nodes) {
        NodeStyle& style = tree.styles[node];
        uint2 start_position = style.OuterBoxPosition();
        uint2 end_position = style.OuterBoxEndPosition();
        for (const Handle child : tree.children[node]) {
            NodeStyle& child_style = tree.styles[child];
            uint2 child_start_position = child_style.OuterBoxPosition();
            uint2 child_end_position = child_style.OuterBoxEndPosition();
            start_position.x = std::min(child_start_position.x, start_position.x);
            start_position.y = std::min(child_start_position.y, start_position.y);
            end_position.x = std::max(child_end_position.x, end_position.x);
            end_position.y = std::max(child_end_position.y, end_position.y);
        }
        const uint2 size = end_position - start_position;
        style.bounding_box = { .x = static_cast<f32>(start_position.x), .y = static_cast<f32>(start_position.y), .w = static_cast<f32>(size.x), .h = static_cast<f32>(size.y) };
    }

    // subtree
    for (const Handle node : nodes) { //  is part of a subtree if the parent is hugging. Else its parent node is the subtree root (siblings depend on nodes width)
        const Handle parent { tree.parents[node] };
        const NodeStyle& parent_style = tree.styles[parent];
        tree.subtree_roots[node] = parent_style.width.constraint == LayoutLength::child_constraint || parent_style.height.constraint == LayoutLength::child_constraint ? tree.subtree_roots[parent] : Handle<SubtreeRoot> { parent.id };
    }
}
void AddNodeToFrameElement(NodeTree& tree, const Handle<Node> node) {
    const auto add_type = [&tree](const ElementType new_type) -> void {
        auto& [element_type, count] = tree.frame_elements.items_in_a_row.back();
        if (element_type == new_type && count < UINT8_MAX) {
            ++count;
        } else {
            tree.frame_elements.items_in_a_row.push_back({ .type = new_type, .count = 1U });
        }
    };

    const NodeStyle& style = tree.styles[node];
    const NodeProperties& properties = tree.node_properties[node];
    if (!properties.text.empty()) {
        add_type(ElementType::text);
        tree.frame_elements.texts.push_back(TextElement { .text = tree.node_ttf_texts[node].Get(), .position = { static_cast<f32>(style.InnerBoxPosition().x), static_cast<f32>(style.InnerBoxPosition().y) } });
    } else if (style.texture.IsValid()) {
        add_type(ElementType::texture);
        tree.frame_elements.textures.push_back(TextureElement { .rect = style.OuterRect(), .texture = data[style.texture.GetHandle()].ToSDL() });
    } else if (style.background_color.a != 0U) {
        add_type(ElementType::rectangle);
        tree.frame_elements.rectangles.push_back(RectangleElement { .color = style.background_color, .rect = style.OuterRect() });
    }
}
const FrameElements& GetFrameElements(NodeTree& tree) {
    if (!tree.dirty_tree && !tree.dirty_subtree.IsValid()) { return tree.frame_elements; }
    RecalculateTreeLayout(tree, tree.dirty_tree ? tree.subtree_roots.front() : tree.dirty_subtree.GetHandle());

    tree.dirty_tree = false;
    tree.dirty_subtree.Reset();
    tree.frame_elements.items_in_a_row.clear();
    tree.frame_elements.rectangles.clear();
    tree.frame_elements.textures.clear();
    tree.frame_elements.texts.clear();

    std::stack<Handle<Node>> nodes;
    nodes.push(tree.Root());
    tree.frame_elements.items_in_a_row.push_back(VariantIndex { .type = ElementType::rectangle, .count = 0U });
    while (!nodes.empty()) {
        const Handle<Node> node = nodes.top();
        nodes.pop();
        nodes.push_range(tree.children[node]);
        AddNodeToFrameElement(tree, node);
    }

    return tree.frame_elements;
}

void Hover(const NodeReference node_reference) {
    // Logger().Log("Hover {}", NodeSystem::node_trees[node_reference.tree].node_properties[node_reference.node].text);
    const NodeProperties& properties = data[node_reference.tree].node_properties[node_reference.node];
    if (properties.on_hover) { properties.on_hover(node_reference); }
}
void HoverOut(const NodeReference node_reference) {
    // Logger().Log("Hover Out {}", NodeSystem::node_trees[node_reference.tree].node_properties[node_reference.node].text);
    const NodeProperties& properties = data[node_reference.tree].node_properties[node_reference.node];
    if (properties.on_hover_out) { properties.on_hover_out(node_reference); }
}
void Click(const NodeReference node_reference) {
    // Logger().Log("Clicked {}", data[node_reference.tree].node_properties[node_reference.node].text);
    const NodeProperties& properties = data[node_reference.tree].node_properties[node_reference.node];
    if (properties.on_click) { properties.on_click(node_reference); }
}
void Propagate(NodeReference node_reference, const NodeReaction& reaction) {
    const NodeTree& tree = data[node_reference.tree];
    const Handle<Node> root = tree.Root();
    while (true) {
        std::invoke(reaction, node_reference);
        if (node_reference.node.id == root.id) { break; };
        node_reference.node = tree.parents[node_reference.node];
    }
}
void NodeInputSystem::operator()() const {
    HoveredType& hovered = singleton.Get<HoveredType>();
    HandleList<NodeTree>& trees = data.Get<NodeTree>();
    if (singleton.Get<InputState>().left_mouse_down && hovered.has_value()) { Propagate(hovered.value(), Click); }

    if (hovered.has_value() && !trees[hovered->tree].styles.ValidHandle(hovered->node)) { hovered = std::nullopt; }
    const HoveredType previous_hovered = hovered;
    hovered = NodeAt(singleton.Get<InputState>().mouse_position);

    if (hovered.has_value() && previous_hovered.has_value() && hovered->tree.id == previous_hovered->tree.id && hovered->node.id == previous_hovered->node.id) { return; }

    if (previous_hovered.has_value()) {
        Propagate(previous_hovered.value(), HoverOut);
        trees[previous_hovered->tree].MarkDirty();
    }
    if (hovered.has_value()) {
        Propagate(hovered.value(), Hover);
        trees[hovered->tree].MarkDirty();
    }
}
void NodeRenderSystem::operator()() const {
    for (NodeTree& tree : data.Get<NodeTree>() | std::views::reverse | std::views::filter(&NodeTree::GetDisplay)) {
        const FrameElements& frame_elements = GetFrameElements(tree);
        std::span rectangles { frame_elements.rectangles };
        std::span textures { frame_elements.textures };
        std::span texts { frame_elements.texts };
        for (const auto [type, count] : frame_elements.items_in_a_row) {
            switch (type) {
                case ElementType::rectangle: {
                    for (const RectangleElement& element : rectangles.first(count)) {
                        (void)SDL_SetRenderDrawColor(singleton.Get<WindowState>().renderer, element.color.r, element.color.g, element.color.b, element.color.a);
                        (void)SDL_RenderFillRect(singleton.Get<WindowState>().renderer, &element.rect);
                    }
                    rectangles = rectangles.subspan(count);
                    break;
                }
                case ElementType::texture: {
                    for (const TextureElement& element : textures.first(count)) { (void)SDL_RenderTexture(singleton.Get<WindowState>().renderer, element.texture, nullptr, &element.rect); }
                    textures = textures.subspan(count);
                    break;
                }
                case ElementType::text: {
                    for (const TextElement& text : texts.first(count)) { (void)TTF_DrawRendererText(text.text, text.position.x, text.position.y); }
                    texts = texts.subspan(count);
                    break;
                }
            }
        }
        STL_ASSERT(rectangles.empty(), "Textures size mismatch");
        STL_ASSERT(textures.empty(), "Textures size mismatch");
        STL_ASSERT(texts.empty(), "Texts size mismatch");
    }
}
} // namespace pce::ui
