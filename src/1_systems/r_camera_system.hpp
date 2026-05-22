#pragma once
#include <SDL3/SDL_keycode.h>

#include "0_engine/g_globals.hpp"
#include "0_engine/u_types.hpp"
#include "0_engine/u_util.hpp"
#include "i_input_system.hpp"

namespace pce {
struct CameraState {
    float2 world_position { 1000.0F, 300.0F };
    f32 scale { 40.0F };
    f32 target_scale { 40.0F };
    float2 zoom_anchor_world { 0.0F, 0.0F };
    [[nodiscard]] constexpr float2 ScreenToWorld(const int2 screen) const { return (float2 { screen } - world_position) / scale; }
    [[nodiscard]] constexpr int2 WorldToScreen(const float2 world) const { return int2 { world * scale + world_position }; }
};

struct CameraSystem {
    static constexpr f32 PAN_SPEED = 8.0F; // pixels per frame
    static constexpr f32 ZOOM_FACTOR = 1.1F;
    static constexpr f32 ZOOM_MIN = 5.0F;
    static constexpr f32 ZOOM_MAX = 200.0F;
    static constexpr f32 ZOOM_LERP = 0.15F; // smoothing per frame

    void operator()() const {
        pce::CameraState& camera_state = Singleton::Get<CameraState>();
        pce::InputState& input_state = Singleton::Get<InputState>();

        if (input_state.keys[SDLK_LEFT]) { camera_state.world_position.x += PAN_SPEED; }
        if (input_state.keys[SDLK_RIGHT]) { camera_state.world_position.x -= PAN_SPEED; }
        if (input_state.keys[SDLK_UP]) { camera_state.world_position.y += PAN_SPEED; }
        if (input_state.keys[SDLK_DOWN]) { camera_state.world_position.y -= PAN_SPEED; }

        if (input_state.mouse_wheel_y != 0.0F) {
            camera_state.zoom_anchor_world = camera_state.ScreenToWorld(input_state.mouse_position);
            const f32 factor = input_state.mouse_wheel_y > 0.0F ? ZOOM_FACTOR : 1.0F / ZOOM_FACTOR;
            camera_state.target_scale = math::Clamp(camera_state.target_scale * factor, ZOOM_MIN, ZOOM_MAX);
        }

        if (const f32 diff = camera_state.target_scale - camera_state.scale; math::Abs(diff) > 0.01F) {
            const f32 old_scale = camera_state.scale;
            camera_state.scale += diff * ZOOM_LERP;
            camera_state.world_position -= camera_state.zoom_anchor_world * (camera_state.scale - old_scale);
        } else {
            camera_state.scale = camera_state.target_scale;
        }
    }
};
}
