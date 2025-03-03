#pragma once
#include <functional>

#include <numbers>

#include "0_engine/u_collections.hpp"
#include "0_engine/u_types.hpp"

namespace pce {
inline f32 EaseInSine(f32 t);
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
    static Handle<Animation> Register(const AnimationDesc& animation_desc);
    static void StartAnimation(Handle<Animation> animation_handle);
    static b8 IsRunning(Handle<Animation> animation_handle);
    void operator()() const;
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
    void NewParticle(const ParticleProtocolHandle protocol_handle, Particle&& particle) { GetParticles(protocol_handle).PushBack(particle); }
    void operator()() const { for (u32 i = 0; i < particles.Size(); i++) { } }
};

inline f32 EaseInSine(const f32 t) { return 1 - math::Cos(t * math::PI * 0.5F); }
inline f32 EaseOutSine(const f32 t) { return 1 - math::Sin(t * math::PI * 0.5F); }
inline f32 EaseInOutSine(const f32 t) { return -(math::Cos(math::PI * t) - 1) * 0.5F; }
inline Handle<Animation> AnimationSystem::Register(const AnimationDesc& animation_desc) {
    Logger().Log("Size of action {} and action {} size {}", sizeof(animation_desc), sizeof(animation_desc.action), data.Get<Animation>().Size());
    HandleList<Animation>& animations = data.Get<Animation>();
    const Animation animation { .action = animation_desc.action, .offset_ms = static_cast<u32>(SDL_GetTicks()), .duration_ms = animation_desc.duration_ms, .state = animation_desc.state };
    for (Handle<Animation> handle { 0U }; handle.id < animations.Size(); ++handle.id) {
        if (animations[handle].state == AnimationState::recycle) {
            animations[handle] = animation;
            return handle;
        }
    }
    return animations.PushBack(animation);
}
inline void AnimationSystem::StartAnimation(const Handle<Animation> animation_handle) {
    Animation& animation = data[animation_handle];
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
inline b8 AnimationSystem::IsRunning(Handle<Animation> animation_handle) {
    switch (data[animation_handle].state) {
        case AnimationState::run_once:
        case AnimationState::repeat:
        case AnimationState::keep_alive:
            return true;
        case AnimationState::recycle:
        case AnimationState::keep_alive_stopped:
            return false;
        default: STL_ASSERT(false, "Unknown animation state {}", data[animation_handle].state);
            return false;
    }
}
inline void AnimationSystem::operator()() const {
    const u32 current_ms = static_cast<u32>(SDL_GetTicks());
    for (Animation& animation : data.Get<Animation>()) {
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
        const f32 value = EaseInOutSine(t);
        animation.action(value);
    }
}
} // namespace pce
