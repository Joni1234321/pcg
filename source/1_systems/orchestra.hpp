#pragma once

#include <functional>
#include <ranges>
#include <typeindex>

#include "0_engine/u_collections.hpp"

namespace pce {
struct OrchestraConfig {
    List<std::function<void()>> systems;
    List<std::unique_ptr<void, void(*)(void*)>> system_storage;
    List<String> names;
    List<u32> nano_seconds;
};
struct Data {
    using Key = std::type_index;
    using Database = UnorderedMap<Key, void*>;
    Database tables;
    static constexpr u32 DEFAULT_SIZE { 64U };
    template <typename T> [[nodiscard]] constexpr HandleList<T>& Get() {
        const Key key { typeid(T) };
        const Database::iterator table_iterator = tables.find(key);
        if (table_iterator == tables.end()) {
            HandleList<T>* table = new HandleList<T> { DEFAULT_SIZE };
            tables[key] = table;
            return *table;
        }
        HandleList<T>* table = static_cast<HandleList<T>*>(table_iterator->second);
        return *table;
    }
    template <typename T> [[nodiscard]] constexpr T& operator[](Handle<T> handle) { return Get<T>()[handle]; }
};
inline Data data { };
struct Orchestra {
    static OrchestraConfig orchestra_config;
    template <typename T> void Add() {
        auto ptr = new T();                                                                            // Create system instance
        orchestra_config.system_storage.EmplaceBack(ptr, [] (void* p) { delete static_cast<T*>(p); }); // Ensure destruction
        orchestra_config.systems.EmplaceBack([ptr]() { (*static_cast<T*>(ptr))(); });                  // Store callable functor
        orchestra_config.names.EmplaceBack(typeid(T).name());
        orchestra_config.nano_seconds.EmplaceBack(1U);
    }
    void RunSystems() {
        using namespace std::chrono;
        for (const auto [i, system] : orchestra_config.systems | std::views::enumerate) {
            TimePoint start = TimeNow();
            system();
            Duration elapsed = TimeNow() - start;
            orchestra_config.nano_seconds[i] = static_cast<f32>(elapsed.count());
        }
    }
    ~Orchestra() { orchestra_config = { }; }
};
inline OrchestraConfig Orchestra::orchestra_config;

struct InputConfig {
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
    static InputConfig input_config;
    void operator()() {
        for (b8& key : input_config.keys_up | std::ranges::views::values) { key = false; }
        for (b8& key : input_config.keys_down | std::ranges::views::values) { key = false; }
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_EVENT_QUIT: {
                    input_config.quit = true;
                    return;
                }
                case SDL_EVENT_KEY_UP:
                    input_config.keys[event.key.key] = false;
                    input_config.keys_up[event.key.key] = true;
                    break;
                case SDL_EVENT_KEY_DOWN:
                    input_config.keys[event.key.key] = true;
                    input_config.keys_down[event.key.key] = true;
                    break;
                default:
                    break;
            }
        }

        float2 mouse_position_f;
        const SDL_MouseButtonFlags state = SDL_GetMouseState(&mouse_position_f.x, &mouse_position_f.y);
        input_config.mouse_position = uint2 { static_cast<u32>(mouse_position_f.x), static_cast<u32>(mouse_position_f.y) };
        input_config.left_mouse_down = state && SDL_BUTTON_LMASK && !input_config.left_mouse;
        input_config.left_mouse_up = input_config.left_mouse && !(state && SDL_BUTTON_LMASK);
        input_config.left_mouse = state && SDL_BUTTON_LMASK;
    }
};
inline InputConfig InputSystem::input_config;
}
