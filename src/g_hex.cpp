#include <algorithm>
#include <cmath>
#include <format>
#include <functional>
#include <iterator>
#include <optional>
#include <queue>
#include <ranges>
#include <unordered_map>
#include <vector>

#include "0_engine/g_globals.hpp"
#include "0_engine/r_window_state.hpp"
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
enum class CountryTag : u8 { TAG_GER, TAG_SOV, TAG_USA };
enum class TerrainType : u8 { TERRAIN_DEEP_OCEAN, TERRAIN_OCEAN, TERRAIN_BEACH, TERRAIN_GRASS, TERRAIN_FOREST, TERRAIN_MOUNTAIN, TERRAIN_SNOW };
struct Hex {
    TerrainType terrain;
};
struct Unit {
    CountryTag tag;
    Echelon echelon;
    UnitIcon icon;
    Color color;
    int2 axial;
    u32 dmg;
    u32 move;
    u32 def;
};
enum class PlayerAction { PLAYER_ACTION_NONE, PLAYER_ACTION_SELECT, PLAYER_ACTION_DESELECT, PLAYER_ACTION_MOVE_CLICK, PLAYER_ACTION_MOVE_HOVER, PLAYER_ACTION_ATTACK_CLICK, PLAYER_ACTION_ATTACK_HOVER };

struct PseudoTarget {
    int2 axial;
    List<Handle<Unit>> units;
};
struct PseudoStates {
    std::optional<int2> axial_hover;
    std::optional<int2> axial_select;
    List<Handle<Unit>> unit_handles_select;
};
struct HexState {
    HexList<Hex> hex_map;
    HandleList<Unit> units;

    // cache logic
    CountryTag player_tag;
    PlayerAction player_action;
    UnorderedMap<int2, List<Handle<Unit>>> units_by_axial;
    PseudoStates pseudo_states;

    // cache drawing
    HexList<HexDrawInfo> hex_draw;
    Pool<Counter> counters;
    Pool<Label> label_pool;
    List<SDL_Vertex> verts { };
};

