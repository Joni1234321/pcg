#include "g_components.hpp"
#include "g_game.hpp"
#include "m_frame.hpp"
#include "r_engine.hpp"
#include "u_collections.hpp"
#include "u_logger.hpp"
#include "u_types.hpp"

namespace pce {
using pcg::game;
using pcg::player_archetype;
using pcg::Tick;
using pce::Reinterpret;
using pce::List;
using namespace pcg::frame;

void RunGame(Engine& engine) {
    ui::UISystem ui_system(engine.renderer);

    MainMenuFrame main_menu_frame(ui_system, engine.font);
    FPSFrame fps_frame(ui_system, engine.font);
    //OverviewFrame table_test_frame(ui_system, engine.font);

    Tick tick { 0U };
    bool running = true;
    while (running) {
        tick += Tick { 1U };
        constexpr u32 skip = 10U;
        const bool debug = tick.Value() % skip == skip - 1U;

        // game.logger.LogLine();
        // game.logger.Log("Tick {:6}", tick);
        game.PlayTick(tick, ui_system, engine.font, debug);

        main_menu_frame.Tick(tick.Value(), ui_system);
        fps_frame.Tick(tick.Value(), ui_system);

        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            (void)ImGui_ImplSDL3_ProcessEvent(&event);
            switch (event.type) {
                case SDL_EVENT_QUIT: running = false;
                    break;
                case SDL_EVENT_KEY_DOWN: switch (event.key.key) {
                        case SDLK_ESCAPE: running = false;
                            break;
                        default: break;
                    }
                    break;
                default: break;
            }
        }
        if (!running) { break; }
        engine.ClearScreen();

        ui_system.Draw(engine.renderer);
        DrawImgui(engine.renderer);

        engine.Present();

        if (debug) {
            //PrintListStats(game.logger, reinterpret<List<f32>>(player_archetype.moneys));
            //game.logger.Print();
            //        (void)std::cin.ignore();
        }
    }
}
b8 Start() {
    SDL_Log("Starting Engine");
    Engine engine;

    if (!engine.Load()) { return false; }

    constexpr u32 width = 1600U;
    constexpr u32 height = 900U;
    if (!engine.InitWindow(width, height)) { return false; }

    SDL_Log("Starting game");
    RunGame(engine);

    return true;
}
} // namespace pce

i32 main(const i32 argc, char** argv) {
    (void)argc;
    (void)argv;

    const b8 result = pce::Start();

    return result ? 0 : 1;
}
