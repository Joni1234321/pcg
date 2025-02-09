
#include <SDL3/SDL_main.h>

#include "g_arcade.hpp"
#include "g_components.hpp"
#include "g_game.hpp"
#include "r_engine.hpp"
#include "u_collections.hpp"
#include "u_logger.hpp"
#include "u_types.hpp"

namespace pce {
using pcg::game;
using pcg::player_archetype;
using pce::Reinterpret;
using pce::List;

b8 Start() {
    SDL_Log("Loading window");
    constexpr uint2 window_size { 1600U, 900U };
    Window window { window_size };

    SDL_Log("Starting game");
    pcg::arcade::RunCosmoClick();
    pcg::arcade::RunClickCore();

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
