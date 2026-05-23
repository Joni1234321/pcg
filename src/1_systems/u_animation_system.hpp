#pragma once
#include <functional>

#include "0_engine/u_collections.hpp"
#include "0_engine/u_ecs.hpp"
#include "0_engine/u_types.hpp"

namespace pce {
enum class AnimationState : u8 { once, recycle, repeat, persistent, persistent_stopped };
struct AnimationDesc {
    std::function<void(f32)> action;
    miliseconds32 duration;
    AnimationState state;
};
struct Animation {
    std::function<void(f32)> action;
    miliseconds32 start;
    miliseconds32 duration;
    AnimationState state;
};
struct AnimationSystem {
    static Handle<Animation> Register(const AnimationDesc& animation_desc);
    static void StartAnimation(Handle<Animation> animation_handle);
    static b8 IsRunning(Handle<Animation> animation_handle);
    void operator()() const;
};

// how it works
// i want to say spawn particle (x, y)
// then the particlesystem will push it upwards until it expires
struct Particle {
    float2 position { 0.0F, 0.0F };
    std::unique_ptr<TTF_Text, DestroyText> text { nullptr };
    miliseconds32 duration { 0U };
    miliseconds32 start { TimeNowMS() };
};
struct ParticleEmitter {
    static constexpr u32 DEFAULT_COUNT = 128U;

    float2 velocity;
    ParticlePool<Particle> particles { DEFAULT_COUNT };
};

struct ParticleSystem {
    void operator()() const {
        constexpr SDL_Color color { colors::black };
        const f32 delta_time { Singleton::Get<TickState>().delta_time };
        const miliseconds32 current_ms { TimeNowMS() };
        (void)SDL_SetRenderDrawColor(Singleton::Get<WindowState>().renderer, color.r, color.g, color.b, color.a);
        for (ParticleEmitter& emitter : globalData.Get<ParticleEmitter>()) {
            for (Particle& particle : emitter.particles.items | std::views::reverse) {
                TTF_DrawRendererText(particle.text.get(), particle.position.x, particle.position.y);
                particle.position += emitter.velocity * float2 { delta_time };
                if (current_ms - particle.start > particle.duration) { emitter.particles.SwapBackErase(particle); }
            }
            emitter.particles.ApplyErase();
        }
    }
};

inline f32 EaseInSine(const f32 t) { return 1.0F - math::Cos(t * math::PI * 0.5F); }
inline f32 EaseOutSine(const f32 t) { return 1.0F - math::Sin(t * math::PI * 0.5F); }
inline f32 EaseInOutSine(const f32 t) { return -(math::Cos(math::PI * t) - 1.0F) * 0.5F; }
inline Handle<Animation> AnimationSystem::Register(const AnimationDesc& animation_desc) {
    const Animation animation { .action = animation_desc.action, .start = TimeNowMS(), .duration = animation_desc.duration, .state = animation_desc.state };
    HandleList<Animation>& animations = globalData.Get<Animation>();
    const auto it = std::ranges::find(animations, AnimationState::repeat, &Animation::state);
    if (it == std::end(animations)) { return animations.PushBack(animation); }
    *it = animation;
    return animations.IteratorToHandle(it);
}
inline void AnimationSystem::StartAnimation(const Handle<Animation> animation_handle) {
    Animation& animation = globalData[animation_handle];
    animation.start = TimeNowMS();
    switch (animation.state) {
        case AnimationState::once: break;
        case AnimationState::recycle: break;
        case AnimationState::repeat: break;
        case AnimationState::persistent: break;
        case AnimationState::persistent_stopped: animation.state = AnimationState::persistent; break;
    }
}
inline b8 AnimationSystem::IsRunning(const Handle<Animation> animation_handle) {
    switch (globalData[animation_handle].state) {
        case AnimationState::once:
        case AnimationState::repeat:
        case AnimationState::persistent: return true;
        case AnimationState::recycle:
        case AnimationState::persistent_stopped: return false;
        default: STL_ASSERT(false, std::format("Unknown animation state {}", data[animation_handle].state)); return false;
    }
}
inline void AnimationSystem::operator()() const {
    const miliseconds32 current_ms { TimeNowMS() };
    for (Animation& animation : globalData.Get<Animation>()) {
        f32 t = static_cast<f32>((current_ms - animation.start).value) / static_cast<f32>(animation.duration.value);
        switch (animation.state) {
            case AnimationState::once:
                if (t >= 1.0F) {
                    t = 1.0F;
                    animation.state = AnimationState::recycle;
                }
                break;
            case AnimationState::recycle: continue;
            case AnimationState::repeat: break;
            case AnimationState::persistent:
                if (t >= 1.0F) {
                    t = 1.0F;
                    animation.state = AnimationState::persistent_stopped;
                }
                break;
            case AnimationState::persistent_stopped: continue;
        }
        const f32 value = EaseInOutSine(t);
        animation.action(value);
    }
}
} // namespace pce
