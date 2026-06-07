module;

export module pce.systems.t_tick_system;

import pce.g_globals;
import pce.u_ecs;
import pce.std;

export namespace pce {
using Tick = StrongType<u32, struct TickTag, Arithmetic>;

struct TickState {
    Tick tick { 0U };
    nanoseconds64 last_tick_time { TimeNowNS() };
    f32 delta_time { 1.0F };
};
struct TickSystem {
    void operator()() const {
        TickState& tick_state = Singleton::Get<TickState>();
        const nanoseconds64 time { TimeNowNS() };
        tick_state.delta_time = static_cast<f32>((time - tick_state.last_tick_time).value) * NS_TO_SECONDS;
        tick_state.last_tick_time = time;
        tick_state.tick += Tick { 1U };
    }
};
} // namespace pce

