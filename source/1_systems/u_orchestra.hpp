#pragma once

#include <functional>
#include <ranges>

#include "0_engine/g_globals.hpp"
#include "0_engine/u_collections.hpp"

namespace pce {
struct OrchestraConfig {
    List<std::function<void()>> systems;
    List<std::unique_ptr<void, void(*)(void*)>> system_storage;
    List<String> names;
    List<u32> nano_seconds;
};
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
} // namespace pce
