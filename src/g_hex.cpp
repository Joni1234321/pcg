#include "0_engine/g_globals.hpp"
#include "0_engine/u_collections.hpp"
#include "0_engine/u_colors.hpp"
#include "1_systems/i_input_system.hpp"
#include "1_systems/r_camera_system.hpp"
#include "1_systems/r_counter_system.hpp"
#include "1_systems/r_hex_system.hpp"
#include "1_systems/r_render.hpp"
#include "1_systems/r_ui_node.hpp"
#include "1_systems/r_ui_node_data.hpp"
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
    hex_map.AddMap({ 20, 6 });
    hex_map.GenerateTerrain(3489);

    CameraState& camera = Singleton::Get<CameraState>();
    camera.map_world_min = { 0.0F, 0.0F };
    camera.map_world_max = HexAxialToWorld(int2 { static_cast<i32>(hex_map.width) - 1, static_cast<i32>(hex_map.height) - 1 });

    CounterState& counters = Singleton::Get<CounterState>();

    // GERMAN (feldgrau) — advancing right along road r=2
    counters.counters.EmplaceBack(Counter { .axial = {  1, 2 }, .colors = {colors::dark_gray, colors::deep_gold},                          .text_bottom = "4.PzD", .text_top = "HQ"  });
    counters.counters.EmplaceBack(Counter { .axial = {  3, 2 }, .colors = {colors::dark_gray, colors::steel_gray, colors::dark_gray},       .text_bottom = "5-5",   .text_top = "xxx" });
    counters.counters.EmplaceBack(Counter { .axial = {  4, 1 }, .colors = {colors::dark_gray, colors::steel_gray},                         .text_bottom = "4-4",   .text_top = "II"  });
    counters.counters.EmplaceBack(Counter { .axial = {  4, 3 }, .colors = {colors::dark_gray, colors::steel_gray},                         .text_bottom = "8-3",   .text_top = "x"   });
    counters.counters.EmplaceBack(Counter { .axial = {  5, 2 }, .colors = {colors::maroon},                                                .text_bottom = "ART",   .text_top = "I"   });
    counters.counters.EmplaceBack(Counter { .axial = {  6, 1 }, .colors = {colors::dark_gray},                                             .text_bottom = "5-4",   .text_top = "I"   });
    counters.counters.EmplaceBack(Counter { .axial = {  6, 2 }, .colors = {colors::dark_gray, colors::steel_gray},                         .text_bottom = "6-4",   .text_top = "II"  });

    // ALLIED (olive drab) — pulling back along road r=3
    counters.counters.EmplaceBack(Counter { .axial = { 11, 3 }, .colors = {colors::olive, colors::dark_dark_brown},                        .text_bottom = "5-6",   .text_top = "II"  });
    counters.counters.EmplaceBack(Counter { .axial = { 11, 2 }, .colors = {colors::olive},                                                 .text_bottom = "4-5",   .text_top = "I"   });
    counters.counters.EmplaceBack(Counter { .axial = { 13, 3 }, .colors = {colors::brown},                                                 .text_bottom = "ART",   .text_top = "I"   });
    counters.counters.EmplaceBack(Counter { .axial = { 14, 4 }, .colors = {colors::olive, colors::dark_gray},                              .text_bottom = "6-4",   .text_top = "x"   });
    counters.counters.EmplaceBack(Counter { .axial = { 14, 2 }, .colors = {colors::olive, colors::dark_dark_brown},                        .text_bottom = "5-7",   .text_top = "II"  });
    counters.counters.EmplaceBack(Counter { .axial = { 15, 3 }, .colors = {colors::olive, colors::dark_dark_brown, colors::olive},          .text_bottom = "4-6",   .text_top = "xxx" });
    counters.counters.EmplaceBack(Counter { .axial = { 17, 3 }, .colors = {colors::olive, colors::deep_gold},                              .text_bottom = "7.Army", .text_top = "HQ xxxx" });


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
    orchestra.Add<RenderCounterSystem>();
    orchestra.Add<RenderNodeSystem>();
    orchestra.Add<RenderWindowSystem>();

    while (!Singleton::Get<InputState>().quit && !Singleton::Get<InputState>().keys_down[SDLK_ESCAPE]) { orchestra.RunSystems(); }

    // Free TTF-owning state BEFORE Window destructor calls TTF_Quit/SDL_Quit,
    // otherwise Labels (TTF_Text) in CounterState are destroyed during static destruction
    // after SDL is dead, causing an access violation (0xC0000005).
    Singleton::Get<CounterState>().counters.clear();
    globalData.Get<NodeTree>().clear();
}
}
