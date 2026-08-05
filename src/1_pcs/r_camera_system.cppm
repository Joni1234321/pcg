module;

#include <SDL3/SDL_keycode.h>
export module pcs.camera;

import pce.std;
import pce.math;
import pce.globals;
import pce.window_state;
import pcs.input;

import pce.std;

export namespace hex {
struct CameraState {
    float2 world_position { -100.0F, -100.0F };
    f32 scale { 140.0F };
    f32 target_scale { 140.0F };
    float2 zoom_anchor_world { 0.0F, 0.0F };
    float2 drag_last_pos { 0, 0 };
    float2 map_world_min { 0.0F, 0.0F };
    float2 map_world_max { 0.0F, 0.0F };
    [[nodiscard]] constexpr float2 ScreenToWorld(const float2 screen) const { return (screen + world_position) / float2 { scale }; }
    [[nodiscard]] constexpr float2 WorldToScreen(const float2 world) const { return world * float2 { scale } - world_position; }
};

struct CameraSystem {
    static constexpr f32 PAN_SPEED = 8.0F;
    static constexpr f32 ZOOM_FACTOR = 1.5F;
    static constexpr f32 ZOOM_MIN = 5.0F;
    static constexpr f32 ZOOM_MAX = 200.0F;
    static constexpr f32 ZOOM_LERP = 0.15F;

    void operator()() const {
        CameraState& camera_state = Singleton::Get<CameraState>();
        InputState& input_state = Singleton::Get<InputState>();

        if (input_state.keys[SDLK_LEFT]) { camera_state.world_position.x -= PAN_SPEED; }
        if (input_state.keys[SDLK_RIGHT]) { camera_state.world_position.x += PAN_SPEED; }
        if (input_state.keys[SDLK_UP]) { camera_state.world_position.y -= PAN_SPEED; }
        if (input_state.keys[SDLK_DOWN]) { camera_state.world_position.y += PAN_SPEED; }

        // mouse drag
        const float2 mouse_delta = input_state.mouse_position - camera_state.drag_last_pos;
        camera_state.drag_last_pos = input_state.mouse_position;
        if (input_state.left_mouse | input_state.right_mouse) { camera_state.world_position -= float2 { mouse_delta }; }

        // zoom
        if (input_state.mouse_wheel_y != 0.0F) {
            camera_state.zoom_anchor_world = camera_state.ScreenToWorld(input_state.mouse_position);
            const f32 factor = input_state.mouse_wheel_y > 0.0F ? ZOOM_FACTOR : 1.0F / ZOOM_FACTOR;
            camera_state.target_scale = math::Clamp(camera_state.target_scale * factor, ZOOM_MIN, ZOOM_MAX);
        }
        if (const f32 diff = camera_state.target_scale - camera_state.scale; math::Abs(diff) > 0.01F) {
            const f32 old_scale = camera_state.scale;
            camera_state.scale += diff * ZOOM_LERP;
            camera_state.world_position += camera_state.zoom_anchor_world * float2 { camera_state.scale - old_scale };
        } else {
            camera_state.scale = camera_state.target_scale;
        }

        // map edge
        const float2 screen_center = float2 { Singleton::Get<WindowState>().screen_size } * float2 { 0.5F };
        const float2 pos_min = camera_state.map_world_min * float2 { camera_state.scale } - screen_center;
        const float2 pos_max = camera_state.map_world_max * float2 { camera_state.scale } - screen_center;
        camera_state.world_position.x = math::Clamp(camera_state.world_position.x, pos_min.x, pos_max.x);
        camera_state.world_position.y = math::Clamp(camera_state.world_position.y, pos_min.y, pos_max.y);
    }
};
} // namespace hex
