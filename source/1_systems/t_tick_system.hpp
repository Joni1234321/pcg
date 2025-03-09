#pragma once

#include "0_engine/u_ecs.hpp"
#include "0_engine/u_types.hpp"
#include "1_systems/u_orchestra.hpp"

namespace pce {
using Tick = NamedType<u32, struct TickTag, Arithmetic>;

struct TickState {
    Tick tick { 0U };
    nanoseconds64 last_tick_time { TimeNowNS() };
    f32 delta_time { 1.0F };
};
struct TickSystem {
    void operator()() const {
        TickState& tick_state = singleton.Get<TickState>();
        const nanoseconds64 time { TimeNowNS() };
        tick_state.delta_time = (time - tick_state.last_tick_time).Value() * NS_TO_SECONDS;
        tick_state.last_tick_time = time;
        tick_state.tick += Tick { 1U };
    }
};
} // namespace pce
