#include <iostream>
#include <ranges>
#include <vector>

#include "collections.hpp"
#include "components.hpp"
#include "game.hpp"
#include "logger.hpp"
#include "types.hpp"

using namespace pcg;
using namespace pce;

inline bool end_game = false;
u32 main() {
    u32 i = 0U;
    while (!end_game) {
        for (const u32 new_limit = i + 10U; i < new_limit; i++) {
            Game.logger.LogLine();
            Game.logger.Log("Turn {:4}", i);
            Game.Tick(i);
        }

        Game.logger.LogVectorStats(reinterpret<List<f32>>(player_archetype.moneys));
        Game.logger.Print();
        (void)std::cin.ignore();
    }

    return 0;
}
