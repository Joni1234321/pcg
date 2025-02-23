#pragma once

#include "0_engine/r_window.hpp"

#include <SDL3/SDL_render.h>

namespace pce {
struct PresentSystem {
    void operator()() const {
        WindowState& window_state = singleton.Get<WindowState>();
        (void)SDL_GetWindowSize(window_state.window, reinterpret_cast<i32*>(&window_state.screen_size.x), reinterpret_cast<i32*>(&window_state.screen_size.y));
        (void)SDL_RenderPresent(window_state.renderer);
        (void)SDL_SetRenderDrawColor(window_state.renderer, window_state.clear_color.r, window_state.clear_color.g, window_state.clear_color.b, window_state.clear_color.a);
        (void)SDL_RenderClear(window_state.renderer);
    }
};
} // namespace pce
