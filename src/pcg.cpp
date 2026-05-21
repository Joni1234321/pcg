#include <SDL3/SDL_main.h>

#include "g_arcade.hpp"
#include "g_components.hpp"
#include "g_game.hpp"

#include "0_engine/r_window.hpp"
#include "0_engine/u_collections.hpp"
#include "0_engine/u_logger.hpp"
#include "0_engine/u_types.hpp"

namespace pce {
using pce::List;
using pce::Reinterpret;
using pcg::game;
using pcg::player_archetype;

b8 Start() {
    SDL_Log("Loading window");
    constexpr uint2 window_size { 2500U, 1500U };
    Window window { window_size };

    Logger().Log("Starting Game");
    pcg::arcade::RunHexBattle();
    // pcg::arcade::RunBattleSim();
    // pcg::arcade::RunCommandStrike();
    // pcg::arcade::RunCosmoClick();
    // pcg::arcade::RunClickCore();
    Logger().Log("Quitting main loop");

    return true;
}
} // namespace pce

i32 main(const i32 argc, char** argv) {
    (void)argc;
    (void)argv;

    const b8 result = pce::Start();

    pce::Logger().Log("Quitting");

    return result ? 0 : 1;
}
