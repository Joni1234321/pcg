module;

#include "SDL3/SDL_keycode.h"
export module pcg.g_minimal;

import pce.globals;
import pce.collections;
import pcs.input;
import pcs.render;
import pcs.node;
import pcs.debug;
import pcs.tick;
import pcs.animation;
import pcs.orchestra;
import pcg.g_arcade;

import pce.colors;

namespace hex {
using namespace hex;
using namespace hex::ui;

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
} // namespace hex
