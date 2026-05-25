#pragma once

#include <SDL3/SDL.h>
#include <imgui.h>
#include <backends/imgui_impl_sdl3.h>
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
    b8 right_mouse { false };
    b8 right_mouse_down { false };
    b8 right_mouse_up { false };
    int2 mouse_position { };
    f32 mouse_wheel_y { 0.0F };
};
struct InputSystem {
    void operator()() const {
        InputState& input_state = Singleton::Get<InputState>();
        for (b8& key : input_state.keys_up | std::ranges::views::values) { key = false; }
        for (b8& key : input_state.keys_down | std::ranges::views::values) { key = false; }
        input_state.mouse_wheel_y = 0.0F;
        // If imgui hasn't been initialised (no current context) these capture
        // queries simply return false, so this code path is safe in either case.
        const b8 imgui_alive   = ImGui::GetCurrentContext() != nullptr;
        const b8 imgui_kbd     = imgui_alive && ImGui::GetIO().WantCaptureKeyboard;
        const b8 imgui_mouse   = imgui_alive && ImGui::GetIO().WantCaptureMouse;
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (imgui_alive) { (void)ImGui_ImplSDL3_ProcessEvent(&event); }
            switch (event.type) {
                case SDL_EVENT_QUIT: {
                    input_state.quit = true;
                    return;
                }
                case SDL_EVENT_KEY_UP:
                    if (imgui_kbd) { break; }
                    input_state.keys[event.key.key] = false;
                    input_state.keys_up[event.key.key] = true;
                    break;
                case SDL_EVENT_KEY_DOWN:
                    if (imgui_kbd) { break; }
                    input_state.keys[event.key.key] = true;
                    input_state.keys_down[event.key.key] = true;
                    break;
                case SDL_EVENT_MOUSE_WHEEL:
                    if (imgui_mouse) { break; }
                    input_state.mouse_wheel_y = event.wheel.y;
                    break;
                default: break;
            }
        }

        float2 mouse_position_f { };
        const SDL_MouseButtonFlags state = SDL_GetMouseState(&mouse_position_f.x, &mouse_position_f.y);
        input_state.mouse_position = static_cast<int2>(mouse_position_f);
        const SDL_MouseButtonFlags effective = imgui_mouse ? SDL_MouseButtonFlags { 0 } : state;
        input_state.left_mouse_down  = effective & SDL_BUTTON_LMASK && !input_state.left_mouse;
        input_state.left_mouse_up    = input_state.left_mouse && !(effective & SDL_BUTTON_LMASK);
        input_state.left_mouse       = effective & SDL_BUTTON_LMASK;
        input_state.right_mouse_down = effective & SDL_BUTTON_RMASK && !input_state.right_mouse;
        input_state.right_mouse_up   = input_state.right_mouse && !(effective & SDL_BUTTON_RMASK);
        input_state.right_mouse      = effective & SDL_BUTTON_RMASK;
    }
};
} // namespace pce
