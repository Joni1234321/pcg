module;

#include "SDL3/SDL_mouse.h"
#include <SDL3/SDL.h>
export module pce.systems.i_input_system;

import std;
import pce.g_globals;
import pce.collections;
import pce.std;
import pce.u_logger;

export namespace pce {
struct InputState {
    UnorderedMap<SDL_Keycode, b8> keys { };
    UnorderedMap<SDL_Keycode, b8> keys_down { };
    UnorderedMap<SDL_Keycode, b8> keys_up { };
    b8 quit { false };
    b8 left_mouse { false };
    b8 left_mouse_down { false };
    b8 left_mouse_up { false };
    b8 right_mouse { false };
    b8 right_mouse_down { false };
    b8 right_mouse_up { false };
    float2 mouse_position { };
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

        const SDL_MouseButtonFlags state = SDL_GetMouseState(&input_state.mouse_position.x, &input_state.mouse_position.y);
        input_state.left_mouse_down = state & SDL_BUTTON_LMASK && !input_state.left_mouse;
        input_state.left_mouse_up = input_state.left_mouse && !(state & SDL_BUTTON_LMASK);
        input_state.left_mouse = state & SDL_BUTTON_LMASK;
        input_state.right_mouse_down = state & SDL_BUTTON_RMASK && !input_state.right_mouse;
        input_state.right_mouse_up = input_state.right_mouse && !(state & SDL_BUTTON_RMASK);
        input_state.right_mouse = state & SDL_BUTTON_RMASK;
    }
};
} // namespace pce
