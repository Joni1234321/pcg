#include "g_arcade.hpp"

#include "SDL3/SDL_keycode.h"

import std;

import pce.sdl;
import pce.std;
import pce.math;
import pce.globals;
import pce.window_state;
import pce.collections;

import pcs.input;
import pcs.camera;
import pcs.render;
import pcs.node;
import pcs.node_data;
import pcs.debug;
import pcs.tick;
import pcs.animation;
import pcs.orchestra;

import rail.types;
import rail.editor;

using namespace hex;
using namespace hex::ui;

void arcade::RunRail() {
    Singleton::Get<WindowState>().clear_color = Color::FromHsl(42.0F, 0.12F, 0.66F);
    Singleton::Get<CameraState>().scale = rail::EDITOR_CAMERA_SCALE;
    Singleton::Get<CameraState>().target_scale = rail::EDITOR_CAMERA_SCALE;

    Orchestra orchestra { };
    orchestra.Add<DebugSystem>();
    orchestra.Add<TickSystem>();

    orchestra.Add<InputSystem>();
    orchestra.Add<InputNodeSystem>();

    orchestra.Add<rail::RailEditorSystem>();

    orchestra.Add<AnimationSystem>();
    orchestra.Add<ParticleSystem>();

    orchestra.Add<CameraSystem>();
    orchestra.Add<RenderNodeSystem>();
    orchestra.Add<RenderWindowSystem>();

    while (!Singleton::Get<InputState>().quit && !Singleton::Get<InputState>().keys_down[SDLK_ESCAPE]) { orchestra.RunSystems(); }

    globalData.Get<ParticleEmitter>().clear();
    globalData.Get<NodeTree>().clear();
}
