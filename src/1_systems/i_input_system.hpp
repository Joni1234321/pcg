#pragma once

#include <SDL3/SDL.h>
#include <ranges>

#include "0_engine/g_globals.hpp"
#include "0_engine/u_collections.hpp"
#include "0_engine/u_logger.hpp"
#include "SDL3/SDL_mouse.h"

namespace pce {
struct InputState {
    UnorderedMap<SDL_Keycode, b8> keys { };
    UnorderedMap<SDL_Keycode, b8> keys_down { };
    UnorderedMap<SDL_Keycode, b8> keys_up { };
    b8 quit { false };
    b8 left_mouse { false };
    b8 left_mouse_down { false };
    b8 left_mouse_up { false };
    uint2 mouse_position { };
    f32 mouse_wheel_y { 0.0F };
};
struct InputSystem {
    void operator()() const {
        InputState& input_state = Singleton::Get<InputState>();
        for (b8& key : input_state.keys_up | std::ranges::views::values) { key = false; }
        for (b8& key : input_state.keys_down | std::ranges::views::values) { key = false; }
        input_state.mouse_wheel_y = 0.0F;
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_EVENT_QUIT: {
                    input_state.quit = true;
                    return;
                }
                case SDL_EVENT_KEY_UP:
                    input_state.keys[event.key.key] = false;
                    input_state.keys_up[event.key.key] = true;
                    break;
                case SDL_EVENT_KEY_DOWN:
                    input_state.keys[event.key.key] = true;
                    input_state.keys_down[event.key.key] = true;
                    break;
                case SDL_EVENT_MOUSE_WHEEL: input_state.mouse_wheel_y = event.wheel.y; break;
                default: break;
            }
        }

        float2 mouse_position_f { };
        const SDL_MouseButtonFlags state = SDL_GetMouseState(&mouse_position_f.x, &mouse_position_f.y);
        Logger().Log("Mouse [{:4}{:4}]", mouse_position_f.x, mouse_position_f.y);
        input_state.mouse_position = uint2 { static_cast<u32>(mouse_position_f.x), static_cast<u32>(mouse_position_f.y) };
        input_state.left_mouse_down = state && SDL_BUTTON_LMASK && !input_state.left_mouse;
        input_state.left_mouse_up = input_state.left_mouse && !(state && SDL_BUTTON_LMASK);
        input_state.left_mouse = state && SDL_BUTTON_LMASK;
    }
};
} // namespace pce
