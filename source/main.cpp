#include <iostream>

#include "collections.hpp"
#include "components.hpp"
#include "game.hpp"
#include "logger.hpp"
#include "types.hpp"

namespace pce {
using pcg::game;
using pcg::player_archetype;
using pcg::Tick;
using pce::Reinterpret;
using pce::List;

[[noreturn]] void RunGame() {
    Tick tick { 0U };
    while (true) {
        constexpr u32 skip = 10U;
        for (u32 i = 0U; i < skip; i++) {
            game.logger.LogLine();
            game.logger.Log("Turn {:4}", tick);
            game.PlayTick(tick, tick.Value() % skip == skip - 1U);
            tick += Tick { 1U } ;
        }
        //PrintListStats(game.logger, reinterpret<List<f32>>(player_archetype.moneys));
        game.logger.Print();
        (void)std::cin.ignore();
    }
}
} // namespace pce

i32 main() { pce::RunGame(); }
