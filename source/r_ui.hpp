#pragma once

#include <r_colors.hpp>

#include "r_engine.hpp"
#include "r_ui_node.hpp"
#include "u_collections.hpp"
#include "u_types.hpp"

#include <SDL3/SDL_render.h>
#include <SDL3_ttf/SDL_ttf.h>

namespace pce::ui {
class TickFrame {
    HandleOptional<Node> tick_handle { };

public:
    Handle<NodeTree> tree_handle { NodeSystem::node_trees.EmplaceBack() };
    TickFrame() { tick_handle = B(tree_handle, hug, { 10U, 0U }).Text("Tick", FontSizes::tiny).Fill(colors::radiant_orange).Build(); }
    void SetInfo(u32 tick, u32 tps, u32 fps) { NodeSystem::node_trees[tree_handle].node_properties[tick_handle.GetHandle()].text = std::format("Tick: {:>8}   |   TPS: {:>4}   |   FPS: {:>4}", tick, tps, fps); }
};
struct InspectorFrame {
    Handle<NodeTree> tree_handle { NodeSystem::node_trees.EmplaceBack() };
    void ShowElementStructure(HoveredType hovered);
};
struct TestFrame {
    Handle<NodeTree> tree_handle { NodeSystem::node_trees.EmplaceBack() };
    TestFrame();
};

struct DebugSystem {
    TickFrame tick_frame { };
    InspectorFrame inspector_frame { };

    void operator()(const TickSystem& tick_system) {
        if (InputSystem::input_table.left_mouse_down) {
            NodeSystem::node_trees[inspector_frame.tree_handle].MarkDirty();
            inspector_frame.ShowElementStructure(NodeSystem::hovered);
        }
        NodeSystem::node_trees[tick_frame.tree_handle].MarkDirty();
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
struct AnimationSystem {
    static constexpr u32 DEFAULT_COUNT = 128U;
    HandleList<Animation> animations { DEFAULT_COUNT };

    Handle<Animation> Register(const AnimationDesc& animation_desc);
    void StartAnimation(Handle<Animation> animation_handle);
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
        return ParticleProtocolHandle { particles.Size() - 1U };
    }
    [[nodiscard]] ParticleProtocol& GetParticleProtocol(const ParticleProtocolHandle protocol_handle) { return protocols[protocol_handle.id]; }
    [[nodiscard]] List<Particle>& GetParticles(const ParticleProtocolHandle protocol_handle) { return particles[protocol_handle.id]; }
    void NewParticle(ParticleProtocolHandle protocol_handle, Particle&& particle) { GetParticles(protocol_handle).PushBack(particle); }
    void operator()() { for (u32 i = 0; i < particles.Size(); i++) { } }
};
} // namespace pce::ui
