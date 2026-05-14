#pragma once

#include <functional>
#include <memory>
#include <ranges>

#include "0_engine/g_globals.hpp"
#include "0_engine/u_collections.hpp"
#include "0_engine/u_ecs.hpp"

namespace pce {
struct OrchestraState {
    List<std::function<void()>> systems;
    List<std::unique_ptr<void, void(*)(void*)>> system_storage;
    List<String> names;
    List<nanoseconds64> ns;
};
struct Orchestra {
    template <typename T> void Add() {
        auto ptr = new T();                                                                                                                                            // Create system instance
        OrchestraState& orchestra_state = singleton.Get<OrchestraState>();
        orchestra_state.system_storage.EmplaceBack(ptr, [] (void* p) { delete static_cast<T*>(p); }); // Ensure destruction
        orchestra_state.systems.EmplaceBack([ptr] ()-> void { (*static_cast<T*>(ptr))(); });                     // Store callable functor
        orchestra_state.names.EmplaceBack(typeid(T).name());
        orchestra_state.ns.EmplaceBack(1U);
    }
    void RunSystems() {
        OrchestraState& orchestra_state = singleton.Get<OrchestraState>();
        for (const auto [i, system] : orchestra_state.systems | std::views::enumerate) {
            nanoseconds64 start = TimeNowNS();
            system();
            orchestra_state.ns[i] = TimeNowNS() - start;
        }
    }
    ~Orchestra() { singleton.Get<OrchestraState>() = { }; }
};
} // namespace pce