#pragma once

#include <filesystem>
#include <functional>
#include <ranges>
#include <u_ecs.hpp>
#include <u_logger.hpp>

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

struct SystemTable {
    std::vector<std::function<void()>> systems;
    std::vector<std::unique_ptr<void, void(*)(void*)>> system_storage;
    std::vector<String> names;
    std::vector<u32> nano_seconds;
};
struct SystemStructure {
    // std::vector<std::unique_ptr<void, void(*)(void*)>> systems;
    // template <typename T> void Add() {
    //     auto deleter = [] (void* ptr) -> void { delete static_cast<T*>(ptr); };
    //     systems.emplace_back(new T { }, deleter);
    // }

    // std::vector<std::function<void()>> systems;
    // template <typename T> void Add() {
    //     systems.emplace_back(new T { });
    // }

    static SystemTable system_table;
    template <typename T>
    void Add() {
        auto ptr = new T();  // Create system instance
        system_table.system_storage.emplace_back(ptr, [](void* p) { delete static_cast<T*>(p); }); // Ensure destruction
        system_table.systems.emplace_back([ptr]() { (*static_cast<T*>(ptr))(); });  // Store callable functor
        system_table.names.emplace_back(typeid(T).name());
        system_table.nano_seconds.emplace_back(1U);
    }

    void RunSystems() {
        using namespace std::chrono;
        for (const auto [i, system] : system_table.systems | std::views::enumerate) {
            TimePoint start = TimeNow();
            system();
            Duration elapsed = TimeNow() - start;
            system_table.nano_seconds[i] = static_cast<f32>(elapsed.count());
        }
    }
};
inline SystemTable SystemStructure::system_table;
struct Window {
    static SDL_Window* window;
    static SDL_Renderer* renderer;
    static TTF_TextEngine* text_engine;
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
        text_engine = TTF_CreateRendererTextEngine(renderer);
        screen_size = size;
    }
    ~Window() {
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        TTF_DestroyRendererTextEngine(text_engine);
        TTF_Quit();
        SDL_Quit();
    }
};
inline SDL_Window* Window::window;
inline SDL_Renderer* Window::renderer;
inline uint2 Window::screen_size;
inline SDL_Color Window::clear_color;
inline TTF_TextEngine* Window::text_engine;

struct RenderSystem {
    void operator()() {
        (void)SDL_SetRenderDrawColor(Window::renderer, Window::clear_color.r, Window::clear_color.g, Window::clear_color.b, Window::clear_color.a);
        (void)SDL_RenderClear(Window::renderer);
    }
};
struct PresentSystem {
    void operator()() {
        (void)SDL_GetWindowSize(Window::window, reinterpret_cast<i32*>(&Window::screen_size.x), reinterpret_cast<i32*>(&Window::screen_size.y));
        (void)SDL_RenderPresent(Window::renderer);
    }
};

struct TickTable {
    Tick tick { 0U };
    std::chrono::time_point<std::chrono::high_resolution_clock> last_tick_start;
    f32 tick_time;
    f32 delta_time;
    b8 running;
};
struct InputTable {
    UnorderedMap<SDL_Keycode, b8> keys { };
    b8 quit { false };
    b8 left_mouse { false };
    b8 left_mouse_down { false };
    b8 left_mouse_up { false };
    uint2 mouse_position { };
};
struct TickSystem {
    static TickTable tick_table;
    TickSystem() {
        tick_table.running = true;
        tick_table.tick = Tick { 0U };
        tick_table.tick_time = 1.0F;
        tick_table.delta_time = 1.0F;
    }
    void operator()() {
        using namespace std::chrono;
        tick_table.delta_time = duration<f32>(high_resolution_clock::now() - tick_table.last_tick_start).count();
        tick_table.last_tick_start = high_resolution_clock::now();
        tick_table.tick += Tick { 1U };
    }
    void CaptureTime() {
        using namespace std::chrono;
        tick_table.tick_time = duration<f32>(high_resolution_clock::now() - tick_table.last_tick_start).count();
    }
};
struct InputSystem {
    static InputTable input_table;
    void operator()() {
        for (b8& key : input_table.keys | std::ranges::views::values) { key = false; }
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_EVENT_QUIT: {
                    input_table.quit = true;
                    return;
                }
                case SDL_EVENT_KEY_DOWN:
                    input_table.keys[event.key.key] = true;
                    break;
                default:
                    break;
            }
        }

        float2 mouse_position_f;
        const SDL_MouseButtonFlags state = SDL_GetMouseState(&mouse_position_f.x, &mouse_position_f.y);
        input_table.mouse_position = uint2 { static_cast<u32>(mouse_position_f.x), static_cast<u32>(mouse_position_f.y) };
        input_table.left_mouse_down = state && SDL_BUTTON_LMASK && !input_table.left_mouse;
        input_table.left_mouse_up = input_table.left_mouse && !(state && SDL_BUTTON_LMASK);
        input_table.left_mouse = state && SDL_BUTTON_LMASK;
    }
};
inline InputTable InputSystem::input_table;
inline TickTable TickSystem::tick_table;
} // namespace pce
