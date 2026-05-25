#include "g_hex.hpp"

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
#include "1_systems/d_imgui_system.hpp"
#include "SDL3_ttf/SDL_ttf.h"
#include "g_arcade.hpp"

#include "SDL3/SDL_keycode.h"
#include "SDL3/SDL_pixels.h"
#include "SDL3/SDL_render.h"

namespace pcg {
using namespace pce;
using namespace pce::ui;
namespace {
HexList<HexDrawInfo> HexToHexDrawInfo(const HexList<Hex>& hexes, const TerrainScheme scheme) {
    HexList<HexDrawInfo> result { };
    result.Resize(hexes.map_size);
    for (u32 i = 0; i < hexes.Size(); i++) {
        const Hex& hex_data = hexes.data[i];
        const int2 axial = hexes.IndexToAxial(i);
        result[axial] = HexDrawInfo { .world = HexAxialToWorld(axial), .color = TerrainToColor(hex_data.terrain, scheme), .overlay = colors::CLEAR, .terrain = hex_data.terrain };
    }
    return result;
}

// Per-terrain decorations rendered on top of the inner hex fill so the map
// reads as more than a sea of flat polygons. These are simple SDL_RenderGeometry
// primitives (triangles and small filled rects) sized relative to the camera
// scale; they're driven by HexTuning.show_decorations / decoration_alpha.
void TerrainDecorationAppend(List<SDL_Vertex>& verts, TerrainType terrain, int2 hex_screen, f32 scale, f32 alpha) {
    const auto tri = [&](float2 c, f32 r, ColorF col) {
        col.a *= alpha;
        verts.EmplaceBack(SDL_FPoint { c.x,             c.y - r        }, col, SDL_FPoint { });
        verts.EmplaceBack(SDL_FPoint { c.x - r * 0.9F, c.y + r * 0.7F }, col, SDL_FPoint { });
        verts.EmplaceBack(SDL_FPoint { c.x + r * 0.9F, c.y + r * 0.7F }, col, SDL_FPoint { });
    };
    const auto rect = [&](float2 c, f32 w, f32 h, ColorF col) {
        col.a *= alpha;
        const f32 x0 = c.x - w; const f32 x1 = c.x + w;
        const f32 y0 = c.y - h; const f32 y1 = c.y + h;
        verts.EmplaceBack(SDL_FPoint { x0, y0 }, col, SDL_FPoint { });
        verts.EmplaceBack(SDL_FPoint { x1, y0 }, col, SDL_FPoint { });
        verts.EmplaceBack(SDL_FPoint { x0, y1 }, col, SDL_FPoint { });
        verts.EmplaceBack(SDL_FPoint { x1, y0 }, col, SDL_FPoint { });
        verts.EmplaceBack(SDL_FPoint { x1, y1 }, col, SDL_FPoint { });
        verts.EmplaceBack(SDL_FPoint { x0, y1 }, col, SDL_FPoint { });
    };
    const float2 c { static_cast<f32>(hex_screen.x), static_cast<f32>(hex_screen.y) };
    const f32    s = scale;
    switch (terrain) {
        case TerrainType::TERRAIN_FOREST: {
            const ColorF green { 0.08F, 0.30F, 0.12F, 1.0F };
            tri({ c.x - s * 0.32F, c.y + s * 0.12F }, s * 0.22F, green);
            tri({ c.x + s * 0.32F, c.y + s * 0.12F }, s * 0.22F, green);
            tri({ c.x,             c.y - s * 0.28F }, s * 0.22F, green);
        } break;
        case TerrainType::TERRAIN_MOUNTAIN: {
            const ColorF dark  { 0.20F, 0.18F, 0.18F, 1.0F };
            const ColorF light { 0.70F, 0.68F, 0.66F, 1.0F };
            tri({ c.x - s * 0.22F, c.y + s * 0.05F }, s * 0.42F, dark);
            tri({ c.x + s * 0.22F, c.y + s * 0.05F }, s * 0.42F, light);
        } break;
        case TerrainType::TERRAIN_GRASS: {
            // small darker dashes evoke crop rows / open fields
            const ColorF dash { 0.20F, 0.45F, 0.20F, 0.7F };
            rect({ c.x - s * 0.25F, c.y - s * 0.15F }, s * 0.12F, s * 0.025F, dash);
            rect({ c.x + s * 0.18F, c.y + s * 0.05F }, s * 0.14F, s * 0.025F, dash);
            rect({ c.x - s * 0.08F, c.y + s * 0.22F }, s * 0.12F, s * 0.025F, dash);
        } break;
        case TerrainType::TERRAIN_DEEP_OCEAN:
        case TerrainType::TERRAIN_OCEAN: {
            // two faint horizontal wave bars
            const ColorF wave { 0.85F, 0.92F, 1.00F, 0.35F };
            rect({ c.x, c.y - s * 0.22F }, s * 0.32F, s * 0.025F, wave);
            rect({ c.x, c.y + s * 0.10F }, s * 0.28F, s * 0.025F, wave);
        } break;
        case TerrainType::TERRAIN_BEACH: {
            // sand stipple
            const ColorF dot { 0.55F, 0.45F, 0.30F, 0.8F };
            rect({ c.x - s * 0.20F, c.y               }, s * 0.04F, s * 0.04F, dot);
            rect({ c.x + s * 0.15F, c.y - s * 0.12F  }, s * 0.04F, s * 0.04F, dot);
            rect({ c.x,             c.y + s * 0.20F  }, s * 0.04F, s * 0.04F, dot);
        } break;
        case TerrainType::TERRAIN_SNOW: {
            // small cross / snowflake glyph
            const ColorF flake { 0.75F, 0.85F, 0.95F, 0.9F };
            rect({ c.x, c.y }, s * 0.10F, s * 0.02F, flake);
            rect({ c.x, c.y }, s * 0.02F, s * 0.10F, flake);
        } break;
    }
}

// thick line segment via two triangles - normal-based offsetting.
inline void SegmentAppend(List<SDL_Vertex>& verts, float2 p0, float2 p1, f32 half_width, ColorF col) {
    const float2 d { p1.x - p0.x, p1.y - p0.y };
    const f32 len = std::sqrt(d.x * d.x + d.y * d.y);
    if (len <= 0.0001F) { return; }
    const float2 n { -d.y / len * half_width, d.x / len * half_width };
    const SDL_FPoint a0 { p0.x + n.x, p0.y + n.y };
    const SDL_FPoint a1 { p0.x - n.x, p0.y - n.y };
    const SDL_FPoint b0 { p1.x + n.x, p1.y + n.y };
    const SDL_FPoint b1 { p1.x - n.x, p1.y - n.y };
    verts.EmplaceBack(a0, col, SDL_FPoint { });
    verts.EmplaceBack(b0, col, SDL_FPoint { });
    verts.EmplaceBack(b1, col, SDL_FPoint { });
    verts.EmplaceBack(a0, col, SDL_FPoint { });
    verts.EmplaceBack(b1, col, SDL_FPoint { });
    verts.EmplaceBack(a1, col, SDL_FPoint { });
}

// 2D integer hash - cheap and stable across runs so the same hex always has
// the same village/city/road/river layout.
[[nodiscard]] inline u32 AxialHash(const int2 a) {
    u32 h = static_cast<u32>(a.x) * 73856093U ^ static_cast<u32>(a.y) * 19349663U;
    h ^= h >> 13;
    h *= 0x5bd1e995U;
    h ^= h >> 15;
    return h;
}
[[nodiscard]] inline u32 EdgeHash(int2 a, int2 b) {
    if (a.x > b.x || (a.x == b.x && a.y > b.y)) { const int2 t = a; a = b; b = t; }
    return AxialHash(a) * 0x9e3779b9U ^ AxialHash(b);
}
[[nodiscard]] inline b8 TerrainIsLand(const TerrainType t) {
    return t != TerrainType::TERRAIN_DEEP_OCEAN && t != TerrainType::TERRAIN_OCEAN;
}
[[nodiscard]] inline b8 TerrainBuildable(const TerrainType t) {
    return t == TerrainType::TERRAIN_GRASS || t == TerrainType::TERRAIN_BEACH || t == TerrainType::TERRAIN_FOREST;
}

[[nodiscard]] b8 AxialIsEnemy(const HexState& hex_state, const int2 axial) {
    return hex_state.units_by_axial.contains(axial) && !std::ranges::contains(hex_state.units_by_axial.at(axial) | hex_state.units.handle_to_view(), hex_state.player_tag, &Unit::tag);
}
List<AxialAndCost> HexAxialPathAStar(HexState& hex_state, const int2 axial_start, const int2 axial_end) {
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
                const u32 heuristic_distance = HexAxialDistance(axial_next, axial_end);
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

void GenerateTerritory(HexState& hex_state) {
    std::ranges::fill(hex_state.hex_map | std::views::transform(&Hex::country_tag), CountryTag::TAG_NONE);

    std::queue<int2> frontier;
    for (const auto& [axial, unit_handles] : hex_state.units_by_axial) {
        if (!hex_state.hex_map.Contains(axial)) { continue; }
        hex_state.hex_map[axial].country_tag = hex_state.units[unit_handles[0]].tag;
        frontier.push(axial);
    }

    while (!frontier.empty()) {
        const int2 current = frontier.front();
        frontier.pop();
        const CountryTag current_tag = hex_state.hex_map[current].country_tag;
        for (const int2 offset : HEX_AXIAL_NEIGHBOURS) {
            const int2 next = current + offset;
            if (!hex_state.hex_map.Contains(next)) { continue; }
            if (hex_state.hex_map[next].country_tag != CountryTag::TAG_NONE) { continue; }
            hex_state.hex_map[next].country_tag = current_tag;
            frontier.push(next);
        }
    }

    // compute distance from border (adjacent to different tag)
    std::ranges::fill(hex_state.hex_map | std::views::transform(&Hex::territory_distance), u8 { 255U });
    std::queue<int2> border_frontier;
    for (u32 i = 0; i < hex_state.hex_map.Size(); i++) {
        const int2 axial = hex_state.hex_map.IndexToAxial(i);
        const CountryTag tag = hex_state.hex_map[axial].country_tag;
        const b8 is_border = std::ranges::any_of(HEX_AXIAL_NEIGHBOURS, [&](const int2 offset) -> b8 {
            const int2 neighbour = axial + offset;
            return !hex_state.hex_map.Contains(neighbour) || hex_state.hex_map[neighbour].country_tag != tag;
        });
        if (is_border) { hex_state.hex_map[axial].territory_distance = 0U; border_frontier.push(axial); }
    }
    while (!border_frontier.empty()) {
        const int2 current = border_frontier.front();
        border_frontier.pop();
        const u8 next_dist = hex_state.hex_map[current].territory_distance + 1U;
        for (const int2 offset : HEX_AXIAL_NEIGHBOURS) {
            const int2 next = current + offset;
            if (!hex_state.hex_map.Contains(next)) { continue; }
            if (hex_state.hex_map[next].territory_distance != 255U) { continue; }
            hex_state.hex_map[next].territory_distance = next_dist;
            border_frontier.push(next);
        }
    }
}
void UnitToCounterAppend(HexState& hex_state) {
    const HexTuning& tuning = Singleton::Get<HexTuning>();
    for (const auto& [axial_unit, unit_handles] : hex_state.units_by_axial) {
        CounterStack& counter = hex_state.counters.Get();
        counter.axial = axial_unit;
        Echelon echelon { Echelon::ECHELON_SQUAD };
        UnitIcon icon { UnitIcon::ICON_INF };
        u32 dmg = 0;
        u32 def = 0;
        u32 move = 0;
        counter.stack = { };
        const u32 counters_on_hex = math::Min<u32>(counter.stack.size(), unit_handles.size());
        if (hex_state.pseudo_states.unit_selection.has_value() && hex_state.pseudo_states.axial_select == axial_unit) {
            dmg = hex_state.pseudo_states.unit_selection->dmg_sum;
            def = hex_state.pseudo_states.unit_selection->def_sum;
            move = hex_state.pseudo_states.unit_selection->move_min;
            // drawing selected
            u32 i = 0;
            for (; i < hex_state.pseudo_states.unit_selection->unit_handles.size(); i++) {
                const Handle<Unit> unit_handle_selected = hex_state.pseudo_states.unit_selection->unit_handles[i];
                const Unit& unit_selected = hex_state.units[unit_handle_selected];
                counter.stack[i] = Counter { .color_background = CountryColorRuntime(tuning, unit_selected.tag), .color_icon = unit_selected.color, .color_border = colors::HEX_SELECT };
                if (unit_selected.echelon >= echelon) {
                    echelon = unit_selected.echelon;
                    icon = unit_selected.icon;
                }
            }

            // only color for rest
            for (u32 j = 0; j < counters_on_hex; j++) {
                const Handle<Unit> unit_handle = unit_handles[j];
                if (hex_state.pseudo_states.unit_selection->unit_handles.Contains(unit_handle)) { continue; }
                const Unit& unit = hex_state.units[unit_handle];
                counter.stack[i++] = Counter { .color_background = CountryColorRuntime(tuning, unit.tag), .color_icon = unit.color, .color_border = colors::YELLOW };
            }
        } else {
            // drawing plain
            for (u32 i = 0; i < counters_on_hex; i++) {
                const Handle<Unit> unit_handle = unit_handles[i];
                const Unit& unit = hex_state.units[unit_handle];
                counter.stack[i] = Counter { .color_background = CountryColorRuntime(tuning, unit.tag), .color_icon = unit.color, .color_border = colors::BLACK };
                dmg += unit.dmg;
                def += unit.def;
                move = math::Max(unit.move, move);
                if (unit.echelon >= echelon) {
                    echelon = unit.echelon;
                    icon = unit.icon;
                }
            }
        }
        counter.label_top.SetText(EchelonToString(echelon));
        counter.label_center.SetText(UnitIconToString(icon));
        counter.label_bottom.SetText(std::format("{}-{}-{}", dmg, def, move));
    }
}
PlayerAction GetPlayerAction(const HexState& hex_state) {
    const InputState& input_state = Singleton::Get<InputState>();

    if (hex_state.pseudo_states.axial_hover.has_value()) {
        // unit selection
        if (input_state.left_mouse_down) { return hex_state.units_by_axial.contains(hex_state.pseudo_states.axial_hover.value()) ? PlayerAction::PLAYER_ACTION_SELECT : PlayerAction::PLAYER_ACTION_DESELECT; }
        if (hex_state.pseudo_states.unit_selection.has_value()) {
            if (AxialIsEnemy(hex_state, hex_state.pseudo_states.axial_hover.value())) { return input_state.right_mouse_down ? PlayerAction::PLAYER_ACTION_ATTACK_CLICK : PlayerAction::PLAYER_ACTION_ATTACK_HOVER; }
            if (hex_state.pseudo_states.axial_hover != hex_state.pseudo_states.axial_select) { return input_state.right_mouse_down ? PlayerAction::PLAYER_ACTION_MOVE_CLICK : PlayerAction::PLAYER_ACTION_MOVE_HOVER; }
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

        if (hex_state.pseudo_states.unit_selection.has_value()) {
            const auto unit_selection = hex_state.pseudo_states.unit_selection->unit_handles | hex_state.units.handle_to_view();
            hex_state.pseudo_states.unit_selection->def_sum = std::ranges::fold_left(unit_selection | std::views::transform(&Unit::def), u32 { 0 }, std::plus { });
            hex_state.pseudo_states.unit_selection->dmg_sum = std::ranges::fold_left(unit_selection | std::views::transform(&Unit::dmg), u32 { 0 }, std::plus { });
            const auto [move_min, move_max] = std::ranges::minmax(unit_selection | std::views::transform(&Unit::move), std::less { });
            hex_state.pseudo_states.unit_selection->move_min = move_min;
            hex_state.pseudo_states.unit_selection->move_max = move_max;
        }

        // pseudo states set
        const float2 mouse_world = camera.ScreenToWorld(input_state.mouse_position);
        if (hex_state.pseudo_states.unit_selection.has_value()) { hex_state.pseudo_states.axial_select = hex_state.units[hex_state.pseudo_states.unit_selection->unit_handles[0]].axial; } // update selection
        hex_state.pseudo_states.axial_hover = hex_state.hex_map.Contains(HexWorldToAxial(mouse_world)) ? Optional { HexWorldToAxial(mouse_world) } : std::nullopt;
        hex_state.player_tag = hex_state.pseudo_states.unit_selection.has_value() ? hex_state.units[hex_state.pseudo_states.unit_selection->unit_handles[0]].tag : CountryTag::TAG_GER;
        hex_state.player_action = PlayerAction::PLAYER_ACTION_NONE;

        // render pseudo states
        if (hex_state.pseudo_states.axial_hover) { HexAppend(hex_state.verts, camera.scale, camera.WorldToScreen(HexAxialToWorld(hex_state.pseudo_states.axial_hover.value())), colors::HEX_HOVER); }
        if (hex_state.pseudo_states.axial_select) { HexAppend(hex_state.verts, camera.scale * 0.95F, camera.WorldToScreen(HexAxialToWorld(hex_state.pseudo_states.axial_select.value())), colors::HEX_SELECT); }

        // render map. Geometry/color knobs are runtime-tunable via the imgui
        // "Hex tuning" window (see d_imgui_system.hpp).
        HexTuning& tuning = Singleton::Get<HexTuning>();
        if (tuning.scheme_dirty) {
            hex_state.hex_draw = HexToHexDrawInfo(hex_state.hex_map, tuning.terrain_scheme);
            tuning.scheme_dirty = false;
        }
        Singleton::Get<WindowState>().clear_color = tuning.background;
        const f32 hex_inner = HexInnerScaleRuntime(tuning);
        const f32 hex_fill  = hex_inner * tuning.hex_fill_scale;
        if (tuning.hex_style == HexStyle::DOUBLE_RING) {
            for (const HexDrawInfo& hex : hex_state.hex_draw.data) { HexAppend(hex_state.verts, camera.scale * tuning.hex_outer_double, camera.WorldToScreen(hex.world), colors::ColorMul(static_cast<ColorF>(hex.color), tuning.double_ring_dark)); }
        }
        for (const HexDrawInfo& hex : hex_state.hex_draw.data) { HexAppend(hex_state.verts, camera.scale * hex_fill, camera.WorldToScreen(hex.world), hex.color); }
        (void)SDL_RenderGeometry(window_state.renderer, nullptr, hex_state.verts.data.data(), static_cast<i32>(hex_state.verts.size()), nullptr, 0);
        hex_state.verts.clear();
        // render per-terrain decorations (trees, mountains, waves, ...) so the
        // map reads visually without needing real sprites yet.
        if (tuning.show_decorations) {
            (void)SDL_SetRenderDrawBlendMode(window_state.renderer, SDL_BLENDMODE_BLEND);
            for (const HexDrawInfo& hex : hex_state.hex_draw.data) {
                TerrainDecorationAppend(hex_state.verts, hex.terrain, camera.WorldToScreen(hex.world), camera.scale * hex_fill, tuning.decoration_alpha);
            }
            (void)SDL_RenderGeometry(window_state.renderer, nullptr, hex_state.verts.data.data(), static_cast<i32>(hex_state.verts.size()), nullptr, 0);
            hex_state.verts.clear();
            (void)SDL_SetRenderDrawBlendMode(window_state.renderer, SDL_BLENDMODE_NONE);
        }

        // map features (cities / villages / roads / rivers). All driven by a
        // deterministic hash from axial coordinates so the same hex always
        // hosts the same thing. Render order: rivers (under), roads (on top
        // of rivers but under settlements), villages, then cities.
        {
            (void)SDL_SetRenderDrawBlendMode(window_state.renderer, SDL_BLENDMODE_BLEND);
            const f32 s = camera.scale * hex_fill;
            // RIVERS - along edges between two land hexes (hash-selected).
            if (tuning.show_rivers) {
                const ColorF river { 0.30F, 0.55F, 0.85F, 0.85F };
                for (u32 i = 0; i < hex_state.hex_map.Size(); i++) {
                    const Hex& hex = hex_state.hex_map.data[i];
                    if (!TerrainIsLand(hex.terrain)) { continue; }
                    const int2 axial = hex_state.hex_map.IndexToAxial(i);
                    const float2 c { static_cast<f32>(camera.WorldToScreen(HexAxialToWorld(axial)).x), static_cast<f32>(camera.WorldToScreen(HexAxialToWorld(axial)).y) };
                    for (u32 ni = 0; ni < HEX_CORNERS; ni++) {
                        const int2 nax = axial + HEX_AXIAL_NEIGHBOURS[ni];
                        if (!hex_state.hex_map.Contains(nax)) { continue; }
                        if (nax.x < axial.x || (nax.x == axial.x && nax.y < axial.y)) { continue; } // emit once per pair
                        if (!TerrainIsLand(hex_state.hex_map[nax].terrain)) { continue; }
                        if ((EdgeHash(axial, nax) & 0x1FU) != 1U) { continue; }
                        const u32 side = (HEX_CORNERS - ni) % HEX_CORNERS;
                        const u32 j    = (side + 1) % HEX_CORNERS;
                        const float2 a = c + HEX_ANGLE[side] * float2 { s, s };
                        const float2 b = c + HEX_ANGLE[j]    * float2 { s, s };
                        SegmentAppend(hex_state.verts, a, b, camera.scale * 0.05F, river);
                    }
                }
            }
            // ROADS - between hex centers across shared edge (both buildable).
            if (tuning.show_roads) {
                const ColorF road { 0.35F, 0.28F, 0.20F, 0.95F };
                for (u32 i = 0; i < hex_state.hex_map.Size(); i++) {
                    const Hex& hex = hex_state.hex_map.data[i];
                    if (!TerrainBuildable(hex.terrain)) { continue; }
                    const int2 axial = hex_state.hex_map.IndexToAxial(i);
                    const int2 ws0   = camera.WorldToScreen(HexAxialToWorld(axial));
                    const float2 c0 { static_cast<f32>(ws0.x), static_cast<f32>(ws0.y) };
                    for (u32 ni = 0; ni < HEX_CORNERS; ni++) {
                        const int2 nax = axial + HEX_AXIAL_NEIGHBOURS[ni];
                        if (!hex_state.hex_map.Contains(nax)) { continue; }
                        if (nax.x < axial.x || (nax.x == axial.x && nax.y < axial.y)) { continue; }
                        if (!TerrainBuildable(hex_state.hex_map[nax].terrain)) { continue; }
                        if ((EdgeHash(axial, nax) & 0x07U) != 0U) { continue; }
                        const int2 ws1 = camera.WorldToScreen(HexAxialToWorld(nax));
                        const float2 c1 { static_cast<f32>(ws1.x), static_cast<f32>(ws1.y) };
                        SegmentAppend(hex_state.verts, c0, c1, camera.scale * 0.035F, road);
                    }
                }
            }
            // VILLAGES + CITIES - settlements on buildable terrain.
            if (tuning.show_villages || tuning.show_cities) {
                const ColorF wall   { 0.18F, 0.16F, 0.14F, 1.0F };
                const ColorF roof   { 0.55F, 0.20F, 0.18F, 1.0F };
                const ColorF roof_h { 0.75F, 0.55F, 0.35F, 1.0F };
                auto rect = [&](float2 cc, f32 w, f32 h, ColorF col) {
                    const f32 x0 = cc.x - w; const f32 x1 = cc.x + w;
                    const f32 y0 = cc.y - h; const f32 y1 = cc.y + h;
                    hex_state.verts.EmplaceBack(SDL_FPoint { x0, y0 }, col, SDL_FPoint { });
                    hex_state.verts.EmplaceBack(SDL_FPoint { x1, y0 }, col, SDL_FPoint { });
                    hex_state.verts.EmplaceBack(SDL_FPoint { x0, y1 }, col, SDL_FPoint { });
                    hex_state.verts.EmplaceBack(SDL_FPoint { x1, y0 }, col, SDL_FPoint { });
                    hex_state.verts.EmplaceBack(SDL_FPoint { x1, y1 }, col, SDL_FPoint { });
                    hex_state.verts.EmplaceBack(SDL_FPoint { x0, y1 }, col, SDL_FPoint { });
                };
                for (u32 i = 0; i < hex_state.hex_map.Size(); i++) {
                    const Hex& hex = hex_state.hex_map.data[i];
                    if (!TerrainBuildable(hex.terrain)) { continue; }
                    const int2 axial = hex_state.hex_map.IndexToAxial(i);
                    const u32 h = AxialHash(axial);
                    const int2 ws = camera.WorldToScreen(HexAxialToWorld(axial));
                    const float2 c { static_cast<f32>(ws.x), static_cast<f32>(ws.y) };
                    if (tuning.show_cities && (h & 0x1FU) == 0U) {
                        // 3x3 cluster of buildings + a contrasting roof on the centre.
                        const f32 b = s * 0.08F;
                        for (i32 ox = -1; ox <= 1; ox++) {
                            for (i32 oy = -1; oy <= 1; oy++) {
                                const float2 cc { c.x + static_cast<f32>(ox) * s * 0.18F, c.y + static_cast<f32>(oy) * s * 0.18F };
                                rect(cc, b, b, (ox == 0 && oy == 0) ? roof_h : wall);
                            }
                        }
                    } else if (tuning.show_villages && (h & 0x07U) == 0U) {
                        // single small house glyph: walls + smaller roof on top
                        rect({ c.x - s * 0.05F, c.y + s * 0.02F }, s * 0.06F, s * 0.05F, wall);
                        rect({ c.x + s * 0.06F, c.y - s * 0.04F }, s * 0.05F, s * 0.04F, roof);
                    }
                }
            }
            (void)SDL_RenderGeometry(window_state.renderer, nullptr, hex_state.verts.data.data(), static_cast<i32>(hex_state.verts.size()), nullptr, 0);
            hex_state.verts.clear();
            (void)SDL_SetRenderDrawBlendMode(window_state.renderer, SDL_BLENDMODE_NONE);
        }
        // render territory overlay (runtime style, see imgui)
        if (tuning.territory_style == TerritoryStyle::OVERLAY) {
            (void)SDL_SetRenderDrawBlendMode(window_state.renderer, SDL_BLENDMODE_BLEND);
            for (u32 i = 0; i < hex_state.hex_map.Size(); i++) {
                const Hex& hex = hex_state.hex_map.data[i];
                if (hex.country_tag == CountryTag::TAG_NONE) { continue; }
                const int2 axial = hex_state.hex_map.IndexToAxial(i);
                const int2 world_pos = camera.WorldToScreen(HexAxialToWorld(axial));
                const Color base = CountryColorRuntime(tuning, hex.country_tag);
                HexAppend(hex_state.verts, camera.scale * hex_fill, world_pos, base.WithAlpha(tuning.territory_alpha), base.WithAlpha(tuning.territory_alpha_center));
            }
            (void)SDL_RenderGeometry(window_state.renderer, nullptr, hex_state.verts.data.data(), static_cast<i32>(hex_state.verts.size()), nullptr, 0);
            hex_state.verts.clear();
            (void)SDL_SetRenderDrawBlendMode(window_state.renderer, SDL_BLENDMODE_NONE);
        } else if (tuning.territory_style == TerritoryStyle::BORDER) {
            // Civ6-style: emit a continuous mitered strip around each
            // country's border. We collect border edges in WORLD space so
            // adjacent hexes share corner coordinates exactly, then look up
            // each edge's neighbour at each endpoint and offset along the
            // angle bisector. The result is one snake, not per-hex
            // trapezoids — corners join cleanly across same-country hexes.
            (void)SDL_SetRenderDrawBlendMode(window_state.renderer, SDL_BLENDMODE_BLEND);
            const f32 outset_w = tuning.border_outset; // world units (1 unit = 1 hex inradius-ish)
            const f32 depth_w  = tuning.border_depth;
            f32 fade_t = 0.0F;
            if (tuning.border_fade_overlay && tuning.fade_scale_max > tuning.fade_scale_min) {
                fade_t = (tuning.fade_scale_max - camera.scale) / (tuning.fade_scale_max - tuning.fade_scale_min);
                if (fade_t < 0.0F) { fade_t = 0.0F; }
                if (fade_t > 1.0F) { fade_t = 1.0F; }
            }
            const f32 border_a_out = tuning.border_alpha * (1.0F - fade_t);
            const f32 border_a_in  = border_a_out * tuning.border_fade;
            const f32 gamma  = tuning.border_fade_curve > 0.0F ? tuning.border_fade_curve : 1.0F;
            const f32 t_mid  = std::pow(0.5F, gamma);
            const f32 border_a_mid = border_a_out + (border_a_in - border_a_out) * t_mid;
            if (border_a_out > 0.001F) {
                // ----- collect edges per country in world space -----
                struct BEdge { float2 w0, w1; float2 inward; i32 partner0 { -1 }; i32 partner1 { -1 }; };
                std::array<std::vector<BEdge>, 4> per_country { }; // indexed by CountryTag
                for (u32 i = 0; i < hex_state.hex_map.Size(); i++) {
                    const Hex& hex = hex_state.hex_map.data[i];
                    if (hex.country_tag == CountryTag::TAG_NONE) { continue; }
                    const int2 axial = hex_state.hex_map.IndexToAxial(i);
                    const float2 center = HexAxialToWorld(axial);
                    for (u32 s = 0; s < HEX_CORNERS; s++) {
                        const int2 nax = axial + HEX_AXIAL_NEIGHBOURS[s];
                        const CountryTag ntag = hex_state.hex_map.Contains(nax) ? hex_state.hex_map[nax].country_tag : CountryTag::TAG_NONE;
                        if (ntag == hex.country_tag) { continue; }
                        const u32 side = (HEX_CORNERS - s) % HEX_CORNERS;
                        const u32 j    = (side + 1) % HEX_CORNERS;
                        // corners live on the LATTICE (radius=1) so neighbouring
                        // hexes share them exactly. hex_inner only affects the
                        // visible fill, not the country boundary.
                        const float2 p0 = center + HEX_ANGLE[side];
                        const float2 p1 = center + HEX_ANGLE[j];
                        const float2 emid { (p0.x + p1.x) * 0.5F, (p0.y + p1.y) * 0.5F };
                        float2 inward { center.x - emid.x, center.y - emid.y };
                        const f32 ilen = std::sqrt(inward.x * inward.x + inward.y * inward.y);
                        if (ilen > 0.0001F) { inward.x /= ilen; inward.y /= ilen; }
                        per_country[static_cast<u32>(hex.country_tag)].push_back(BEdge { p0, p1, inward });
                    }
                }
                // ----- link partner edges via spatial hash on shared corners -----
                auto snap_key = [](float2 p) -> u64 {
                    const i32 qx = static_cast<i32>(std::round(p.x * 4096.0F));
                    const i32 qy = static_cast<i32>(std::round(p.y * 4096.0F));
                    return (static_cast<u64>(static_cast<u32>(qx)) << 32) | static_cast<u64>(static_cast<u32>(qy));
                };
                for (u32 tag_i = 0; tag_i < per_country.size(); tag_i++) {
                    auto& edges = per_country[tag_i];
                    if (edges.empty()) { continue; }
                    std::unordered_map<u64, std::array<i32, 4>> bucket;
                    for (i32 ei = 0; ei < static_cast<i32>(edges.size()); ei++) {
                        for (const float2 cp : { edges[ei].w0, edges[ei].w1 }) {
                            auto& slots = bucket.try_emplace(snap_key(cp), std::array<i32, 4> { -1, -1, -1, -1 }).first->second;
                            for (i32& s : slots) { if (s == -1) { s = ei; break; } }
                        }
                    }
                    auto find_partner = [&](float2 corner, i32 self) -> i32 {
                        const auto it = bucket.find(snap_key(corner));
                        if (it == bucket.end()) { return -1; }
                        for (const i32 other : it->second) {
                            if (other == -1)   { break; }
                            if (other != self) { return other; }
                        }
                        return -1;
                    };
                    for (i32 ei = 0; ei < static_cast<i32>(edges.size()); ei++) {
                        edges[ei].partner0 = find_partner(edges[ei].w0, ei);
                        edges[ei].partner1 = find_partner(edges[ei].w1, ei);
                    }
                    // ----- emit mitered strip -----
                    const Color  base    = CountryColorRuntime(tuning, static_cast<CountryTag>(tag_i));
                    const ColorF col_out = static_cast<ColorF>(base.WithAlpha(border_a_out));
                    const ColorF col_mid = static_cast<ColorF>(base.WithAlpha(border_a_mid));
                    const ColorF col_in  = static_cast<ColorF>(base.WithAlpha(border_a_in));
                    auto miter_dir = [&](float2 my_inward, i32 partner) -> float2 {
                        // returns a vector m such that displacing the corner by
                        // m*offset moves it `offset` perpendicular to BOTH edges
                        // (so two strips meet without overlap or gap).
                        if (partner < 0) { return my_inward; }
                        const float2 other = edges[partner].inward;
                        float2 m { my_inward.x + other.x, my_inward.y + other.y };
                        const f32 mlen = std::sqrt(m.x * m.x + m.y * m.y);
                        if (mlen < 0.0001F) { return my_inward; }
                        m.x /= mlen; m.y /= mlen;
                        const f32 d = m.x * my_inward.x + m.y * my_inward.y;
                        // clamp miter length so very sharp corners don't spike.
                        const f32 inv = d > 0.25F ? 1.0F / d : 4.0F;
                        return float2 { m.x * inv, m.y * inv };
                    };
                    auto w_to_s = [&](float2 w) -> float2 {
                        return float2 { w.x * camera.scale - camera.world_position.x,
                                        w.y * camera.scale - camera.world_position.y };
                    };
                    for (const BEdge& e : edges) {
                        const float2 m0 = miter_dir(e.inward, e.partner0);
                        const float2 m1 = miter_dir(e.inward, e.partner1);
                        const float2 a_out_w { e.w0.x - m0.x * outset_w, e.w0.y - m0.y * outset_w };
                        const float2 b_out_w { e.w1.x - m1.x * outset_w, e.w1.y - m1.y * outset_w };
                        const float2 a_in_w  { e.w0.x + m0.x * depth_w,  e.w0.y + m0.y * depth_w  };
                        const float2 b_in_w  { e.w1.x + m1.x * depth_w,  e.w1.y + m1.y * depth_w  };
                        const float2 a_mid_w { (a_out_w.x + a_in_w.x) * 0.5F, (a_out_w.y + a_in_w.y) * 0.5F };
                        const float2 b_mid_w { (b_out_w.x + b_in_w.x) * 0.5F, (b_out_w.y + b_in_w.y) * 0.5F };
                        const float2 ao = w_to_s(a_out_w);
                        const float2 bo = w_to_s(b_out_w);
                        const float2 am = w_to_s(a_mid_w);
                        const float2 bm = w_to_s(b_mid_w);
                        const float2 ai = w_to_s(a_in_w);
                        const float2 bi = w_to_s(b_in_w);
                        // outer trapezoid (outer -> mid).
                        hex_state.verts.EmplaceBack(SDL_FPoint { ao.x, ao.y }, col_out, SDL_FPoint { });
                        hex_state.verts.EmplaceBack(SDL_FPoint { bo.x, bo.y }, col_out, SDL_FPoint { });
                        hex_state.verts.EmplaceBack(SDL_FPoint { bm.x, bm.y }, col_mid, SDL_FPoint { });
                        hex_state.verts.EmplaceBack(SDL_FPoint { ao.x, ao.y }, col_out, SDL_FPoint { });
                        hex_state.verts.EmplaceBack(SDL_FPoint { bm.x, bm.y }, col_mid, SDL_FPoint { });
                        hex_state.verts.EmplaceBack(SDL_FPoint { am.x, am.y }, col_mid, SDL_FPoint { });
                        // inner trapezoid (mid -> inner).
                        hex_state.verts.EmplaceBack(SDL_FPoint { am.x, am.y }, col_mid, SDL_FPoint { });
                        hex_state.verts.EmplaceBack(SDL_FPoint { bm.x, bm.y }, col_mid, SDL_FPoint { });
                        hex_state.verts.EmplaceBack(SDL_FPoint { bi.x, bi.y }, col_in,  SDL_FPoint { });
                        hex_state.verts.EmplaceBack(SDL_FPoint { am.x, am.y }, col_mid, SDL_FPoint { });
                        hex_state.verts.EmplaceBack(SDL_FPoint { bi.x, bi.y }, col_in,  SDL_FPoint { });
                        hex_state.verts.EmplaceBack(SDL_FPoint { ai.x, ai.y }, col_in,  SDL_FPoint { });
                    }
                }
                (void)SDL_RenderGeometry(window_state.renderer, nullptr, hex_state.verts.data.data(), static_cast<i32>(hex_state.verts.size()), nullptr, 0);
                hex_state.verts.clear();
            }
            // zoomed-out overlay pass: rises from 0 -> territory_alpha as we
            // zoom out, so the country becomes a solid coloured blob.
            if (fade_t > 0.0F) {
                const f32 overlay_a = tuning.territory_alpha * fade_t;
                for (u32 i = 0; i < hex_state.hex_map.Size(); i++) {
                    const Hex& hex = hex_state.hex_map.data[i];
                    if (hex.country_tag == CountryTag::TAG_NONE) { continue; }
                    const int2 axial = hex_state.hex_map.IndexToAxial(i);
                    const int2 ws = camera.WorldToScreen(HexAxialToWorld(axial));
                    const Color base = CountryColorRuntime(tuning, hex.country_tag);
                    HexAppend(hex_state.verts, camera.scale * hex_inner, ws, base.WithAlpha(overlay_a), base.WithAlpha(0.0F));
                }
                (void)SDL_RenderGeometry(window_state.renderer, nullptr, hex_state.verts.data.data(), static_cast<i32>(hex_state.verts.size()), nullptr, 0);
                hex_state.verts.clear();
            }
            (void)SDL_SetRenderDrawBlendMode(window_state.renderer, SDL_BLENDMODE_NONE);
        }

        // fog of war: tiles farther than `fog_radius` axial steps from any
        // player-owned hex get a dark overlay. The ring exactly at radius+1
        // is half-strength so the edge of vision softens. BFS is cheap; just
        // run it every frame for now.
        if (tuning.fog_alpha > 0.001F && hex_state.player_tag != CountryTag::TAG_NONE) {
            (void)SDL_SetRenderDrawBlendMode(window_state.renderer, SDL_BLENDMODE_BLEND);
            std::unordered_map<int2, i32> fog_dist;
            std::queue<int2> q;
            for (u32 i = 0; i < hex_state.hex_map.Size(); i++) {
                const Hex& hex = hex_state.hex_map.data[i];
                if (hex.country_tag != hex_state.player_tag) { continue; }
                const int2 axial = hex_state.hex_map.IndexToAxial(i);
                fog_dist[axial] = 0;
                q.push(axial);
            }
            while (!q.empty()) {
                const int2 cur = q.front(); q.pop();
                const i32 d = fog_dist[cur];
                for (const int2 off : HEX_AXIAL_NEIGHBOURS) {
                    const int2 nx = cur + off;
                    if (!hex_state.hex_map.Contains(nx))       { continue; }
                    if (fog_dist.contains(nx))                 { continue; }
                    fog_dist[nx] = d + 1;
                    q.push(nx);
                }
            }
            const Color fog_dark { 0U, 0U, 0U, 255U };
            for (u32 i = 0; i < hex_state.hex_map.Size(); i++) {
                const Hex& hex = hex_state.hex_map.data[i];
                if (hex.country_tag == hex_state.player_tag) { continue; }
                const int2 axial = hex_state.hex_map.IndexToAxial(i);
                const auto it = fog_dist.find(axial);
                const i32 d = it == fog_dist.end() ? INT32_MAX : it->second;
                f32 a = 0.0F;
                if      (d <= tuning.fog_radius)     { a = 0.0F; }
                else if (d == tuning.fog_radius + 1) { a = tuning.fog_alpha * 0.5F; }
                else                                  { a = tuning.fog_alpha; }
                if (a <= 0.001F) { continue; }
                const int2 ws = camera.WorldToScreen(HexAxialToWorld(axial));
                HexAppend(hex_state.verts, camera.scale * hex_inner, ws, fog_dark.WithAlpha(a));
            }
            (void)SDL_RenderGeometry(window_state.renderer, nullptr, hex_state.verts.data.data(), static_cast<i32>(hex_state.verts.size()), nullptr, 0);
            hex_state.verts.clear();
            (void)SDL_SetRenderDrawBlendMode(window_state.renderer, SDL_BLENDMODE_NONE);
        }

        // render units
        hex_state.counters.Clear();
        UnitToCounterAppend(hex_state);
        RenderCounters(hex_state.counters);

        // logic and render
        hex_state.player_action = GetPlayerAction(hex_state);

        switch (hex_state.player_action) {
            case PlayerAction::PLAYER_ACTION_NONE: break;
            case PlayerAction::PLAYER_ACTION_SELECT: {
                const b8 select_new = hex_state.pseudo_states.axial_select != hex_state.pseudo_states.axial_hover;
                hex_state.pseudo_states.axial_select = hex_state.pseudo_states.axial_hover;
                const List<Handle<Unit>>& unit_handles = hex_state.units_by_axial[hex_state.pseudo_states.axial_hover.value()];
                List<Handle<Unit>> unit_handles_selection;
                if (select_new) {
                    unit_handles_selection = List { { unit_handles[0] } };
                } else {
                    const u32 i = unit_handles.IndexOf(hex_state.pseudo_states.unit_selection->unit_handles[0]);
                    const u32 next = i + 1; // select next or all
                    unit_handles_selection = next == unit_handles.size() ? unit_handles : List { { unit_handles[next] } };
                }
                hex_state.pseudo_states.unit_selection = UnitGroup { .unit_handles = unit_handles_selection };
                break;
            }
            case PlayerAction::PLAYER_ACTION_DESELECT: {
                hex_state.pseudo_states.axial_select = std::nullopt;
                hex_state.pseudo_states.unit_selection = std::nullopt;
                break;
            }
            case PlayerAction::PLAYER_ACTION_MOVE_CLICK: {
                // movement https://www.redblobgames.com/grids/hexagons/#distances
                const int2 axial_start = hex_state.pseudo_states.axial_select.value();
                const int2 axial_hover = hex_state.pseudo_states.axial_hover.value();
                auto units_selected = hex_state.pseudo_states.unit_selection->unit_handles | hex_state.units.handle_to_view();

                constexpr Color COLOR { colors::RUBY_RED };
                (void)SDL_SetRenderDrawColor(window_state.renderer, COLOR.r, COLOR.g, COLOR.b, COLOR.a);

                // path finding
                List<AxialAndCost> axial_path = HexAxialPathAStar(hex_state, axial_start, axial_hover);
                const auto it = std::ranges::upper_bound(axial_path, hex_state.pseudo_states.unit_selection->move_min, std::less { }, &AxialAndCost::cost);
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
                const List<AxialAndCost> axial_path = HexAxialPathAStar(hex_state, hex_state.pseudo_states.axial_select.value(), hex_state.pseudo_states.axial_hover.value());

                auto units_selected = hex_state.pseudo_states.unit_selection->unit_handles | hex_state.units.handle_to_view();
                const u32 units_selected_movement = std::ranges::min(units_selected | std::views::transform(&Unit::move));
                for (const AxialAndCost cost_and_axial : axial_path) {
                    const float2 world = HexAxialToWorld(cost_and_axial.axial);
                    const int2 screen = camera.WorldToScreen(world);

                    const Color movement_color = cost_and_axial.cost > units_selected_movement ? colors::BLACK : colors::RUBY_RED;
                    HexAppend(hex_state.verts, camera.scale * 0.25F, screen, movement_color);
                }
                (void)SDL_RenderGeometry(window_state.renderer, nullptr, hex_state.verts.data.data(), static_cast<i32>(hex_state.verts.size()), nullptr, 0);
                hex_state.verts.clear();

                const f32 pt = camera.scale * 0.3F;
                const Font& font_movement = font_collection.GetFontBold(static_cast<FontSizes>(pt));
                TTF_SetFontWrapAlignment(font_movement, TTF_HORIZONTAL_ALIGN_CENTER);
                for (AxialAndCost cost_and_axial : axial_path) {
                    const float2 world = HexAxialToWorld(cost_and_axial.axial);
                    const int2 screen = camera.WorldToScreen(world);
                    const float2 screen_f = static_cast<float2>(screen);

                    const Label& label = hex_state.label_pool.Get();
                    const String string_distance = std::format("{}", cost_and_axial.cost);
                    (void)TTF_SetTextWrapWidth(label, static_cast<i32>(camera.scale));
                    (void)TTF_SetTextFont(label, font_movement);
                    (void)TTF_SetTextString(label, string_distance.c_str(), string_distance.size());
                    (void)TTF_DrawRendererText(label, screen_f.x - camera.scale * 0.5F, screen_f.y - pt * 0.5F);
                }
                break;
            }
            case PlayerAction::PLAYER_ACTION_ATTACK_CLICK: {
                const b8 within_range = std::ranges::all_of(hex_state.pseudo_states.unit_selection->unit_handles | hex_state.units.handle_to_view(), [&](const Unit& unit) -> b8 { return HexAxialDistance(unit.axial, hex_state.pseudo_states.axial_hover.value()) == 1; });
                const b8 attacker_is_hq = std::ranges::contains(hex_state.pseudo_states.unit_selection->unit_handles | hex_state.units.handle_to_view(), UnitIcon::ICON_HQ, &Unit::icon);
                const b8 can_attack = hex_state.pseudo_states.unit_selection->move_min > MOVE_COST_ATTACK && within_range && !attacker_is_hq;
                if (!can_attack) { break; }

                auto units_attacker = hex_state.pseudo_states.unit_selection->unit_handles | hex_state.units.handle_to_view();
                auto units_defender = hex_state.units_by_axial[hex_state.pseudo_states.axial_hover.value()] | hex_state.units.handle_to_view();
                // attack
                const u32 dmg = hex_state.pseudo_states.unit_selection->dmg_sum;
                const u32 def = std::ranges::fold_left(units_defender | std::views::transform(&Unit::def), 0U, std::plus { });
                const f32 ratio = static_cast<f32>(dmg) / static_cast<f32>(def);
                enum class PostBattleOutcome { DEFENDER_COUNTER_ATTACKED, DEFENDER_SCOUTED, DEFENDER_HELD, DEFENDER_RETREAT, DEFENDER_ROUT, DEFENDER_SURRENDER };
                struct BattleOutcome {
                    PostBattleOutcome battle_outcome;
                    u32 dmg_attacker;
                    u32 dmg_defender;
                } battle_outcome;
                if (ratio >= 2.0F) {
                    battle_outcome = BattleOutcome { .battle_outcome = PostBattleOutcome::DEFENDER_RETREAT, .dmg_attacker = 1U, .dmg_defender = 3U };
                } else if (ratio >= 1.5F ) {
                    battle_outcome = BattleOutcome { .battle_outcome = PostBattleOutcome::DEFENDER_HELD, .dmg_attacker = 2U, .dmg_defender = 1U };
                }
                else if (ratio <= 0.3F) {
                    battle_outcome = BattleOutcome { .battle_outcome = PostBattleOutcome::DEFENDER_COUNTER_ATTACKED, .dmg_attacker = 3U, .dmg_defender = 1U };
                }
                else {
                    battle_outcome = BattleOutcome {.battle_outcome = PostBattleOutcome::DEFENDER_SCOUTED, .dmg_attacker = 2U, .dmg_defender = 1U };
                }
                for (Unit& attacker : units_attacker) {
                    attacker.dmg = math::SaturatingSub(attacker.dmg, battle_outcome.dmg_attacker);
                    attacker.def = math::SaturatingSub(attacker.def, battle_outcome.dmg_attacker / 2);
                    attacker.move = math::SaturatingSub(attacker.move, MOVE_COST_ATTACK);
                }
                for (Unit& defender : units_defender) {
                    defender.dmg = math::SaturatingSub(defender.dmg, battle_outcome.dmg_defender);
                    defender.def = math::SaturatingSub(defender.def, battle_outcome.dmg_defender / 2);
                }
                switch (battle_outcome.battle_outcome) {
                    case PostBattleOutcome::DEFENDER_COUNTER_ATTACKED:
                        for (Unit& attacker : units_attacker) {
                            const int2 axial_retreat = attacker.tag == CountryTag::TAG_GER ? int2 {-2, 0} : int2 {2, 0};
                            attacker.axial += axial_retreat;
                        }
                        break;
                    case PostBattleOutcome::DEFENDER_SCOUTED:
                    case PostBattleOutcome::DEFENDER_HELD: break;
                    case PostBattleOutcome::DEFENDER_RETREAT:
                        for (Unit& defender : units_defender) {
                            const int2 axial_retreat = defender.tag == CountryTag::TAG_GER ? int2 {-2, 0} : int2 {2, 0};
                            defender.axial += axial_retreat;
                        }
                        break;
                    case PostBattleOutcome::DEFENDER_ROUT:
                        for (Unit& defender : units_defender) {
                            const int2 axial_retreat = defender.tag == CountryTag::TAG_GER ? int2 {-3, 0} : int2 {3, 0};
                            defender.axial += axial_retreat;
                        }
                        break;
                    case PostBattleOutcome::DEFENDER_SURRENDER:
                        // unit breaks
                        break;
                }

                break;
            }
            case PlayerAction::PLAYER_ACTION_ATTACK_HOVER:
                const b8 within_range = std::ranges::all_of(hex_state.pseudo_states.unit_selection->unit_handles | hex_state.units.handle_to_view(), [&](const Unit& unit) -> b8 { return HexAxialDistance(unit.axial, hex_state.pseudo_states.axial_hover.value()) == 1; });
                const b8 attacker_is_hq = std::ranges::contains(hex_state.pseudo_states.unit_selection->unit_handles | hex_state.units.handle_to_view(), UnitIcon::ICON_HQ, &Unit::icon);
                const b8 can_attack = hex_state.pseudo_states.unit_selection->move_min > MOVE_COST_ATTACK && within_range && !attacker_is_hq;

                const float2 world = HexAxialToWorld(hex_state.pseudo_states.axial_hover.value());
                const int2 screen = camera.WorldToScreen(world);

                Color color_inner = can_attack ? colors::RUBY_RED : colors::BLACK;
                HexAppend(hex_state.verts, camera.scale * 0.25F, screen, colors::GRAY);
                HexAppend(hex_state.verts, camera.scale * 0.20F, screen, color_inner);
                if (can_attack) { HexAppend(hex_state.verts, camera.scale * 0.10F, screen, colors::DARK_GREEN); }
                (void)SDL_RenderGeometry(window_state.renderer, nullptr, hex_state.verts.data.data(), static_cast<i32>(hex_state.verts.size()), nullptr, 0);
                hex_state.verts.clear();

                break;
        }
    }
};
} // namespace

void arcade::RunHex() {
    Singleton::Get<WindowState>().clear_color = colors::MAP_BACKGROUND;

    HexState& hex_state = Singleton::Get<HexState>();
    HexTuning& tuning   = Singleton::Get<HexTuning>();
    hex_state.hex_map = GenerateTerrain({ 100, 100 }, 3489);
    hex_state.hex_draw = HexToHexDrawInfo(hex_state.hex_map, tuning.terrain_scheme);
    ImGuiInit imgui_init { };

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

    for (u32 i = 0; i < hex_state.units.size(); i++) {
        Handle<Unit> unit_handle = hex_state.units.IndexToHandle(i);
        hex_state.units_by_axial[hex_state.units[unit_handle].axial].EmplaceBack(unit_handle);
    }
    GenerateTerritory(hex_state);
    hex_state.units_by_axial.clear();

    // Systems
    Orchestra orchestra { };
    orchestra.Add<DebugSystem>();
    orchestra.Add<TickSystem>();

    orchestra.Add<InputSystem>();
    orchestra.Add<InputNodeSystem>();
    orchestra.Add<ImGuiBeginFrameSystem>();

    orchestra.Add<HexSystem>();
    orchestra.Add<HexTuningPanelSystem>();

    orchestra.Add<AnimationSystem>();
    orchestra.Add<ParticleSystem>();

    orchestra.Add<CameraSystem>();
    orchestra.Add<RenderNodeSystem>();
    orchestra.Add<ImGuiRenderSystem>();
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
