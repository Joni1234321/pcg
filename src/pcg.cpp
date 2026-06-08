#include <SDL3/SDL_log.h>
#include <SDL3/SDL_main.h>

#include "g_arcade.hpp"

import std;
import pce.r_window;
import pce.logger;
import pce.collections;
import pce.std;

namespace hex {
b8 Start() {
    SDL_Log("Loading window");
    constexpr uint2 WINDOW_SIZE { 2500U, 1500U };
    Window window { WINDOW_SIZE };

    Logger().Log("Starting Game");
    hex::arcade::RunHex();
    // pcg::arcade::RunMinimal();
    // pcg::arcade::RunHexBattle();
    // pcg::arcade::RunBattleSim();
    // pcg::arcade::RunCommandStrike();
    // pcg::arcade::RunCosmoClick();
    // pcg::arcade::RunClickCore();
    Logger().Log("Quitting main loop");

    return true;
}
} // namespace hex

i32 main(const i32 argc, char** argv) {
    (void)argc;
    (void)argv;

    std::printf("Hello world");

    const b8 result = hex::Start();

    hex::Logger().Log("Quitting");

    return result ? 0 : 1;
}
