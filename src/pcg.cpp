#include <SDL3/SDL_log.h>
#include <SDL3/SDL_main.h>

import pcg.g_arcade;
import pce.r_window;
import pce.u_logger;
import pce.collections;
import pce.std;

namespace pce {
b8 Start() {
    SDL_Log("Loading window");
    constexpr uint2 WINDOW_SIZE { 2500U, 1500U };
    Window window { WINDOW_SIZE };

    Logger().Log("Starting Game");
    pcg::arcade::RunHex();
    // pcg::arcade::RunMinimal();
    // pcg::arcade::RunHexBattle();
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
