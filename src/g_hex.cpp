#include "0_engine/g_globals.hpp"
#include "0_engine/u_collections.hpp"
#include "0_engine/u_colors.hpp"
#include "0_engine/u_util.hpp"
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

struct HexDraw {
    float2 position { };
    SDL_Color color { };
    HexDraw() = default;
    HexDraw(float2 position, SDL_Color color) : position(position), color(color) { }
};
struct Hex {
    f32 terrain;
};
struct HexState {
    Optional<int2> axial_selected;
    HexList<HexDraw> hex_draw;
    HexList<Hex> hex_map;
};

HexList<Hex> GenerateTerrain(const uint2 map_size, u32 seed) {
    HexList<Hex> hexes;
    hexes.Resize(map_size);
    constexpr f32 SCALE = 0.04F;
    const f32 seed_f = static_cast<f32>(seed);
    for (u32 i = 0; i < hexes.Size(); i++) {
        const int2 axial = hexes.IndexToAxial(i);
        const float2 world = HexAxialToWorld(axial);
        hexes[axial].terrain = (noise::Fbm(world.x * SCALE + seed_f, world.y * SCALE + seed_f) + 1.0F) * 0.5F;
    }
    return hexes;
}
HexList<HexDraw> HexToHexDraw(const HexList<Hex>& hexes) {
    HexList<HexDraw> result { };
    result.Resize(hexes.map_size);
    for (u32 i = 0; i < hexes.Size(); i++) {
        const Hex& hex_data = hexes.data[i];
        const int2 axial = hexes.IndexToAxial(i);

        SDL_Color color;
        if (hex_data.terrain < 0.25F) {
            color = SDL_Color { 20U, 60U, 120U, 255U }; // deep ocean
        } else if (hex_data.terrain < 0.38F) {
            color = SDL_Color { 50U, 100U, 180U, 255U }; // ocean
        } else if (hex_data.terrain < 0.43F) {
            color = colors::khaki; // beach
        } else if (hex_data.terrain < 0.60F) {
            color = SDL_Color { 100U, 190U, 80U, 255U }; // grass
        } else if (hex_data.terrain < 0.72F) {
            color = colors::forest_green; // forest
        } else if (hex_data.terrain < 0.85F) {
            color = colors::gray; // mountain
        } else {
            color = colors::white; // snow
        }

        float2 world = HexAxialToWorld(axial);
        result[axial] = HexDraw { world, color };
    }
    return result;
}

struct GHexSystem {
    void operator()() const {
        const CameraState& camera = Singleton::Get<CameraState>();
        const InputState& input_state = Singleton::Get<InputState>();
        GeometryRenderingState& hex_rendering_state = Singleton::Get<GeometryRenderingState>();
        HexState& hex_state = Singleton::Get<HexState>();

        // hover
        const int2 mouse_axial = HexWorldToAxial(camera.ScreenToWorld(input_state.mouse_position));
        if (hex_state.hex_draw.Contains(mouse_axial)) {
            HexAppend(hex_rendering_state.vertecies, camera.scale, camera.WorldToScreen(HexAxialToWorld(mouse_axial)), colors::ToSDL_FColor(colors::teal));
            if (input_state.left_mouse_down) { hex_state.axial_selected = mouse_axial; }
        }
        if (hex_state.axial_selected) { HexAppend(hex_rendering_state.vertecies, camera.scale * 0.95F, camera.WorldToScreen(HexAxialToWorld(hex_state.axial_selected.value())), colors::ToSDL_FColor(colors::violet)); }

        // render map
        for (const HexDraw& hex : hex_state.hex_draw.data) { HexAppend(hex_rendering_state.vertecies, camera.scale * 0.90F, camera.WorldToScreen(hex.position), colors::ToSDL_FColor(hex.color)); }
    }
};

