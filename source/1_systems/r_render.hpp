#pragma once

#include "0_engine/r_window.hpp"

#include <SDL3/SDL_render.h>

namespace pce {
struct RenderClearSystem {
    void operator()() {
        (void)SDL_SetRenderDrawColor(Window::window_config.renderer, Window::window_config.clear_color.r, Window::window_config.clear_color.g, Window::window_config.clear_color.b, Window::window_config.clear_color.a);
        (void)SDL_RenderClear(Window::window_config.renderer);
    }
};
struct PresentSystem {
    void operator()() {
        (void)SDL_GetWindowSize(Window::window_config.window, reinterpret_cast<i32*>(&Window::window_config.screen_size.x), reinterpret_cast<i32*>(&Window::window_config.screen_size.y));
        (void)SDL_RenderPresent(Window::window_config.renderer);
    }
};
} // namespace pce