[[nodiscard]] constexpr TerrainType FloatToTerrain(const f32 terrain) {
    if (terrain < 0.25F) { return TerrainType::TERRAIN_DEEP_OCEAN; }
    if (terrain < 0.38F) { return TerrainType::TERRAIN_OCEAN; }
    if (terrain < 0.43F) { return TerrainType::TERRAIN_BEACH; }
    if (terrain < 0.60F) { return TerrainType::TERRAIN_GRASS; }
    if (terrain < 0.72F) { return TerrainType::TERRAIN_FOREST; }
    if (terrain < 0.85F) { return TerrainType::TERRAIN_MOUNTAIN; }
    return TerrainType::TERRAIN_SNOW;
}
[[nodiscard]] constexpr Color TerrainToColor(const TerrainType terrain) {
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
[[nodiscard]] constexpr u32 TerrainToMovementCost(const TerrainType terrain) {
    switch (terrain) {
        case TerrainType::TERRAIN_DEEP_OCEAN: return 255U;
        case TerrainType::TERRAIN_OCEAN: return 255U;
        case TerrainType::TERRAIN_BEACH: return 2U;
        case TerrainType::TERRAIN_GRASS: return 1U;
        case TerrainType::TERRAIN_FOREST: return 3U;
        case TerrainType::TERRAIN_MOUNTAIN: return 5U;
        case TerrainType::TERRAIN_SNOW: return 4U;
    }
    __builtin_unreachable();
}
[[nodiscard]] constexpr Color CountryTagToColor(const CountryTag tag) {
    switch (tag) {
        case CountryTag::TAG_GER: return colors::WG_GER_BG;
        case CountryTag::TAG_SOV: return colors::WG_SOV_BG;
        case CountryTag::TAG_USA: return colors::WG_USA_BG;
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

struct AxialAndCost {
    u32 cost;
    int2 axial;
};
[[nodiscard]] b8 AxialIsEnemy(const HexState& hex_state, const int2 axial) { return hex_state.units_by_axial.contains(axial) && std::ranges::any_of(hex_state.units_by_axial.at(axial) | hex_state.units.handle_to_view(),  [&](const Unit& unit) -> b8 { return unit.tag != hex_state.player_tag; }); }
List<AxialAndCost> HexAStarPath(HexState& hex_state, const int2 axial_start, const int2 axial_end) {
    List<AxialAndCost> axial_path;
    auto cmp = [](const AxialAndCost& a, const AxialAndCost& b) -> ::b8 { return a.cost > b.cost; };
    std::priority_queue<AxialAndCost, std::vector<AxialAndCost>, decltype(cmp)> frontier(cmp);
    std::unordered_map<int2, int2> came_from;
    std::unordered_map<int2, u32> cost_at_axial;

    frontier.push(AxialAndCost { .cost = 0, .axial = axial_start });
    came_from[axial_start] = axial_start;
    cost_at_axial[axial_start] = 0;

    while (!frontier.empty()) {
        const AxialAndCost current = frontier.top();
        frontier.pop();

        if (current.axial == axial_end) { break; } // finished

        for (const int2 axial_neighbour_offset : HEX_AXIAL_NEIGHBOURS) {
            const int2 axial_next = current.axial + axial_neighbour_offset;
            if (!hex_state.hex_map.Contains(axial_next) || AxialIsEnemy(hex_state, axial_next)) { continue; }

            const u32 cost = TerrainToMovementCost(hex_state.hex_map[axial_next].terrain);
            const u32 new_cost = cost_at_axial[current.axial] + cost;
            if (!cost_at_axial.contains(axial_next) || new_cost < cost_at_axial[axial_next]) {
                const u32 heuristic_distance = HexCubeDistance(HexAxialToCube(axial_next), HexAxialToCube(axial_end));
                cost_at_axial[axial_next] = new_cost;
                frontier.push({ .cost = new_cost + heuristic_distance, .axial = axial_next });
                came_from[axial_next] = current.axial;
            }
        }
    }
    if (came_from.contains(axial_end)) {
        for (int2 axial = axial_end; axial != axial_start; axial = came_from[axial]) { axial_path.EmplaceBack(AxialAndCost { .cost = cost_at_axial[axial], .axial = axial }); }
        std::ranges::reverse(axial_path);
    }
    return axial_path;
}
PlayerAction GetPlayerAction(const HexState& hex_state) {
    const InputState& input_state = Singleton::Get<InputState>();

    if (hex_state.pseudo_states.axial_hover.has_value()) {
        // unit selection
        if (input_state.left_mouse_down) {
            return hex_state.units_by_axial.contains(hex_state.pseudo_states.axial_hover.value()) ? PlayerAction::PLAYER_ACTION_SELECT : PlayerAction::PLAYER_ACTION_DESELECT;
        }
        if (!hex_state.pseudo_states.unit_handles_select.empty()) {
            auto units_selected = hex_state.pseudo_states.unit_handles_select | hex_state.units.handle_to_view();
            if (AxialIsEnemy(hex_state, hex_state.pseudo_states.axial_hover.value())) {
                const u32 units_selected_movement = std::ranges::min(units_selected | std::views::transform(&Unit::move));
                constexpr u32 MOVE_COST_ATTACK = 3U;
                if (units_selected_movement > MOVE_COST_ATTACK) { return input_state.right_mouse_down ? PlayerAction::PLAYER_ACTION_ATTACK_CLICK : PlayerAction::PLAYER_ACTION_ATTACK_HOVER; }
            } else if (hex_state.pseudo_states.axial_hover != hex_state.pseudo_states.axial_select) { return input_state.right_mouse_down ? PlayerAction::PLAYER_ACTION_MOVE_CLICK : PlayerAction::PLAYER_ACTION_MOVE_HOVER; }
        }
    }
    return PlayerAction::PLAYER_ACTION_NONE;
}

struct HexSystem {
    void operator()() const {
        const WindowState& window_state = Singleton::Get<WindowState>();
        const CameraState& camera = Singleton::Get<CameraState>();
        const InputState& input_state = Singleton::Get<InputState>();
        const FontCollection& font_collection = Singleton::Get<FontCollection>();
        HexState& hex_state = Singleton::Get<HexState>();

        // precompute
        hex_state.label_pool.Clear();
        hex_state.units_by_axial.clear();
        for (u32 i = 0; i < hex_state.units.size(); i++) {
            Handle<Unit> unit_handle = hex_state.units.IndexToHandle(i);
            hex_state.units_by_axial[hex_state.units[unit_handle].axial].EmplaceBack(unit_handle);
        }

        // pseudo states set
        const float2 mouse_world = camera.ScreenToWorld(input_state.mouse_position);
        hex_state.pseudo_states.axial_hover = hex_state.hex_draw.Contains(HexWorldToAxial(mouse_world)) ? Optional { HexWorldToAxial(mouse_world) } : std::nullopt;
        hex_state.player_tag = CountryTag::TAG_GER;
        hex_state.player_action = PlayerAction::PLAYER_ACTION_NONE;

        // pseudo states draw
        if (hex_state.pseudo_states.axial_hover) { HexAppend(hex_state.verts, camera.scale, camera.WorldToScreen(HexAxialToWorld(hex_state.pseudo_states.axial_hover.value())), colors::HEX_HOVER); }
        if (hex_state.pseudo_states.axial_select) { HexAppend(hex_state.verts, camera.scale * 0.95F, camera.WorldToScreen(HexAxialToWorld(hex_state.pseudo_states.axial_select.value())), colors::HEX_SELECT); }

        // render map
        for (const HexDrawInfo& hex : hex_state.hex_draw.data) { HexAppend(hex_state.verts, camera.scale * 0.90F, camera.WorldToScreen(hex.world), hex.color); }
        (void)SDL_RenderGeometry(window_state.renderer, nullptr, hex_state.verts.data.data(), static_cast<i32>(hex_state.verts.size()), nullptr, 0);
        hex_state.verts.clear();

        // render units
        hex_state.counters.Clear();
        for (const auto& [axial, unit_handles] : hex_state.units_by_axial) {
            Counter& counter = hex_state.counters.Get();
            counter.axial = axial;
            Echelon echelon { Echelon::ECHELON_SQUAD };
            UnitIcon icon { UnitIcon::ICON_INF };
            u32 dmg = 0;
            u32 move = 0;
            counter.stack = { };
            const u32 counters_on_hex = math::Min<u32>(counter.stack.size(), unit_handles.size());
            if (hex_state.pseudo_states.axial_select == axial) {
                // drawing selected
                u32 i = 0;
                for (; i < hex_state.pseudo_states.unit_handles_select.size(); i++) {
                    const Handle<Unit> unit_handle_selected = hex_state.pseudo_states.unit_handles_select[i];
                    const Unit& unit_selected = hex_state.units[unit_handle_selected];
                    counter.stack[i] = CounterStack { .color_background = CountryTagToColor(unit_selected.tag), .color_icon = unit_selected.color, .color_border = colors::HEX_SELECT };
                    dmg += unit_selected.dmg;
                    move = math::Max(unit_selected.move, move);
                    if (unit_selected.echelon >= echelon) {
                        echelon = unit_selected.echelon;
                        icon = unit_selected.icon;
                    }
                }

                // only color for rest
                for (u32 j = 0; j < counters_on_hex; j++) {
                    const Handle<Unit> unit_handle = unit_handles[j];
                    if (hex_state.pseudo_states.unit_handles_select.Contains(unit_handle)) { continue; }
                    const Unit& unit = hex_state.units[unit_handle];
                    counter.stack[i++] = CounterStack { .color_background = CountryTagToColor(unit.tag), .color_icon = unit.color, .color_border = colors::YELLOW };
                }
            } else {
                // drawing plain
                for (u32 i = 0; i < counters_on_hex; i++) {
                    const Handle<Unit> unit_handle = unit_handles[i];
                    const Unit& unit = hex_state.units[unit_handle];
                    counter.stack[i] = CounterStack { .color_background = CountryTagToColor(unit.tag), .color_icon = unit.color, .color_border = colors::BLACK };
                    dmg += unit.dmg;
                    move = math::Max(unit.move, move);
                    if (unit.echelon >= echelon) {
                        echelon = unit.echelon;
                        icon = unit.icon;
                    }
                }
            }
            counter.label_top.SetText(EchelonToString(echelon));
            counter.label_center.SetText(UnitIconToString(icon));
            counter.label_bottom.SetText(std::format("{}-{}", dmg, move));
        }
        RenderCounters(hex_state.counters);

        hex_state.player_action = GetPlayerAction(hex_state);
        switch (hex_state.player_action) {
            case PlayerAction::PLAYER_ACTION_NONE:
                break;
            case PlayerAction::PLAYER_ACTION_SELECT:
            {
                const b8 select_new = hex_state.pseudo_states.axial_select != hex_state.pseudo_states.axial_hover;
                hex_state.pseudo_states.axial_select = hex_state.pseudo_states.axial_hover;
                const List<Handle<Unit>>& unit_handles = hex_state.units_by_axial[hex_state.pseudo_states.axial_hover.value()];
                if (select_new) {
                    hex_state.pseudo_states.unit_handles_select = List { { unit_handles[0] } };
                } else {
                    const u32 i = unit_handles.IndexOf(hex_state.pseudo_states.unit_handles_select[0]);
                    const u32 next = i + 1; // select next or all
                    hex_state.pseudo_states.unit_handles_select = next == unit_handles.size() ? unit_handles : List { { unit_handles[next] } };
                }
                break;
            }
            case PlayerAction::PLAYER_ACTION_DESELECT: {
                hex_state.pseudo_states.axial_select = std::nullopt;
                hex_state.pseudo_states.unit_handles_select.clear();
                break;
            }
            case PlayerAction::PLAYER_ACTION_MOVE_CLICK: {
                // movement https://www.redblobgames.com/grids/hexagons/#distances
                const int2 axial_start = hex_state.pseudo_states.axial_select.value();
                const int2 axial_hover = hex_state.pseudo_states.axial_hover.value();
                auto units_selected = hex_state.pseudo_states.unit_handles_select | hex_state.units.handle_to_view();
                const u32 units_selected_movement = std::ranges::min(units_selected | std::views::transform(&Unit::move));

                constexpr Color COLOR { colors::RUBY_RED };
                (void)SDL_SetRenderDrawColor(window_state.renderer, COLOR.r, COLOR.g, COLOR.b, COLOR.a);

                // path finding
                List<AxialAndCost> axial_path = HexAStarPath(hex_state, axial_start, axial_hover);
                const auto it = std::ranges::upper_bound(axial_path, units_selected_movement, std::less { }, &AxialAndCost::cost);
                if (it != axial_path.begin()) {
                    const AxialAndCost& cost_and_axial_end = *std::prev(it);
                    for (Unit& unit : units_selected) {
                        unit.axial = cost_and_axial_end.axial;
                        unit.move -= cost_and_axial_end.cost;
                    }
                    hex_state.pseudo_states.axial_select = cost_and_axial_end.axial;
                }
                break;
            }
            case PlayerAction::PLAYER_ACTION_MOVE_HOVER: {
                const int2 axial_start = hex_state.pseudo_states.axial_select.value();
                const int2 axial_hover = hex_state.pseudo_states.axial_hover.value();

                auto units_selected = hex_state.pseudo_states.unit_handles_select | hex_state.units.handle_to_view();
                const u32 units_selected_movement = std::ranges::min(units_selected | std::views::transform(&Unit::move));

                List<AxialAndCost> axial_path = HexAStarPath(hex_state, axial_start, axial_hover);

                const f32 pt = camera.scale * 0.3F;
                const Font& font_movement = font_collection.GetFontBold(static_cast<FontSizes>(pt));
                TTF_SetFontWrapAlignment(font_movement, TTF_HORIZONTAL_ALIGN_CENTER);
                for (AxialAndCost cost_and_axial : axial_path) {
                    const float2 world = HexAxialToWorld(cost_and_axial.axial);
                    const int2 screen = camera.WorldToScreen(world);
                    const float2 screen_f = static_cast<float2>(screen);

                    const Color movement_color = cost_and_axial.cost > units_selected_movement ? colors::BLACK : colors::RUBY_RED;
                    HexAppend(hex_state.verts, camera.scale * 0.25F, screen, movement_color);
                    (void)SDL_RenderGeometry(window_state.renderer, nullptr, hex_state.verts.data.data(), static_cast<i32>(hex_state.verts.size()), nullptr, 0);
                    hex_state.verts.clear();

                    const Label& label = hex_state.label_pool.Get();
                    const String string_distance = std::format("{}", cost_and_axial.cost);
                    (void)TTF_SetTextWrapWidth(label, camera.scale);
                    (void)TTF_SetTextFont(label, font_movement);
                    (void)TTF_SetTextString(label, string_distance.c_str(), string_distance.size());
                    (void)TTF_DrawRendererText(label, screen_f.x - camera.scale * 0.5F, screen_f.y - pt * 0.5F);

                }
                break;
        }
            case PlayerAction::PLAYER_ACTION_ATTACK_CLICK: {
                auto units_selected = hex_state.pseudo_states.unit_handles_select | hex_state.units.handle_to_view();
                auto units_hover = hex_state.units_by_axial[hex_state.pseudo_states.axial_hover.value()] | hex_state.units.handle_to_view();
                // attack
                const u32 dmg = std::ranges::fold_left(units_selected | std::views::transform(&Unit::dmg), 0U, std::plus { });
                const u32 def = std::ranges::fold_left(units_hover | std::views::transform(&Unit::def), 0U, std::plus { });
                const f32 ratio = static_cast<f32>(dmg) / static_cast<f32>(def);
                if (ratio >= 2.0F) {
                    for (Unit& defender : units_hover) {
                        defender.dmg = math::SaturatingSub(defender.dmg, 3U);
                        defender.def = math::SaturatingSub(defender.def, 3U);
                    }
                    for (Unit& attacker : units_selected) {
                        attacker.dmg = math::SaturatingSub(attacker.dmg, 1U);
                        attacker.def = math::SaturatingSub(attacker.def, 1U);
                    }
                } else {
                    for (Unit& attacker : units_selected) {
                        attacker.dmg = math::SaturatingSub(attacker.dmg, 3U);
                        attacker.def = math::SaturatingSub(attacker.def, 3U);
                    }
                    for (Unit& defender : units_hover) {
                        defender.dmg = math::SaturatingSub(defender.dmg, 1U);
                        defender.def = math::SaturatingSub(defender.def, 1U);
                    }
                }
                break;
            }
            case PlayerAction::PLAYER_ACTION_ATTACK_HOVER: break;
        }
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

    f32 hue = 0.0F;
    auto next_color = [&hue]() -> Color {
        const Color c = Color::FromHsl(hue, 0.5F, 0.5F);
        hue = std::fmod(hue + 37.0F, 360.0F);
        return c;
    };

    (void)hex_state.units.EmplaceBack(Unit { .tag = CountryTag::TAG_GER, .echelon = Echelon::ECHELON_ARMY, .icon = UnitIcon::ICON_HQ, .color = next_color(), .axial = { 0, 2 }, .dmg = 0, .move = 50, .def = 0 });       // Heeresgruppe HQ
    (void)hex_state.units.EmplaceBack(Unit { .tag = CountryTag::TAG_GER, .echelon = Echelon::ECHELON_CORPS, .icon = UnitIcon::ICON_HQ, .color = next_color(), .axial = { 2, 2 }, .dmg = 0, .move = 50, .def = 0 });      // I.Korps HQ
    (void)hex_state.units.EmplaceBack(Unit { .tag = CountryTag::TAG_GER, .echelon = Echelon::ECHELON_REGIMENT, .icon = UnitIcon::ICON_INF, .color = next_color(), .axial = { 4, 1 }, .dmg = 6, .move = 7, .def = 2 });   // Inf.Rgt.7
    (void)hex_state.units.EmplaceBack(Unit { .tag = CountryTag::TAG_GER, .echelon = Echelon::ECHELON_REGIMENT, .icon = UnitIcon::ICON_INF, .color = next_color(), .axial = { 4, 3 }, .dmg = 6, .move = 7, .def = 2 });   // Inf.Rgt.8
    (void)hex_state.units.EmplaceBack(Unit { .tag = CountryTag::TAG_GER, .echelon = Echelon::ECHELON_BATTALION, .icon = UnitIcon::ICON_TANK, .color = next_color(), .axial = { 5, 2 }, .dmg = 7, .move = 7, .def = 1 }); // Pz.Abt.5
    (void)hex_state.units.EmplaceBack(Unit { .tag = CountryTag::TAG_GER, .echelon = Echelon::ECHELON_BATTALION, .icon = UnitIcon::ICON_ART, .color = next_color(), .axial = { 3, 2 }, .dmg = 5, .move = 7, .def = 0 });  // Art.Abt.3
    (void)hex_state.units.EmplaceBack(Unit { .tag = CountryTag::TAG_GER, .echelon = Echelon::ECHELON_BATTALION, .icon = UnitIcon::ICON_INF, .color = next_color(), .axial = { 6, 1 }, .dmg = 5, .move = 7, .def = 1 });  // II./Rgt.7
    (void)hex_state.units.EmplaceBack(Unit { .tag = CountryTag::TAG_GER, .echelon = Echelon::ECHELON_BATTALION, .icon = UnitIcon::ICON_INF, .color = next_color(), .axial = { 6, 3 }, .dmg = 5, .move = 7, .def = 1 });  // II./Rgt.8

    (void)hex_state.units.EmplaceBack(Unit { .tag = CountryTag::TAG_SOV, .echelon = Echelon::ECHELON_ARMY, .icon = UnitIcon::ICON_HQ, .color = next_color(), .axial = { 14, 3 }, .dmg = 0, .move = 50, .def = 0 });       // Front HQ
    (void)hex_state.units.EmplaceBack(Unit { .tag = CountryTag::TAG_SOV, .echelon = Echelon::ECHELON_CORPS, .icon = UnitIcon::ICON_HQ, .color = next_color(), .axial = { 13, 3 }, .dmg = 0, .move = 50, .def = 0 });      // I Gds Corps HQ
    (void)hex_state.units.EmplaceBack(Unit { .tag = CountryTag::TAG_SOV, .echelon = Echelon::ECHELON_REGIMENT, .icon = UnitIcon::ICON_INF, .color = next_color(), .axial = { 11, 2 }, .dmg = 5, .move = 7, .def = 3 });   // 16th Rifle Rgt
    (void)hex_state.units.EmplaceBack(Unit { .tag = CountryTag::TAG_SOV, .echelon = Echelon::ECHELON_REGIMENT, .icon = UnitIcon::ICON_INF, .color = next_color(), .axial = { 11, 4 }, .dmg = 5, .move = 7, .def = 3 });   // 18th Rifle Rgt
    (void)hex_state.units.EmplaceBack(Unit { .tag = CountryTag::TAG_SOV, .echelon = Echelon::ECHELON_BATTALION, .icon = UnitIcon::ICON_TANK, .color = next_color(), .axial = { 10, 3 }, .dmg = 6, .move = 7, .def = 1 }); // T-34 Bn
    (void)hex_state.units.EmplaceBack(Unit { .tag = CountryTag::TAG_SOV, .echelon = Echelon::ECHELON_BATTALION, .icon = UnitIcon::ICON_ART, .color = next_color(), .axial = { 12, 3 }, .dmg = 5, .move = 7, .def = 0 });  // 62nd Art Bn
    (void)hex_state.units.EmplaceBack(Unit { .tag = CountryTag::TAG_SOV, .echelon = Echelon::ECHELON_BATTALION, .icon = UnitIcon::ICON_INF, .color = next_color(), .axial = { 9, 2 }, .dmg = 4, .move = 7, .def = 2 });   // 1/16th Bn
    (void)hex_state.units.EmplaceBack(Unit { .tag = CountryTag::TAG_SOV, .echelon = Echelon::ECHELON_BATTALION, .icon = UnitIcon::ICON_INF, .color = next_color(), .axial = { 9, 4 }, .dmg = 4, .move = 7, .def = 2 });   // 1/18th Bn

    (void)hex_state.units.EmplaceBack(Unit { .tag = CountryTag::TAG_SOV, .echelon = Echelon::ECHELON_REGIMENT, .icon = UnitIcon::ICON_INF, .color = next_color(), .axial = { 4, 2 }, .dmg = 4, .move = 7, .def = 2 }); // 55th Rifle Rgt (encircled)

    (void)hex_state.units.EmplaceBack(Unit { .tag = CountryTag::TAG_USA, .echelon = Echelon::ECHELON_ARMY, .icon = UnitIcon::ICON_HQ, .color = next_color(), .axial = { 17, 1 }, .dmg = 0, .move = 50, .def = 0 });       // 12th Army Group HQ
    (void)hex_state.units.EmplaceBack(Unit { .tag = CountryTag::TAG_USA, .echelon = Echelon::ECHELON_CORPS, .icon = UnitIcon::ICON_HQ, .color = next_color(), .axial = { 17, 2 }, .dmg = 0, .move = 50, .def = 0 });      // V Corps HQ
    (void)hex_state.units.EmplaceBack(Unit { .tag = CountryTag::TAG_USA, .echelon = Echelon::ECHELON_REGIMENT, .icon = UnitIcon::ICON_INF, .color = next_color(), .axial = { 17, 3 }, .dmg = 6, .move = 7, .def = 2 });   // 16th Inf Rgt
    (void)hex_state.units.EmplaceBack(Unit { .tag = CountryTag::TAG_USA, .echelon = Echelon::ECHELON_BATTALION, .icon = UnitIcon::ICON_TANK, .color = next_color(), .axial = { 17, 4 }, .dmg = 7, .move = 7, .def = 1 }); // 745th Tank Bn (Sherman)

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
    hex_state.label_pool.Destroy();
    globalData.Get<NodeTree>().clear();
}
} // namespace pcg
