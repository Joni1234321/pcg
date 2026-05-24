#include <format>
#include <optional>

#include "0_engine/g_globals.hpp"
#include "0_engine/r_window_state.hpp"
#include "0_engine/u_algorithm.hpp"
#include "0_engine/u_collections.hpp"
#include "0_engine/u_colors.hpp"
#include "0_engine/u_fonts.hpp"
#include "0_engine/u_texture.hpp"
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
#include "SDL3_ttf/SDL_ttf.h"
#include "g_arcade.hpp"

#include "SDL3/SDL_keycode.h"
#include "SDL3/SDL_pixels.h"
#include "SDL3/SDL_render.h"

namespace pcg {
using namespace pce;
using namespace pce::ui;
namespace {
struct HexDrawInfo {
    float2 world { };
    Color color { };
};
enum class TerrainType : u8 { TERRAIN_DEEP_OCEAN, TERRAIN_OCEAN, TERRAIN_BEACH, TERRAIN_GRASS, TERRAIN_FOREST, TERRAIN_MOUNTAIN, TERRAIN_SNOW };
struct Hex {
    TerrainType terrain;
};
struct Unit {
    Echelon echelon;
    Color color;
    int2 axial;
    u32 dmg;
    u32 move;
    u32 def;
};

struct PseudoStates {
    std::optional<int2> axial_hover_now;
    std::optional<int2> axial_hover_enter;
    std::optional<int2> axial_hover_exit;

