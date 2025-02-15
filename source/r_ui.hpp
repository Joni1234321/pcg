#pragma once

#include <engine/u_colors.hpp>

#include "engine/r_engine.hpp"
#include "r_ui_node.hpp"
#include "engine/u_collections.hpp"
#include "engine/u_types.hpp"

#include <SDL3_ttf/SDL_ttf.h>

namespace pce::ui {
struct TickFrame {
    Handle<NodeTree> tree_handle { NodeRenderSystem::node_trees.EmplaceBack() };
    TickFrame() { DisplayInfo(); }
    void DisplayInfo() {
        u32 tick = TickSystem::tick_table.tick.Value();
        u32 tps = 1.0F / TickSystem::tick_table.tick_time;
        u32 fps = 1.0F / TickSystem::tick_table.delta_time;
        NodeRenderSystem::node_trees[tree_handle].Clear();
        Handle<Node> frame = B(tree_handle, hug, {10U, 0U}).Direction(vertical).Build();
        Handle<Node> ticks = B(tree_handle, frame, hug).Text(std::format("Tick: {:>8}   |   TPS: {:>4}   |   FPS: {:>4}", tick, tps, fps), FontSizes::tiny).Fill(colors::radiant_orange).Build();
        Handle<Node> systems = B(tree_handle, frame, hug).Direction(vertical).Gap(10).Build();
        for (const auto [name, ns] : std::views::zip(Orchestra::orchestra_table.names, Orchestra::orchestra_table.nano_seconds)) {
            constexpr f32 THOUSANDTH = 0.001F;
            (void)B(tree_handle, systems, fill).Fill(colors::radiant_orange).Text(std::format("{:.3f}ms | {}", ns * THOUSANDTH * THOUSANDTH, name), FontSizes::tiny).Build();
        }
        NodeRenderSystem::node_trees[tree_handle].MarkDirty();
    }
};
struct InspectorFrame {
    Handle<NodeTree> tree_handle { NodeRenderSystem::node_trees.EmplaceBack() };
    void ShowElementStructure(HoveredType hovered);
};
struct TestFrame {
    Handle<NodeTree> tree_handle { NodeRenderSystem::node_trees.EmplaceBack() };
    TestFrame();
};

struct DebugSystem {
    TickFrame tick_frame { };
    InspectorFrame inspector_frame { };

    void operator()() {
        if (InputSystem::input_table.left_mouse_down) {
            NodeRenderSystem::node_trees[inspector_frame.tree_handle].MarkDirty();
            if (InputSystem::input_table.keys[SDLK_LALT]) { inspector_frame.ShowElementStructure(NodeInputSystem::hovered); }
            else { NodeRenderSystem::node_trees[inspector_frame.tree_handle].Clear(); }
        }
        NodeRenderSystem::node_trees[tick_frame.tree_handle].MarkDirty();
        if (TickSystem::tick_table.tick.Value() % 100 == 0) { tick_frame.DisplayInfo(); }
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
struct AnimationTable {
    static constexpr u32 DEFAULT_COUNT = 128U;
    HandleList<Animation> animations { DEFAULT_COUNT };
};
struct AnimationSystem {
    static AnimationTable animation_table;
    static Handle<Animation> Register(const AnimationDesc& animation_desc);
    static void StartAnimation(Handle<Animation> animation_handle);
    void operator()();
};
inline AnimationTable AnimationSystem::animation_table;

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
        return ParticleProtocolHandle { particles.Size() - 1U };
    }
    [[nodiscard]] ParticleProtocol& GetParticleProtocol(const ParticleProtocolHandle protocol_handle) { return protocols[protocol_handle.id]; }
    [[nodiscard]] List<Particle>& GetParticles(const ParticleProtocolHandle protocol_handle) { return particles[protocol_handle.id]; }
    void NewParticle(ParticleProtocolHandle protocol_handle, Particle&& particle) { GetParticles(protocol_handle).PushBack(particle); }
    void operator()() { for (u32 i = 0; i < particles.Size(); i++) { } }
};
} // namespace pce::ui
