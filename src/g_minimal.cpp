module;

#include "SDL3/SDL_keycode.h"
export module pcg.g_minimal;

import pce.g_globals;
import pce.collections;
import pce.systems.i_input_system;
import pce.systems.r_render;
import pce.systems.r_ui_node;
import pce.systems.t_debug_system;
import pce.systems.t_tick_system;
import pce.systems.u_animation_system;
import pce.systems.u_orchestra;
import pcg.g_arcade;

import pce.colors;

namespace pcg {
using namespace pce;
using namespace pce::ui;

void arcade::RunMinimal() {
    Singleton::Get<WindowState>().clear_color = colors::COLOR_LIGHT_SKY_BLUE;

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
