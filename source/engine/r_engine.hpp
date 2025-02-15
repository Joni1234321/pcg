#pragma once

#include <filesystem>
#include <functional>
#include <ranges>

#include "u_ecs.hpp"
#include "u_collections.hpp"
#include "u_types.hpp"

#include <SDL3/SDL_init.h>
#include <SDL3/SDL_log.h>
#include <SDL3/SDL_render.h>

#include <SDL3_ttf/SDL_ttf.h>

#define PCE_TIMER_ENABLE

#define PCE_TIMER(NAME) const pce::Timer _timer_ = pce::timer::Timer(NAME)
#define PCE_TIMER1(NAME) const pce::Timer _timer_1 = pce::timer::Timer(NAME)
#define PCE_TIMER2(NAME) const pce::Timer _timer_2 = pce::timer::Timer(NAME)

namespace pce::assets {
inline AbsolutePath Absolute(const RelativePath& relative_path) { return absolute(relative_path); }
inline AbsolutePath Asset(const AssetPath& asset_path) {
    const RelativePath assets_dir = R"(../assets)";
    return absolute(assets_dir / asset_path);
}
}

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

class Timer {
public:
    explicit Timer(const std::string& name) {
        start_ = std::chrono::high_resolution_clock::now();
        name_ = name;
    }
    ~Timer() {
        #ifdef PCE_TIMER_ENABLE
        const std::chrono::duration<f32> duration = std::chrono::high_resolution_clock::now() - start_;
        const f32 duration_ms = duration.count() * 1000.0F;
        // logger::LogTiming(name_, duration_ms);
        #endif
    }
private:
    std::string name_;
    std::chrono::time_point<std::chrono::steady_clock> start_;
};
} // namespace pce
