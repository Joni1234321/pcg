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
using pce::reinterpret;
using pce::List;

[[noreturn]] void RunGame() {
    Tick turn { 0U };
    while (true) {
        constexpr u32 skip = 10U;
        for (u32 i = 0U; i < skip; i++) {
            game.logger.LogLine();
            game.logger.Log("Turn {:4}", turn);
            game.PlayTick(turn, i + 1U == skip);
            turn.value++;
        }
        //PrintListStats(game.logger, reinterpret<List<f32>>(player_archetype.moneys));
        game.logger.Print();
        (void)std::cin.ignore();
    }
}
} // namespace pce

i32 main() { pce::RunGame(); }
