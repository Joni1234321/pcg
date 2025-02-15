#pragma once

#include <chrono>

#include "0_engine/u_ecs.hpp"
#include "0_engine/u_types.hpp"

namespace pce {
using Tick = NamedType<u32, struct TickTag, Arithmetic>;

struct TickConfig {
    Tick tick { 0U };
    std::chrono::time_point<std::chrono::high_resolution_clock> last_tick_start;
    f32 delta_time;
    b8 running;
};
struct TickSystem {
    static TickConfig tick_config;
    TickSystem() {
        tick_config.running = true;
        tick_config.tick = Tick { 0U };
        tick_config.delta_time = 1.0F;
    }
    void operator()() {
        using namespace std::chrono;
        tick_config.delta_time = duration<f32>(high_resolution_clock::now() - tick_config.last_tick_start).count();
        tick_config.last_tick_start = high_resolution_clock::now();
        tick_config.tick += Tick { 1U };
    }
};
inline TickConfig TickSystem::tick_config;
} // namespace pce
