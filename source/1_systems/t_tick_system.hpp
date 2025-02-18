#pragma once

#include <chrono>

#include "0_engine/u_ecs.hpp"
#include "0_engine/u_types.hpp"
#include "1_systems/orchestra.hpp"

namespace pce {
using Tick = NamedType<u32, struct TickTag, Arithmetic>;

struct TickState {
    Tick tick { 0U };
    std::chrono::time_point<std::chrono::high_resolution_clock> last_tick_start;
    f32 delta_time { 1.0F };
    b8 running { true };
};
struct TickSystem {
    void operator()() const {
        using namespace std::chrono;
        TickState& tick_state = singleton.Get<TickState>();
        tick_state.delta_time = duration<f32>(high_resolution_clock::now() - tick_state.last_tick_start).count();
        tick_state.last_tick_start = high_resolution_clock::now();
        tick_state.tick += Tick { 1U };
    }
};
} // namespace pce
