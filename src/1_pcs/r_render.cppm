module;

#include <SDL3/SDL_render.h>
export module pcs.render;

import pce.std;
import pce.window_state;
import pce.globals;

export struct RenderWindowSystem {
    void operator()() const {
        hex::WindowState& window_state = hex::Singleton::Get<hex::WindowState>();
        const f32 display_scale = SDL_GetWindowDisplayScale(window_state.window);
        window_state.ui_scale = hex::UI_SCALE * (display_scale > 0.0F ? display_scale : 1.0F);
        (void)SDL_SetRenderScale(window_state.renderer, window_state.ui_scale, window_state.ui_scale);
        (void)SDL_GetWindowSize(window_state.window, reinterpret_cast<i32*>(&window_state.screen_size.x), reinterpret_cast<i32*>(&window_state.screen_size.y));
        window_state.screen_size.x = static_cast<u32>(static_cast<f32>(window_state.screen_size.x) / window_state.ui_scale);
        window_state.screen_size.y = static_cast<u32>(static_cast<f32>(window_state.screen_size.y) / window_state.ui_scale);
        (void)SDL_RenderPresent(window_state.renderer);
        (void)SDL_SetRenderDrawColor(window_state.renderer, window_state.clear_color.r, window_state.clear_color.g, window_state.clear_color.b, window_state.clear_color.a);
        (void)SDL_RenderClear(window_state.renderer);
    }
};
