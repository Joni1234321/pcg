#pragma once

#include <filesystem>
#include <ranges>
#include <u_ecs.hpp>

#include "u_collections.hpp"
#include "u_types.hpp"

#include <SDL3/SDL_init.h>
#include <SDL3/SDL_log.h>
#include <SDL3/SDL_render.h>

#include <SDL3_ttf/SDL_ttf.h>

namespace pce::assets {
inline AbsolutePath Absolute(const RelativePath& relative_path) { return absolute(relative_path); }
inline AbsolutePath Asset(const AssetPath& asset_path) {
    const RelativePath assets_dir = R"(../assets)";
    return absolute(assets_dir / asset_path);
}
}

namespace pce {
using Tick = NamedType<u32, struct TickTag, Arithmetic>;

struct Window {
    static SDL_Window* window;
    static SDL_Renderer* renderer;
    static uint2 screen_size;
    static SDL_Color clear_color;

    explicit Window(const uint2 size) {
        constexpr u32 window_flags = SDL_WINDOW_RESIZABLE;
        if (!SDL_CreateWindowAndRenderer("Video Game", static_cast<i32>(size.x), static_cast<i32>(size.y), window_flags, &window, &renderer)) {
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
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

        screen_size = size;
    }
    ~Window() {
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        TTF_Quit();
        SDL_Quit();
    }
};
inline SDL_Window* Window::window;
inline SDL_Renderer* Window::renderer;
inline uint2 Window::screen_size;
inline SDL_Color Window::clear_color;

struct RenderSystem {
    void operator()() {
        (void)SDL_SetRenderDrawColor(Window::renderer, Window::clear_color.r, Window::clear_color.g, Window::clear_color.b, Window::clear_color.a);
        (void)SDL_RenderClear(Window::renderer);
    }
    void Present() {
        (void)SDL_GetWindowSize(Window::window, reinterpret_cast<i32*>(&Window::screen_size.x), reinterpret_cast<i32*>(&Window::screen_size.y));
        (void)SDL_RenderPresent(Window::renderer);
    }
};
struct TickSystem {
    b8 running { true };
    Tick tick { 0U };
    std::chrono::time_point<std::chrono::high_resolution_clock> last_tick_start;
    f32 tick_time { 1.0f };
    f32 delta_time { 1.0f };

    void operator()() {
        using namespace std::chrono;
        delta_time = duration<f32>(high_resolution_clock::now() - last_tick_start).count();
        last_tick_start = high_resolution_clock::now();
        tick += Tick { 1U };
    }
    void CaptureTime() {
        using namespace std::chrono;
        tick_time = duration<f32>(high_resolution_clock::now() - last_tick_start).count();
    }
};

class InputSystem {
    b8 quit { false };
    b8 left_mouse { false };
    b8 left_mouse_down { false };
    b8 left_mouse_up { false };
    uint2 mouse_position { };

public:
    UnorderedMap<SDL_Keycode, b8> keys { };
    void operator()() {
        for (b8& key : keys | std::ranges::views::values) { key = false; }
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_EVENT_QUIT: {
                    quit = true;
                    return;
                }
                case SDL_EVENT_KEY_DOWN:
                    keys[event.key.key] = true;
                    break;
                default:
                    break;
            }
        }

        float2 mouse_position_f;
        const SDL_MouseButtonFlags state = SDL_GetMouseState(&mouse_position_f.x, &mouse_position_f.y);
        mouse_position = uint2 { static_cast<u32>(mouse_position_f.x), static_cast<u32>(mouse_position_f.y) };
        left_mouse_down = state && SDL_BUTTON_LMASK && !left_mouse;
        left_mouse_up = left_mouse && !(state && SDL_BUTTON_LMASK);
        left_mouse = state && SDL_BUTTON_LMASK;
    }

    [[nodiscard]] constexpr uint2 MousePosition() const noexcept { return mouse_position; }
    [[nodiscard]] constexpr b8 LeftMouseDown() const noexcept { return left_mouse_down; }
    [[nodiscard]] constexpr b8 LeftMouseUp() const noexcept { return left_mouse_up; }
    [[nodiscard]] constexpr b8 LeftMouse() const noexcept { return left_mouse; }
    [[nodiscard]] constexpr b8 Quit() const noexcept { return quit; }
};
}
