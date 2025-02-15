#pragma once

#include <chrono>

#include "engine/u_ecs.hpp"
#include "engine/u_types.hpp"

namespace pce {
using Tick = NamedType<u32, struct TickTag, Arithmetic>;

struct TickTable {
    Tick tick { 0U };
    std::chrono::time_point<std::chrono::high_resolution_clock> last_tick_start;
    f32 delta_time;
    b8 running;
};
struct TickSystem {
    static TickTable tick_table;
    TickSystem() {
        tick_table.running = true;
        tick_table.tick = Tick { 0U };
        tick_table.delta_time = 1.0F;
    }
    void operator()() {
        using namespace std::chrono;
        tick_table.delta_time = duration<f32>(high_resolution_clock::now() - tick_table.last_tick_start).count();
        tick_table.last_tick_start = high_resolution_clock::now();
        tick_table.tick += Tick { 1U };
    }
};
inline TickTable TickSystem::tick_table;
} // namespace pce
