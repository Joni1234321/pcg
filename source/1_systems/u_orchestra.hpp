#pragma once

#include <functional>
#include <ranges>

#include "0_engine/g_globals.hpp"
#include "0_engine/u_collections.hpp"

namespace pce {
struct OrchestraState {
    List<std::function<void()>> systems;
    List<std::unique_ptr<void, void(*)(void*)>> system_storage;
    List<String> names;
    List<u32> nano_seconds;
};
struct Orchestra {
    template <typename T> void Add() {
        auto ptr = new T();                                                                                                                                            // Create system instance
        OrchestraState& orchestra_state = singleton.Get<OrchestraState>();
        orchestra_state.system_storage.EmplaceBack(ptr, [] (void* p) { delete static_cast<T*>(p); }); // Ensure destruction
        orchestra_state.systems.EmplaceBack([ptr]() { (*static_cast<T*>(ptr))(); });                                                                                   // Store callable functor
        orchestra_state.names.EmplaceBack(typeid(T).name());
        orchestra_state.nano_seconds.EmplaceBack(1U);
    }
    void RunSystems() {
        using namespace std::chrono;
        OrchestraState& orchestra_state = singleton.Get<OrchestraState>();
        for (const auto [i, system] : orchestra_state.systems | std::views::enumerate) {
            TimePoint start = TimeNow();
            system();
            Duration elapsed = TimeNow() - start;
            orchestra_state.nano_seconds[i] = static_cast<f32>(elapsed.count());
        }
    }
    ~Orchestra() { singleton.Get<OrchestraState>() = { }; }
};
} // namespace pce
