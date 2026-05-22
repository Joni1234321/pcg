#include "0_engine/g_globals.hpp"
#include "0_engine/u_collections.hpp"
#include "0_engine/u_colors.hpp"
#include "1_systems/i_input_system.hpp"
#include "1_systems/r_camera_system.hpp"
#include "1_systems/r_hex.hpp"
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

void arcade::RunHex() {
    Singleton::Get<WindowState>().clear_color = colors::light_sky_blue;

    HexMapState& hex_map = Singleton::Get<HexMapState>();
    hex_map.AddMap({ 100, 100 });
    hex_map.GenerateTerrain(42);

    CameraState& camera = Singleton::Get<CameraState>();
    camera.map_world_min = { 0.0F, 0.0F };
    camera.map_world_max = HexAxialToWorld(int2 { static_cast<i32>(hex_map.width) - 1, static_cast<i32>(hex_map.height) - 1 });

    // Systems
    Orchestra orchestra { };
    orchestra.Add<DebugSystem>();
    orchestra.Add<TickSystem>();

    orchestra.Add<InputSystem>();
    orchestra.Add<InputNodeSystem>();

    orchestra.Add<AnimationSystem>();
    orchestra.Add<ParticleSystem>();

    orchestra.Add<CameraSystem>();
    orchestra.Add<RenderHexSystem>();
    orchestra.Add<RenderNodeSystem>();
    orchestra.Add<RenderWindowSystem>();

    while (!Singleton::Get<InputState>().quit && !Singleton::Get<InputState>().keys_down[SDLK_ESCAPE]) { orchestra.RunSystems(); }
}
}
