#include "0_engine/g_globals.hpp"
#include "0_engine/u_collections.hpp"
#include "0_engine/u_colors.hpp"
#include "1_systems/i_input_system.hpp"
#include "1_systems/r_render.hpp"
#include "1_systems/r_ui_node.hpp"
#include "1_systems/t_debug_system.hpp"
#include "1_systems/t_tick_system.hpp"
#include "1_systems/u_animation_system.hpp"
#include "1_systems/u_orchestra.hpp"
#include "SDL3/SDL_keycode.h"
#include "g_arcade.hpp"

namespace pcg {
using namespace pce;
using namespace pce::ui;

void arcade::RunMinimal() {
    Singleton::Get<WindowState>().clear_color = colors::LIGHT_SKY_BLUE;

    // Systems
    Orchestra orchestra { };
    orchestra.Add<DebugSystem>();

    orchestra.Add<TickSystem>();
    orchestra.Add<InputSystem>();
    orchestra.Add<InputNodeSystem>();

    // your systems
    // orchestra.Add<CosmoClickSystem>();
    // orchestra.Add<CosmoClickUISystem>();

    orchestra.Add<AnimationSystem>();
    orchestra.Add<RenderNodeSystem>();
    orchestra.Add<ParticleSystem>();
    orchestra.Add<RenderWindowSystem>();

    while (!Singleton::Get<InputState>().quit && !Singleton::Get<InputState>().keys_down[SDLK_ESCAPE]) { orchestra.RunSystems(); }
}
} // namespace pcg
