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
    Tick tick { 0U };
    bool running = true;
    while (running) {
        tick += Tick { 1U };
        constexpr u32 skip = 10U;
        bool debug = tick.Value() % skip == skip - 1U;

        game.logger.LogLine();
        game.logger.Log("Tick {:6}", tick);
        game.PlayTick(tick, debug);

        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL3_ProcessEvent(&event);
            switch (event.type) {
                case SDL_EVENT_QUIT:
                    running = false;
                    break;
                default:
                    break;
            }
        }
        if (!running) { break; }
        TestDraw(tick.Value());

        if (debug) {
            //PrintListStats(game.logger, reinterpret<List<f32>>(player_archetype.moneys));
            //game.logger.Print();
            //        (void)std::cin.ignore();
        }
    }
}
} // namespace pce

i32 main(const i32 argc, char** argv) {
    (void)argc;
    (void)argv;

    SDL_Log("Starting game");

    if (!pce::InitEngine()) { return -1; }

    constexpr u32 width = 1600U;
    constexpr u32 height = 900U;
    if (!pce::SetWindow(width, height)) { return -1; }

    SDL_Log("Starting game");

    pce::RunGame();

    pce::DestroyEngine();
    return 0;
}