    std::optional<int2> axial_select_now;
    std::optional<int2> axial_select_enter;
    std::optional<int2> axial_select_exit;
};
struct HexState {
    PseudoStates pseudo_states;
    HexList<HexDrawInfo> hex_draw;
    HexList<Hex> hex_map;
    HandleList<Unit> units;
    Pool<Counter> counters;
    Pool<Label> label_pool;
    List<SDL_Vertex> vertecies { };
};

TerrainType FloatToTerrain(const f32 terrain) {
    if (terrain < 0.25F) { return TerrainType::TERRAIN_DEEP_OCEAN; }
    if (terrain < 0.38F) { return TerrainType::TERRAIN_OCEAN; }
    if (terrain < 0.43F) { return TerrainType::TERRAIN_BEACH; }
    if (terrain < 0.60F) { return TerrainType::TERRAIN_GRASS; }
    if (terrain < 0.72F) { return TerrainType::TERRAIN_FOREST; }
    if (terrain < 0.85F) { return TerrainType::TERRAIN_MOUNTAIN; }
    return TerrainType::TERRAIN_SNOW;
}
Color TerrainToColor(const TerrainType terrain) {
    switch (terrain) {
        case TerrainType::TERRAIN_DEEP_OCEAN: return Color { 20U, 60U, 120U };
        case TerrainType::TERRAIN_OCEAN: return Color { 50U, 100U, 180U };
        case TerrainType::TERRAIN_BEACH: return colors::KHAKI;
        case TerrainType::TERRAIN_GRASS: return Color { 100U, 190U, 80U };
        case TerrainType::TERRAIN_FOREST: return colors::FOREST_GREEN;
        case TerrainType::TERRAIN_MOUNTAIN: return colors::GRAY;
        case TerrainType::TERRAIN_SNOW: return colors::WHITE;
    }
    __builtin_unreachable();
}
HexList<Hex> GenerateTerrain(const uint2 map_size, const u32 seed) {
    HexList<Hex> hexes;
    hexes.Resize(map_size);
    constexpr f32 SCALE = 0.04F;
    const f32 seed_f = static_cast<f32>(seed);
    for (u32 i = 0; i < hexes.Size(); i++) {
        const int2 axial = hexes.IndexToAxial(i);
        const float2 world = HexAxialToWorld(axial);
        hexes[axial].terrain = FloatToTerrain((noise::Fbm(world.x * SCALE + seed_f, world.y * SCALE + seed_f) + 1.0F) * 0.5F);
    }
    return hexes;
}

HexList<HexDrawInfo> HexToHexDraw(const HexList<Hex>& hexes) {
    HexList<HexDrawInfo> result { };
    result.Resize(hexes.map_size);
    for (u32 i = 0; i < hexes.Size(); i++) {
        const Hex& hex_data = hexes.data[i];
        const int2 axial = hexes.IndexToAxial(i);
        result[axial] = HexDrawInfo { .world = HexAxialToWorld(axial), .color = TerrainToColor(hex_data.terrain) };
    }
    return result;
}

struct HexSystem {
    void operator()() const {
        const WindowState& window_state = Singleton::Get<WindowState>();
        const CameraState& camera = Singleton::Get<CameraState>();
        const InputState& input_state = Singleton::Get<InputState>();
        const FontCollection& font_collection = Singleton::Get<FontCollection>();
        HexState& hex_state = Singleton::Get<HexState>();

        // pseudo states set
        const float2 mouse_world = camera.ScreenToWorld(input_state.mouse_position);
        const Optional<int2> mouse_axial = hex_state.hex_draw.Contains(HexWorldToAxial(mouse_world)) ? Optional { HexWorldToAxial(mouse_world) } : std::nullopt;

        const b8 hover_new = hex_state.pseudo_states.axial_hover_now != mouse_axial;
        hex_state.pseudo_states.axial_hover_enter = hover_new ? mouse_axial : std::nullopt;
        hex_state.pseudo_states.axial_hover_exit = hover_new ? hex_state.pseudo_states.axial_hover_now : std::nullopt;
        hex_state.pseudo_states.axial_hover_now = mouse_axial;

        if (input_state.left_mouse_down) {
            const b8 select_new = hex_state.pseudo_states.axial_select_now != mouse_axial;
            hex_state.pseudo_states.axial_select_enter = select_new ? mouse_axial : std::nullopt;
            hex_state.pseudo_states.axial_select_exit = select_new ? hex_state.pseudo_states.axial_select_now : std::nullopt;
            hex_state.pseudo_states.axial_select_now = mouse_axial;
        }

        // pseudo states draw
        if (hex_state.pseudo_states.axial_hover_now) { HexAppend(hex_state.vertecies, camera.scale, camera.WorldToScreen(HexAxialToWorld(hex_state.pseudo_states.axial_hover_now.value())), colors::HEX_HOVER); }
        if (hex_state.pseudo_states.axial_select_now) { HexAppend(hex_state.vertecies, camera.scale * 0.95F, camera.WorldToScreen(HexAxialToWorld(hex_state.pseudo_states.axial_select_now.value())), colors::HEX_SELECT); }

        // render map
        for (const HexDrawInfo& hex : hex_state.hex_draw.data) { HexAppend(hex_state.vertecies, camera.scale * 0.90F, camera.WorldToScreen(hex.world), hex.color); }

        (void)SDL_RenderGeometry(window_state.renderer, nullptr, hex_state.vertecies.data.data(), static_cast<i32>(hex_state.vertecies.size()), nullptr, 0);
        hex_state.vertecies.clear();

        // render units
        hex_state.counters.Clear();
        for (const Unit& unit : hex_state.units) {
            Counter& counter = hex_state.counters.Get();
            counter.axial = unit.axial;
            counter.colors[0] = unit.color;
            counter.label_top.SetText(EchelonToString(unit.echelon));
            counter.label_bottom.SetText(std::format("{}-{}", unit.dmg, unit.move));
        }
        RenderCounters(hex_state.counters);

        // movement https://www.redblobgames.com/grids/hexagons/#distances
        const f32 pt = camera.scale * 0.3F;
        const Font& font_movement = font_collection.GetFontBold(static_cast<FontSizes>(pt));
        TTF_SetFontWrapAlignment(font_movement, TTF_HORIZONTAL_ALIGN_CENTER);

        if (hex_state.pseudo_states.axial_hover_now && hex_state.pseudo_states.axial_select_now && hex_state.pseudo_states.axial_hover_now != hex_state.pseudo_states.axial_select_now) {
            const int2 axial_start = hex_state.pseudo_states.axial_select_now.value();
            const OptionalHandle<Unit> unit_handle_opt = find_handle_of(hex_state.units, axial_start, &Unit::axial);
            if (unit_handle_opt.IsValid()) {
                const Handle<Unit> unit_handle = unit_handle_opt.GetHandle();
                Unit& unit = hex_state.units[unit_handle];

                constexpr Color COLOR { colors::RUBY_RED };
                (void)SDL_SetRenderDrawColor(window_state.renderer, COLOR.r, COLOR.g, COLOR.b, COLOR.a);

                const int2 axial_hover = hex_state.pseudo_states.axial_hover_now.value();
                const int3 cube_start = HexAxialToCube(axial_start);
                const int3 cube_hover = HexAxialToCube(axial_hover);
                const u32 distance = HexCubeDistance(cube_start, cube_hover);
                const u32 distance_end = math::Min(distance, unit.move);

                List<int2> axials;
                const f32 distance_inv = 1.0F / static_cast<f32>(distance);
                for (u32 i = 0; i <= distance; ++i) { axials.EmplaceBack(HexCubeToAxial(HexCubeRound(HexCubeLerp(cube_start, cube_hover, static_cast<f32>(i) * distance_inv)))); }

                const int2 axial_end = axials[distance_end];

                for (u32 i = 1; i < axials.size(); ++i) {
                    const int2& axial = axials[i];
                    const float2 world = HexAxialToWorld(axial);
                    const int2 screen = camera.WorldToScreen(world);
                    const float2 screen_f = static_cast<float2>(screen);

                    const Color movement_color = i > distance_end ? colors::BLACK : colors::RUBY_RED;
                    HexAppend(hex_state.vertecies, camera.scale * 0.25F, screen, movement_color);
                    (void)SDL_RenderGeometry(window_state.renderer, nullptr, hex_state.vertecies.data.data(), static_cast<i32>(hex_state.vertecies.size()), nullptr, 0);
                    hex_state.vertecies.clear();

                    const Label& label = hex_state.label_pool.Get();
                    const String string_distance = std::format("{}", i);
                    (void)TTF_SetTextWrapWidth(label, camera.scale);
                    (void)TTF_SetTextFont(label, font_movement);
                    (void)TTF_SetTextString(label, string_distance.c_str(), string_distance.size());
                    (void)TTF_DrawRendererText(label, screen_f.x - camera.scale * 0.5F, screen_f.y - pt * 0.5F);
                }

                // move
                if (input_state.right_mouse_down) {
                    unit.axial = axial_end;
                    unit.move -= distance_end;
                    hex_state.pseudo_states.axial_select_exit = hex_state.pseudo_states.axial_select_now;
                    hex_state.pseudo_states.axial_select_now = hex_state.pseudo_states.axial_select_enter = axial_end;
                }
            }
        }

        hex_state.label_pool.Clear();
    }
};
} // namespace

void arcade::RunHex() {
    Singleton::Get<WindowState>().clear_color = colors::LIGHT_SKY_BLUE;

    HexState& hex_state = Singleton::Get<HexState>();
    hex_state.hex_map = GenerateTerrain({ 20, 6 }, 3489);
    hex_state.hex_draw = HexToHexDraw(hex_state.hex_map);

    CameraState& camera = Singleton::Get<CameraState>();
    camera.map_world_min = { 0.0F, 0.0F };
    camera.map_world_max = HexAxialToWorld(static_cast<int2>(hex_state.hex_map.map_size - uint2 { 1, 1 }));

    // GERMAN (feldgrau) — advancing right along road r=2
    (void)hex_state.units.EmplaceBack(Unit { .echelon = Echelon::ECHELON_HQ,        .color = colors::DARK_GRAY, .axial = {  1, 2 }, .dmg = 0, .move = 0, .def = 0 });
    (void)hex_state.units.EmplaceBack(Unit { .echelon = Echelon::ECHELON_CORPS,     .color = colors::DARK_GRAY, .axial = {  3, 2 }, .dmg = 5, .move = 5, .def = 0 });
    (void)hex_state.units.EmplaceBack(Unit { .echelon = Echelon::ECHELON_BATTALION, .color = colors::DARK_GRAY, .axial = {  4, 1 }, .dmg = 4, .move = 4, .def = 0 });
    (void)hex_state.units.EmplaceBack(Unit { .echelon = Echelon::ECHELON_BRIGADE,   .color = colors::DARK_GRAY, .axial = {  4, 3 }, .dmg = 8, .move = 3, .def = 0 });
    (void)hex_state.units.EmplaceBack(Unit { .echelon = Echelon::ECHELON_COMPANY,   .color = colors::MAROON,    .axial = {  5, 2 }, .dmg = 0, .move = 0, .def = 0 }); // ART
    (void)hex_state.units.EmplaceBack(Unit { .echelon = Echelon::ECHELON_COMPANY,   .color = colors::DARK_GRAY, .axial = {  6, 1 }, .dmg = 5, .move = 4, .def = 0 });
    (void)hex_state.units.EmplaceBack(Unit { .echelon = Echelon::ECHELON_BATTALION, .color = colors::DARK_GRAY, .axial = {  6, 2 }, .dmg = 6, .move = 4, .def = 0 });

    // ALLIED (olive drab) — pulling back along road r=3
    (void)hex_state.units.EmplaceBack(Unit { .echelon = Echelon::ECHELON_BATTALION, .color = colors::OLIVE,     .axial = { 11, 3 }, .dmg = 5, .move = 6, .def = 0 });
    (void)hex_state.units.EmplaceBack(Unit { .echelon = Echelon::ECHELON_COMPANY,   .color = colors::OLIVE,     .axial = { 11, 2 }, .dmg = 4, .move = 5, .def = 0 });
    (void)hex_state.units.EmplaceBack(Unit { .echelon = Echelon::ECHELON_COMPANY,   .color = colors::BROWN,     .axial = { 13, 3 }, .dmg = 0, .move = 0, .def = 0 }); // ART
    (void)hex_state.units.EmplaceBack(Unit { .echelon = Echelon::ECHELON_BRIGADE,   .color = colors::OLIVE,     .axial = { 14, 4 }, .dmg = 6, .move = 4, .def = 0 });
    (void)hex_state.units.EmplaceBack(Unit { .echelon = Echelon::ECHELON_BATTALION, .color = colors::OLIVE,     .axial = { 14, 2 }, .dmg = 5, .move = 7, .def = 0 });
    (void)hex_state.units.EmplaceBack(Unit { .echelon = Echelon::ECHELON_CORPS,     .color = colors::OLIVE,     .axial = { 15, 3 }, .dmg = 4, .move = 6, .def = 0 });
    (void)hex_state.units.EmplaceBack(Unit { .echelon = Echelon::ECHELON_HQ,        .color = colors::OLIVE,     .axial = { 17, 3 }, .dmg = 0, .move = 0, .def = 0 }); // 7.Army

    // Systems
    Orchestra orchestra { };
    orchestra.Add<DebugSystem>();
    orchestra.Add<TickSystem>();

    orchestra.Add<InputSystem>();
    orchestra.Add<InputNodeSystem>();

    orchestra.Add<HexSystem>();

    orchestra.Add<AnimationSystem>();
    orchestra.Add<ParticleSystem>();

    orchestra.Add<CameraSystem>();
    orchestra.Add<RenderNodeSystem>();
    orchestra.Add<RenderWindowSystem>();

    while (!Singleton::Get<InputState>().quit && !Singleton::Get<InputState>().keys_down[SDLK_ESCAPE]) { orchestra.RunSystems(); }

    // Free TTF-owning state BEFORE Window destructor calls TTF_Quit/SDL_Quit,
    // otherwise Labels (TTF_Text) in CounterState are destroyed during static destruction
    // after SDL is dead, causing an access violation (0xC0000005).
    hex_state.counters.Destroy();
    globalData.Get<NodeTree>().clear();
}
} // namespace pcg
