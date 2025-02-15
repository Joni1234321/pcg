#pragma once

#include <functional>
#include <ranges>

#include "0_engine/u_collections.hpp"

namespace pce {
struct OrchestraConfig {
    List<std::function<void()>> systems;
    List<std::unique_ptr<void, void(*)(void*)>> system_storage;
    List<String> names;
    List<u32> nano_seconds;
};
struct Orchestra {
    static OrchestraConfig orchestra_table;
    template <typename T> void Add() {
        auto ptr = new T();                                                                           // Create system instance
        orchestra_table.system_storage.EmplaceBack(ptr, [] (void* p) { delete static_cast<T*>(p); }); // Ensure destruction
        orchestra_table.systems.EmplaceBack([ptr]() { (*static_cast<T*>(ptr))(); });                  // Store callable functor
        orchestra_table.names.EmplaceBack(typeid(T).name());
        orchestra_table.nano_seconds.EmplaceBack(1U);
    }
    void RunSystems() {
        using namespace std::chrono;
        for (const auto [i, system] : orchestra_table.systems | std::views::enumerate) {
            TimePoint start = TimeNow();
            system();
            Duration elapsed = TimeNow() - start;
            orchestra_table.nano_seconds[i] = static_cast<f32>(elapsed.count());
        }
    }
    ~Orchestra() { orchestra_table = { }; }
};
inline OrchestraConfig Orchestra::orchestra_table;


struct InputTable {
    UnorderedMap<SDL_Keycode, b8> keys { };
    UnorderedMap<SDL_Keycode, b8> keys_down { };
    UnorderedMap<SDL_Keycode, b8> keys_up { };
    b8 quit { false };
    b8 left_mouse { false };
    b8 left_mouse_down { false };
    b8 left_mouse_up { false };
    uint2 mouse_position { };
};
struct InputSystem {
    static InputTable input_table;
    void operator()() {
        for (b8& key : input_table.keys_up | std::ranges::views::values) { key = false; }
        for (b8& key : input_table.keys_down | std::ranges::views::values) { key = false; }
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_EVENT_QUIT: {
                    input_table.quit = true;
                    return;
                }
                case SDL_EVENT_KEY_UP:
                    input_table.keys[event.key.key] = false;
                input_table.keys_up[event.key.key] = true;
                break;
                case SDL_EVENT_KEY_DOWN:
                    input_table.keys[event.key.key] = true;
                input_table.keys_down[event.key.key] = true;
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
}
