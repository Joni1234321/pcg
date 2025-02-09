#pragma once

#include <r_colors.hpp>

#include "r_engine.hpp"
#include "r_ui_node.hpp"
#include "u_collections.hpp"

#include <SDL3/SDL_render.h>
#include <SDL3_ttf/SDL_ttf.h>

namespace pce::ui {
using HoveredType = std::optional<WeakNodeReference>;
static const RelativePath font_path { "font.ttf" };
static const RelativePath font_bold_path { "TitilliumWeb-SemiBold.ttf" };
struct DestroyRenderTextEngine {
    void operator()(TTF_TextEngine* engine) const {
        Logger().Destroyed("TTF_TextEngine");
        TTF_DestroyRendererTextEngine(engine);
    }
};
struct NodeRenderSystem {
    HoveredType hovered { };
    FontCollection font { assets::Asset(font_path) };
    FontCollection font_bold { assets::Asset(font_bold_path) };
    UniquePointer<TTF_TextEngine, DestroyRenderTextEngine> text_engine { TTF_CreateRendererTextEngine(Window::renderer) };
    static List<std::reference_wrapper<NodeTree>> node_trees;
    NodeRenderSystem() { node_trees.Clear(); };
    void HoverClickEvents(const InputSystem& input_system);
    void RenderTrees(SDL_Renderer* renderer);

    [[nodiscard]] NodeTree& AddNodeTree(NodeTree& node_tree);
    [[nodiscard]] auto GetNodeTrees() const { return node_trees | std::views::transform(&std::reference_wrapper<NodeTree>::get); }
};
class TickFrame {
    NodeHandleOptional tick_handle { };

public:
    NodeTree tree;
    TickFrame() { tick_handle = B(tree, hug, { 10U, 0U }).Text("Tick", FontSizes::tiny).Fill(colors::radiant_orange).Build(); }
    void SetInfo(u32 tick, u32 tps, u32 fps) { tree.GetProperties(tick_handle.GetHandle()).text = std::format("Tick: {:>8}   |   TPS: {:>4}   |   FPS: {:>4}", tick, tps, fps); }
};
struct InspectorFrame {
    NodeTree tree;
    void ShowElementStructure(const HoveredType& hovered);
};
struct TestFrame {
    NodeTree tree;
    TestFrame();
};

struct DebugSystem {
    TickFrame tick_frame { };
    InspectorFrame debug_frame { };

    explicit DebugSystem() {
        NodeRenderSystem::node_trees.EmplaceBack(tick_frame.tree);
        NodeRenderSystem::node_trees.EmplaceBack(debug_frame.tree);
    }
    void operator()(const InputSystem& input_system, const TickSystem& tick_system, const NodeRenderSystem& node_render_system) {
        if (input_system.LeftMouseDown()) {
            debug_frame.tree.MarkDirty();
            debug_frame.ShowElementStructure(node_render_system.hovered);
        }
        tick_frame.tree.MarkDirty();
        tick_frame.SetInfo(tick_system.tick.Value(), 1.0F / tick_system.tick_time, 1.0F / tick_system.delta_time);
    }
};
inline f32 EasingSin(f32 t);
inline f32 EasingCos(f32 t);
enum class AnimationState : u8 { run_once, recycle, repeat, keep_alive, keep_alive_stopped };
struct AnimationDesc {
    std::function<void(f32)> action;
    u32 duration_ms;
    AnimationState state;
};
struct Animation {
    std::function<void(f32)> action;
    u32 offset_ms;
    u32 duration_ms;
    AnimationState state;
};
struct AnimationHandle {
    u32 id;
};
struct AnimationSystem {
    static constexpr u32 DEFAULT_COUNT = 128U;
    List<Animation> animations { DEFAULT_COUNT };

    AnimationHandle Register(const AnimationDesc& animation_desc);
    [[nodiscard]] Animation& GetAnimation(AnimationHandle animation_handle);
    void StartAnimation(AnimationHandle animation_handle);
    void operator()();
};

struct Particle {
    uint2 position;
};
struct ParticleProtocol {
    int count;
    uint2 velocity;
};
struct ParticleProtocolHandle {
    u32 id;
};
struct ParticleSystem {
    static constexpr u32 DEFAULT_COUNT = 128U;
    List<ParticleProtocol> protocols { DEFAULT_COUNT };
    List<List<Particle>> particles { DEFAULT_COUNT };

    ParticleProtocolHandle Register(const ParticleProtocol& protocol) {
        protocols.PushBack(protocol);
        particles.EmplaceBack(DEFAULT_COUNT);
        return ParticleProtocolHandle { particles.Size() - 1U };
    }
    [[nodiscard]] ParticleProtocol& GetParticleProtocol(const ParticleProtocolHandle protocol_handle) { return protocols[protocol_handle.id]; }
    [[nodiscard]] List<Particle>& GetParticles(const ParticleProtocolHandle protocol_handle) { return particles[protocol_handle.id]; }
    void NewParticle(ParticleProtocolHandle protocol_handle, Particle&& particle) { GetParticles(protocol_handle).PushBack(particle); }
    void operator()() {
        for (u32 i = 0; i < particles.Size(); i++) {
        }
    }
};
} // namespace pce::ui