void arcade::RunHex() {
    Singleton::Get<WindowState>().clear_color = colors::light_sky_blue;

    GeometryRenderingState& hex_rendering_state = Singleton::Get<GeometryRenderingState>();
    HexState& hex_state = Singleton::Get<HexState>();
    hex_state.hex_map = GenerateTerrain({ 20, 6 }, 3489);
    hex_state.hex_draw = HexToHexDraw(hex_state.hex_map);

    CameraState& camera = Singleton::Get<CameraState>();
    camera.map_world_min = { 0.0F, 0.0F };
    camera.map_world_max = HexAxialToWorld(static_cast<int2>(hex_state.hex_map.map_size - uint2 { 1, 1 }));

    CounterState& counters = Singleton::Get<CounterState>();

    // GERMAN (feldgrau) — advancing right along road r=2
    counters.counters.EmplaceBack(Counter { .axial = { 1, 2 }, .colors = { colors::dark_gray, colors::deep_gold }, .text_bottom = "4.PzD", .text_top = "HQ" });
    counters.counters.EmplaceBack(Counter { .axial = { 3, 2 }, .colors = { colors::dark_gray, colors::steel_gray, colors::dark_gray }, .text_bottom = "5-5", .text_top = "xxx" });
    counters.counters.EmplaceBack(Counter { .axial = { 4, 1 }, .colors = { colors::dark_gray, colors::steel_gray }, .text_bottom = "4-4", .text_top = "II" });
    counters.counters.EmplaceBack(Counter { .axial = { 4, 3 }, .colors = { colors::dark_gray, colors::steel_gray }, .text_bottom = "8-3", .text_top = "x" });
    counters.counters.EmplaceBack(Counter { .axial = { 5, 2 }, .colors = { colors::maroon }, .text_bottom = "ART", .text_top = "I" });
    counters.counters.EmplaceBack(Counter { .axial = { 6, 1 }, .colors = { colors::dark_gray }, .text_bottom = "5-4", .text_top = "I" });
    counters.counters.EmplaceBack(Counter { .axial = { 6, 2 }, .colors = { colors::dark_gray, colors::steel_gray }, .text_bottom = "6-4", .text_top = "II" });

    // ALLIED (olive drab) — pulling back along road r=3
    counters.counters.EmplaceBack(Counter { .axial = { 11, 3 }, .colors = { colors::olive, colors::dark_dark_brown }, .text_bottom = "5-6", .text_top = "II" });
    counters.counters.EmplaceBack(Counter { .axial = { 11, 2 }, .colors = { colors::olive }, .text_bottom = "4-5", .text_top = "I" });
    counters.counters.EmplaceBack(Counter { .axial = { 13, 3 }, .colors = { colors::brown }, .text_bottom = "ART", .text_top = "I" });
    counters.counters.EmplaceBack(Counter { .axial = { 14, 4 }, .colors = { colors::olive, colors::dark_gray }, .text_bottom = "6-4", .text_top = "x" });
    counters.counters.EmplaceBack(Counter { .axial = { 14, 2 }, .colors = { colors::olive, colors::dark_dark_brown }, .text_bottom = "5-7", .text_top = "II" });
    counters.counters.EmplaceBack(Counter { .axial = { 15, 3 }, .colors = { colors::olive, colors::dark_dark_brown, colors::olive }, .text_bottom = "4-6", .text_top = "xxx" });
    counters.counters.EmplaceBack(Counter { .axial = { 17, 3 }, .colors = { colors::olive, colors::deep_gold }, .text_bottom = "7.Army", .text_top = "HQ xxxx" });

    // Systems
    Orchestra orchestra { };
    orchestra.Add<DebugSystem>();
    orchestra.Add<TickSystem>();

    orchestra.Add<InputSystem>();
    orchestra.Add<InputNodeSystem>();

    orchestra.Add<GHexSystem>();

    orchestra.Add<AnimationSystem>();
    orchestra.Add<ParticleSystem>();

    orchestra.Add<CameraSystem>();
    orchestra.Add<RenderGeometrySystem>();
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
