module;

export module pcs.node;

import pce.colors;
import pce.std;
import pce.font;
import pcs.node_data;
import pce.collections;
import pce.sdl;
import pce.globals;

// Function bodies live in r_ui_node.cpp (module pcs.node;). Keeping them out of
// the interface is what makes this module cheap to import: the BMI carries only
// declarations, and consumers unblock as soon as it is written instead of
// waiting for the layout/render code to be parsed and optimised.

export namespace hex::ui {
struct InputNodeSystem {
    void operator()() const;
    ~InputNodeSystem() { Singleton::Get<HoveredType>() = { }; }
};
struct RenderNodeSystem {
    void operator()() const;
    ~RenderNodeSystem() { globalData.Get<NodeTree>().clear(); }
};

class NodeBuilder {
    NodeReference node_reference;
    NodeStyle& style { globalData[node_reference.tree].styles[node_reference.node] };
    NodeProperties& properties { globalData[node_reference.tree].node_properties[node_reference.node] };

public:
    NodeBuilder(Handle<NodeTree> tree, Layout new_layout, uint2 position);
    NodeBuilder(Handle<NodeTree> tree, Handle<Node> parent, Layout new_layout);
    NodeBuilder(NodeReference parent, Layout new_layout);
    [[nodiscard]] NodeBuilder& Name(const String& name);
    [[nodiscard]] NodeBuilder& Fill(Color color);
    [[nodiscard]] NodeBuilder& Texture(Handle<Texture> texture);
    [[nodiscard]] NodeBuilder& Padding(u32 padding);
    [[nodiscard]] NodeBuilder& Padding2(uint2 padding);
    [[nodiscard]] NodeBuilder& Padding4(uint4 padding);
    [[nodiscard]] NodeBuilder& Gap(u32 gap);
    [[nodiscard]] NodeBuilder& GapAuto();
    [[nodiscard]] NodeBuilder& Direction(FlexDirection direction);
    [[nodiscard]] NodeBuilder& Text(const String& string, Color color);
    [[nodiscard]] NodeBuilder& Text(String&& string, Color color);
    [[nodiscard]] NodeBuilder& Text(const String& string, FontSizes font_size, Color color);
    [[nodiscard]] NodeBuilder& Text(String&& string, FontSizes font_size, Color color);
    [[nodiscard]] NodeBuilder& Text(FontSizes font_size, Color color);
    [[nodiscard]] NodeBuilder& Alignment(Alignment alignment);
    [[nodiscard]] NodeBuilder& Right();
    [[nodiscard]] NodeBuilder& Center();
    [[nodiscard]] NodeBuilder& Left();
    [[nodiscard]] NodeBuilder& OnClick(NodeReaction&& reaction);
    Handle<Node> Build() const;
};
using B = NodeBuilder;

struct NodeBuilderHelper {
    NodeReference parent;
    explicit NodeBuilderHelper(const NodeReference parent) : parent { parent } { }
    [[nodiscard]] NodeBuilder Node(const RelativeConstraint constraint) const { return NodeBuilder(parent, Layout { constraint }); }
    [[nodiscard]] NodeBuilder Node(const u32 size) const { return NodeBuilder(parent, Layout { uint2 { size, size } }); }
    [[nodiscard]] NodeBuilder Node(const u32 width, const u32 height) const { return NodeBuilder(parent, Layout { uint2 { width, height } }); }
    [[nodiscard]] NodeBuilder Node(const RelativeConstraint width, const RelativeConstraint height) const { return NodeBuilder(parent, Layout { width, height }); }
    [[nodiscard]] NodeBuilder Node(const u32 width, const RelativeConstraint height) const { return NodeBuilder(parent, Layout { width, height }); }
    [[nodiscard]] NodeBuilder Node(const RelativeConstraint width, const u32 height) const { return NodeBuilder(parent, Layout { width, height }); }
    template <NodeComponent C> [[nodiscard]] C Component() { return C(parent); }
    template <NodeComponent C> [[nodiscard]] C Component(const typename C::Property& property) { return SingleComponent<C>(parent, property); }
    template <NodeComponent C> [[nodiscard]] NodeComponentPool<C> Pool() { return NodeComponentPool<C>(parent); }
};
struct NodeComponentBase {
    NodeReference root;
    explicit NodeComponentBase(const Handle<NodeTree> tree, const Handle<Node> root) : root({ tree, root }) { }

protected:
    [[nodiscard]] NodeBuilderHelper B(const Handle<Node> parent) const { return NodeBuilderHelper(NodeReference(root.tree, parent)); }
    [[nodiscard]] static NodeBuilderHelper B(const NodeReference parent) { return NodeBuilderHelper(parent); }
};
struct Frame {
    Handle<NodeTree> tree { globalData.Create<NodeTree>() };
    Handle<Node> frame { NodeBuilder(tree, fill, uint2 { 0U, 0U }).Build() };

protected:
    [[nodiscard]] NodeBuilderHelper B(const Handle<Node> parent) const { return NodeBuilderHelper(NodeReference(tree, parent)); }
    [[nodiscard]] static NodeBuilderHelper B(const NodeReference parent) { return NodeBuilderHelper(parent); }
};

constexpr Color DEFAULT_TEXT_COLOR = colors::COLOR_BLACK;

HandleOptional<Node> NodeAt(const NodeTree& tree, int2 screen_position);
HoveredType NodeAt(int2 mouse_position);
void RecalculateTreeLayout(NodeTree& tree, Handle<SubtreeRoot> subtree_root);
void AddNodeToFrameElement(NodeTree& tree, Handle<Node> node);
const FrameElements& GetFrameElements(NodeTree& tree);
void Hover(NodeReference node_reference);
void HoverOut(NodeReference node_reference);
void Click(NodeReference node_reference);
void Propagate(NodeReference node_reference, const NodeReaction& reaction);
} // namespace hex::ui
