#include <iostream>
#include <ranges>
#include <vector>

#include "collections.hpp"
#include "components.hpp"
#include "game.hpp"
#include "logger.hpp"
#include "types.hpp"

namespace pce {
using pcg::game;
using pcg::player_archetype;
using pce::reinterpret;
using pce::List;
[[noreturn]] void RunGame() {
    u32 turn = 0U;
    while (true) {
        constexpr u32 skip = 10U;
        for (u32 i = 0U; i < skip; i++) {
            game.logger.LogLine();
            game.logger.Log("Turn {:4}", turn);
            game.Tick(turn);
            turn++;
        }
        game.logger.LogVectorStats(reinterpret<List<f32>>(player_archetype.moneys));
        game.logger.Print();
        (void)std::cin.ignore();
    }
}
} // namespace pce

i32 main() { pce::RunGame(); }
