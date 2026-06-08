module;

#include <SDL3/SDL_render.h>
export module pcs.render;

import pce.std;
import pce.r_window;
import pce.window_state;
import pce.globals;

export struct RenderWindowSystem {
    void operator()() const {
        hex::WindowState& window_state = hex::Singleton::Get<hex::WindowState>();
        (void)SDL_GetWindowSize(window_state.window, reinterpret_cast<i32*>(&window_state.screen_size.x), reinterpret_cast<i32*>(&window_state.screen_size.y));
        (void)SDL_RenderPresent(window_state.renderer);
        (void)SDL_SetRenderDrawColor(window_state.renderer, window_state.clear_color.r, window_state.clear_color.g, window_state.clear_color.b, window_state.clear_color.a);
        (void)SDL_RenderClear(window_state.renderer);
    }
};
