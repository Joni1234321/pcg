#include <SDL3/SDL_main.h>

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
    InputSystem input_system { };
    ui::UISystem ui_system(engine);

    MainMenuFrame main_menu_frame(ui_system);
    TickFrame tick_frame(ui_system);
    TestFrame test_frame(ui_system);
    DebugFrame debug_frame(ui_system);

    ui::NodeTree& active_tree = test_frame.tree;
    Tick tick { 0U };
    bool running = true;
    while (running) {
        tick += Tick { 1U };
        constexpr u32 skip = 10U;
        const bool debug = tick.Value() % skip == skip - 1U;

        game.PlayTick(tick, ui_system, debug);

        // main_menu_frame.Tick(tick.Value(), ui_system);
        tick_frame.Tick(tick.Value());
        test_frame.Tick(tick.Value(), ui_system);
        debug_frame.Tick(tick.Value(), ui_system);

        input_system.Tick();
        ui_system.Tick(input_system, active_tree);

        ui_system.left_mouse_down = false;

        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            (void)ImGui_ImplSDL3_ProcessEvent(&event);
            switch (event.type) {
                case SDL_EVENT_QUIT:
                    running = false;
                    break;
                case SDL_EVENT_KEY_DOWN:
                    switch (event.key.key) {
                        case SDLK_ESCAPE:
                            Logger().Log("Quit requested");
                            running = false;
                            return;
                        default:
                            break;
                    }
                    break;
                case SDL_EVENT_MOUSE_BUTTON_DOWN:
                    if (event.button.button == SDL_BUTTON_LEFT) {
                        ui_system.left_mouse_down = true;
                        ui_system.LeftClick(active_tree);
                    }
                    break;
                default:
                    //Logger().Log("Unhandled event {}", event.type);
                    break;
            }
        }

        if (!running) { break; }

        engine.ClearScreen();

        ui_system.RenderElements(engine.renderer);
        ui_system.RenderTree(engine.renderer, tick_frame.tree);
        ui_system.RenderTree(engine.renderer, test_frame.tree);
        ui_system.RenderTree(engine.renderer, debug_frame.tree);
        ui_system.RenderTree(engine.renderer, main_menu_frame.tree);


        //DrawImgui(engine.renderer);
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
