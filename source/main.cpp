#include "collections.hpp"
#include "components.hpp"
#include "engine.hpp"
#include "game.hpp"
#include "logger.hpp"
#include "types.hpp"

namespace pce {
using pcg::game;
using pcg::player_archetype;
using pcg::Tick;
using pce::Reinterpret;
using pce::List;

void RunGame() {
    Tick tick{0U};

    bool running = true;
    while (running) {
        constexpr u32 skip = 10U;
        for (u32 i = 0U; i < skip; i++) {
            game.logger.LogLine();
            game.logger.Log("Turn {:4}", tick);
            game.PlayTick(tick, tick.Value() % skip == skip - 1U);
            tick += Tick{1U};

            SDL_Event event;
            while (SDL_PollEvent(&event)) {
                switch (event.type) {
                    case SDL_EVENT_QUIT:
                        running = false;

                    default:
                        break;
                }
            }
            if (!running) break;
            TestDraw(tick.Value());
        }

        //PrintListStats(game.logger, reinterpret<List<f32>>(player_archetype.moneys));
        //game.logger.Print();
        //        (void)std::cin.ignore();
    }
}
} // namespace pce

i32 main(i32 argc, c8 *argv[]) {
    (void) argc;
    (void) argv;

    SDL_Log("Starting game");

    if (!pce::InitEngine() || !pce::SetWindow(640U, 480U)) return -1;

    SDL_Log("Starting game");

    pce::RunGame();

    pce::DestroyEngine();
    return 0;
}