#pragma once

#include <functional>
#include <ranges>

#include "u_types.hpp"

#include <SDL3/SDL_init.h>
#include <SDL3/SDL_log.h>

#include <SDL3_ttf/SDL_ttf.h>

namespace pce {

struct WindowConfig {
    SDL_Window* window;
    SDL_Renderer* renderer;
    TTF_TextEngine* text_engine;
    uint2 screen_size;
    SDL_Color clear_color;
};
struct Window {
    static WindowConfig window_config;

    explicit Window(const uint2 size) {
        constexpr u32 window_flags = SDL_WINDOW_RESIZABLE;
        if (!SDL_CreateWindowAndRenderer("Video Game", static_cast<i32>(size.x), static_cast<i32>(size.y), window_flags, &window_config.window, &window_config.renderer)) {
            SDL_Log("SDL_CreateWindowAndRenderer failed (%s)", SDL_GetError());
            SDL_Quit();
        }
        if (!SDL_Init(SDL_INIT_VIDEO)) {
            SDL_Log("SDL_Init failed (%s)", SDL_GetError());
            SDL_Quit();
        }
        if (!TTF_Init()) {
            SDL_Log("SDL_ttf failed (%s)", SDL_GetError());
            SDL_Quit();
        }
        SDL_SetRenderDrawBlendMode(window_config.renderer, SDL_BLENDMODE_BLEND);
        window_config.text_engine = TTF_CreateRendererTextEngine(window_config.renderer);
        window_config.screen_size = size;
    }
    ~Window() {
        SDL_DestroyRenderer(window_config.renderer);
        SDL_DestroyWindow(window_config.window);
        TTF_DestroyRendererTextEngine(window_config.text_engine);
        TTF_Quit();
        SDL_Quit();
    }
};
inline WindowConfig Window::window_config;
} // namespace pce
