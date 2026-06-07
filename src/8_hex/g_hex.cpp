#include "0_engine/g_globals.hpp"
#include "0_engine/r_window_state.hpp"
#include "0_engine/u_collections.hpp"
#include "0_engine/u_fonts.hpp"
#include "0_engine/u_texture.hpp"
#include "0_engine/u_util.hpp"
#include "1_systems/i_input_system.hpp"
#include "1_systems/r_camera_system.hpp"
#include "1_systems/r_render.hpp"
#include "1_systems/r_ui_node.hpp"
#include "1_systems/r_ui_node_data.hpp"
#include "1_systems/t_debug_system.hpp"
#include "1_systems/t_tick_system.hpp"
#include "1_systems/u_animation_system.hpp"
#include "1_systems/u_orchestra.hpp"
#include "SDL3/SDL_keycode.h"
#include "SDL3_ttf/SDL_ttf.h"
#include "g_arcade.hpp"

import pcg.hex.core;
import pcg.hex.counter;
import pcg.hex.types;
import pcg.hex.terrain;
import pcg.hex.render;
import pce.engine.colors;

namespace pcg {
using namespace pce;
using namespace pce::ui;
namespace {
[[nodiscard]] b8 AxialIsEnemy(const HexState& hex_state, const int2 axial) { return hex_state.units_by_axial.contains(axial) && !std::ranges::contains(hex_state.units_by_axial.at(axial) | hex_state.units.handle_to_view(), hex_state.player_tag, &Unit::tag); }
[[nodiscard]] b8 AxialIsEnemyOrZoc(const HexState& hex_state, const int2 axial) {
    return AxialIsEnemy(hex_state, axial) || std::ranges::any_of(HEX_AXIAL_NEIGHBOURS, [&](const int2 axial_offset) -> b8 { return AxialIsEnemy(hex_state, axial + axial_offset); });
}
[[nodiscard]] u8 AxialAdjecentEnemyControl(const HexState& hex_state, const int2 axial) {
    return std::ranges::count_if(HEX_AXIAL_NEIGHBOURS, [&](const int2 axial_offset) -> b8 { return hex_state.hex_map.Contains(axial + axial_offset) && hex_state.hex_map[axial + axial_offset].owner.tag != hex_state.player_tag; });
}
List<AxialAndCost> HexAxialPathAStar(HexState& hex_state, const int2 axial_start, const int2 axial_end) {
    List<AxialAndCost> axial_path;
    auto cmp = [](const AxialAndCost& a, const AxialAndCost& b) -> ::b8 { return a.cost > b.cost; };
    std::priority_queue<AxialAndCost, std::vector<AxialAndCost>, decltype(cmp)> frontier(cmp);
    UnorderedMap<int2, int2> came_from;
    UnorderedMap<int2, u32> cost_at_axial;

    frontier.push(AxialAndCost { .axial = axial_start, .cost = 0 });
    came_from[axial_start] = axial_start;
    cost_at_axial[axial_start] = 0;

    //

    while (!frontier.empty()) {
        const AxialAndCost current = frontier.top();
        frontier.pop();

        if (current.axial == axial_end) { break; } // finished

        for (u32 side = 0; side < HEX_CORNERS; side++) {
            const int2 axial_neighbour_offset = HEX_AXIAL_NEIGHBOURS[side];
            const int2 axial_next = current.axial + axial_neighbour_offset;
            if (!hex_state.hex_map.Contains(axial_next) || AxialIsEnemy(hex_state, axial_next)) { continue; }

            const Hex& hex = hex_state.hex_map[axial_next];
            const u32 cost_conquer = (hex.owner.tag != hex_state.player_tag) + hex.owner.contested + AxialIsEnemyOrZoc(hex_state, axial_next) + (AxialAdjecentEnemyControl(hex_state, axial_next) > 0) * 1U;
            u32 cost_terrain = TerrainToMovementCost(hex.terrain_type);
            const b8 has_road = hex_state.hex_map.Contains(current.axial) && static_cast<RoadLevel>(hex_state.hex_map[current.axial].roads.Test(side)) != RoadLevel::ROAD_LEVEL_NONE;
            if (has_road) { cost_terrain = cost_terrain > MOVE_COST_ROAD_REDUCTION ? cost_terrain - MOVE_COST_ROAD_REDUCTION : 1U; }
            const u32 cost_new = cost_at_axial[current.axial] + cost_terrain + cost_conquer;
            if (!cost_at_axial.contains(axial_next) || cost_new < cost_at_axial[axial_next]) {
                const u32 heuristic_distance = HexAxialDistance(axial_next, axial_end);
                cost_at_axial[axial_next] = cost_new;
                frontier.push({ .axial = axial_next, .cost = cost_new + heuristic_distance });
                came_from[axial_next] = current.axial;
            }
        }
    }
    if (came_from.contains(axial_end)) {
        for (int2 axial = axial_end; axial != axial_start; axial = came_from[axial]) { axial_path.EmplaceBack(AxialAndCost { .axial = axial, .cost = cost_at_axial[axial] }); }
        std::ranges::reverse(axial_path);
    }
    return axial_path;
}

void UnitToCounterAppend(HexState& hex_state) {
    for (const auto& [axial_unit, unit_handles] : hex_state.units_by_axial) {
        CounterStack& counter = hex_state.counters.Get();
        counter.axial = axial_unit;
        u32 dmg = 0;
        u32 def = 0;
        u32 move = 0;
        counter.stack = { };
        const u32 counters_on_hex = math::Min<u32>(counter.stack.size(), unit_handles.size());
        b8 axial_is_selected = hex_state.pseudo_states.unit_selection.has_value() && hex_state.pseudo_states.axial_select == axial_unit;
        if (axial_is_selected) {
            dmg = hex_state.pseudo_states.unit_selection->dmg_sum;
            def = hex_state.pseudo_states.unit_selection->def_sum;
            move = hex_state.pseudo_states.unit_selection->move_min;
            // drawing selected
            u32 i = 0;
            for (; i < hex_state.pseudo_states.unit_selection->unit_handles.size(); i++) {
                const Handle<Unit> unit_handle_selected = hex_state.pseudo_states.unit_selection->unit_handles[i];
                const Unit& unit_selected = hex_state.units[unit_handle_selected];
                counter.stack[i] = Counter { .color_background = CountryTagToColor(unit_selected.tag), .color_icon = unit_selected.color, .color_border = colors::HEX_SELECT };
            }

            // only color for rest
            for (u32 j = 0; j < counters_on_hex; j++) {
                const Handle<Unit> unit_handle = unit_handles[j];
                if (hex_state.pseudo_states.unit_selection->unit_handles.Contains(unit_handle)) { continue; }
                const Unit& unit = hex_state.units[unit_handle];
                counter.stack[i++] = Counter { .color_background = CountryTagToColor(unit.tag), .color_icon = unit.color, .color_border = colors::YELLOW };
            }
        } else {
            // drawing plain
            for (u32 i = 0; i < counters_on_hex; i++) {
                const Handle<Unit> unit_handle = unit_handles[i];
                const Unit& unit = hex_state.units[unit_handle];
                counter.stack[i] = Counter { .color_background = CountryTagToColor(unit.tag), .color_icon = unit.color, .color_border = colors::BLACK };
                dmg += unit.dmg();
                def += unit.def();
                move = math::Max(unit.move, move);
            }
        }

        Handle<Unit> unit_handle_largest_echelon = std::ranges::max(axial_is_selected ? hex_state.pseudo_states.unit_selection->unit_handles : unit_handles, { }, [&](Handle<Unit> unit_handle) -> Echelon { return hex_state.units[unit_handle].echelon; });

        const Unit& unit_largest_echelon = hex_state.units[unit_handle_largest_echelon];

        counter.icon = unit_largest_echelon.icon;
        counter.label_top.SetText(EchelonToString(unit_largest_echelon.echelon));
        counter.label_center.SetText(UnitIconToString(unit_largest_echelon.icon));
        counter.label_bottom.SetText(std::format("{}-{}-{}", dmg / 10, def / 10, move));
        String unit_name = String(Span(unit_largest_echelon.name));
        counter.label_vertical.SetText(unit_name);
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
        if (hex_state.pseudo_states.axial_hover) { VertHexAppend(hex_state.verts, camera.scale, camera.WorldToScreen(HexAxialToWorld(hex_state.pseudo_states.axial_hover.value())), colors::HEX_HOVER); }
        if (hex_state.pseudo_states.axial_select) { VertHexAppend(hex_state.verts, camera.scale * 0.95F, camera.WorldToScreen(HexAxialToWorld(hex_state.pseudo_states.axial_select.value())), colors::HEX_SELECT); }

        // render map
        for (u32 i = 0; i < hex_state.hex_map.Size(); i++) {
            const Hex& hex = hex_state.hex_map.data[i];
            const int2 axial = hex_state.hex_map.IndexToAxial(i);
            const float2 world = HexAxialToWorld(axial);
            Color color = TerrainToColorScheme(hex.terrain_type);
            VertHexAppend(hex_state.verts, camera.scale * 0.9F, camera.WorldToScreen(world), color);
            if (hex.owner.contested) { VertHexAppend(hex_state.verts, camera.scale * 0.70F, camera.WorldToScreen(world), color.Mul(0.80F)); }
        }
        (void)SDL_RenderGeometry(window_state.renderer, nullptr, hex_state.verts);
        hex_state.verts.clear();

        AppendCountryBorders(hex_state, camera);
        AppendRiverMesh(hex_state, camera);
        AppendRoadMesh(hex_state, camera);
        (void)SDL_RenderGeometry(window_state.renderer, nullptr, hex_state.verts);
        hex_state.verts.clear();

        AppendTerrainFeatures(hex_state, camera);

        // render hq
        if (hex_state.pseudo_states.unit_selection) {
            auto draw_line = [&](const Color color, const int2 axial_a, const int2 axial_b) {
                const float2 screen_a = camera.WorldToScreen(HexAxialToWorld(axial_a));
                const float2 screen_b = camera.WorldToScreen(HexAxialToWorld(axial_b));
                VertObbAppend(hex_state.verts, OBB::BetweenPoints(screen_a, screen_b, camera.scale * 0.15F), colors::BLACK.WithAlpha(0.7F));
                VertObbAppend(hex_state.verts, OBB::BetweenPoints(screen_a, screen_b, camera.scale * 0.1F), color);
            };
            for (const Handle<Unit>& unit_handle : hex_state.pseudo_states.unit_selection.value().unit_handles) {
                const int2 axial_unit = hex_state.units[unit_handle].axial;

                for (const Handle<Unit>& unit_handle_child : hex_state.units_oob[unit_handle]) {
                    const int2 axial_child = hex_state.units[unit_handle_child].axial;
                    draw_line(colors::LIGHT_SKY_BLUE, axial_unit, axial_child);
                }
                const HandleOptional<Unit>& unit_handle_parent = hex_state.units[unit_handle].parent;
                if (unit_handle_parent.IsValid()) {
                    const int2 axial_parent = hex_state.units[unit_handle_parent.GetHandle()].axial;
                    draw_line(colors::RED, axial_unit, axial_parent);
                    for (const Handle<Unit>& unit_handle_sibling : hex_state.units_oob[unit_handle_parent.GetHandle()]) {
                        if (unit_handle_sibling == unit_handle) { continue; }
                        const int2 axial_sibling = hex_state.units[unit_handle_sibling].axial;
                        draw_line(colors::YELLOW, axial_parent, axial_sibling);
                    }
                }
            }
        }
        (void)SDL_RenderGeometry(window_state.renderer, nullptr, hex_state.verts);
        hex_state.verts.clear();

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

                // path finding
                List<AxialAndCost> axial_path = HexAxialPathAStar(hex_state, axial_start, axial_hover);
                const List<AxialAndCost>::iterator it = std::ranges::upper_bound(axial_path, hex_state.pseudo_states.unit_selection->move_min, std::less { }, &AxialAndCost::cost);
                if (it != axial_path.begin()) {
                    for (List<AxialAndCost>::iterator step = axial_path.begin(); step != it; ++step) {
                        Hex& hex_center = hex_state.hex_map[step->axial];
                        if (hex_center.owner.tag != hex_state.player_tag) { hex_center.owner = HexOwner { .tag = hex_state.player_tag, .contested = true }; } // conquer
                        // zone of control
                        for (const int2 offset : HEX_AXIAL_NEIGHBOURS) {
                            const int2 axial_neighbour = step->axial + offset;
                            if (!hex_state.hex_map.Contains(axial_neighbour)) { continue; }
                            if (!AxialIsEnemyOrZoc(hex_state, axial_neighbour)) {
                                Hex& hex_neighbour = hex_state.hex_map[axial_neighbour];
                                if (hex_neighbour.owner.tag != hex_state.player_tag) { hex_neighbour.owner = HexOwner { .tag = hex_state.player_tag, .contested = true }; } // conquer
                            }
                        }
                    }

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
                    const float2 screen = camera.WorldToScreen(world);

                    const Color movement_color = cost_and_axial.cost > units_selected_movement ? colors::BLACK : colors::RUBY_RED;
                    VertHexAppend(hex_state.verts, camera.scale * 0.25F, screen, movement_color);
                }
                (void)SDL_RenderGeometry(window_state.renderer, nullptr, hex_state.verts);
                hex_state.verts.clear();

                const f32 pt = camera.scale * 0.3F;
                if (static_cast<FontSize>(pt) < FONT_MIN_SIZE) { break; }
                const Font& font_movement = font_collection.GetFontBold(static_cast<FontSizes>(pt));
                TTF_SetFontWrapAlignment(font_movement, TTF_HORIZONTAL_ALIGN_CENTER);
                for (AxialAndCost cost_and_axial : axial_path) {
                    const float2 world = HexAxialToWorld(cost_and_axial.axial);
                    const float2 screen = camera.WorldToScreen(world);

                    const Label& label = hex_state.label_pool.Get();
                    const String string_distance = std::format("{}", cost_and_axial.cost);
                    (void)TTF_SetTextWrapWidth(label, static_cast<i32>(camera.scale));
                    (void)TTF_SetTextFont(label, font_movement);
                    label.SetText(string_distance);
                    (void)TTF_DrawRendererText(label, screen.x - camera.scale * 0.5F, screen.y - pt * 0.5F);
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
                int2 axial_battle = units_defender[0].axial;

                // attack
                const u32 dmg = hex_state.pseudo_states.unit_selection->dmg_sum;
                const u32 def = std::ranges::fold_left(units_defender | std::views::transform(&Unit::def), 0U, std::plus { });
                const f32 ratio = static_cast<f32>(dmg) / static_cast<f32>(def);
                enum class PostBattleOutcome { DEFENDER_COUNTER_ATTACKED, DEFENDER_SCOUTED, DEFENDER_HELD, DEFENDER_RETREAT, DEFENDER_ROUT, DEFENDER_SURRENDER };
                struct BattleOutcome {
                    PostBattleOutcome post_battle_outcome;
                    u32 dmg_attacker;
                    u32 dmg_defender;
                } battle_outcome;
                if (ratio >= 2.0F) {
                    battle_outcome = BattleOutcome { .post_battle_outcome = PostBattleOutcome::DEFENDER_RETREAT, .dmg_attacker = 1U, .dmg_defender = 3U };
                } else if (ratio >= 1.5F) {
                    battle_outcome = BattleOutcome { .post_battle_outcome = PostBattleOutcome::DEFENDER_HELD, .dmg_attacker = 2U, .dmg_defender = 1U };
                } else if (ratio <= 0.3F) {
                    battle_outcome = BattleOutcome { .post_battle_outcome = PostBattleOutcome::DEFENDER_COUNTER_ATTACKED, .dmg_attacker = 3U, .dmg_defender = 1U };
                } else {
                    battle_outcome = BattleOutcome { .post_battle_outcome = PostBattleOutcome::DEFENDER_SCOUTED, .dmg_attacker = 2U, .dmg_defender = 1U };
                }

                const b8 attacker_won = battle_outcome.post_battle_outcome == PostBattleOutcome::DEFENDER_RETREAT || battle_outcome.post_battle_outcome == PostBattleOutcome::DEFENDER_ROUT || battle_outcome.post_battle_outcome == PostBattleOutcome::DEFENDER_SURRENDER;

                for (Unit& attacker : units_attacker) {
                    attacker.squad_inf = math::SaturatingSub(attacker.squad_inf, battle_outcome.dmg_attacker);
                    attacker.squad_art = math::SaturatingSub(attacker.squad_art, battle_outcome.dmg_attacker / 8);
                    attacker.squad_tank = math::SaturatingSub(attacker.squad_art, battle_outcome.dmg_attacker / 64);
                    attacker.move = math::SaturatingSub(attacker.move, MOVE_COST_ATTACK);
                }
                for (Unit& defender : units_defender) {
                    defender.squad_inf = math::SaturatingSub(defender.squad_inf, battle_outcome.dmg_defender);
                    defender.squad_art = math::SaturatingSub(defender.squad_art, battle_outcome.dmg_defender / 8);
                    defender.squad_tank = math::SaturatingSub(defender.squad_art, battle_outcome.dmg_defender / 64);
                }
                switch (battle_outcome.post_battle_outcome) {
                    case PostBattleOutcome::DEFENDER_COUNTER_ATTACKED:
                        for (Unit& attacker : units_attacker) {
                            const int2 axial_retreat = attacker.tag == CountryTag::TAG_GER ? int2 { -2, 0 } : int2 { 2, 0 };
                            attacker.axial += axial_retreat;
                        }
                        break;
                    case PostBattleOutcome::DEFENDER_SCOUTED:
                    case PostBattleOutcome::DEFENDER_HELD: break;
                    case PostBattleOutcome::DEFENDER_RETREAT:
                        for (Unit& defender : units_defender) {
                            const int2 axial_retreat = defender.tag == CountryTag::TAG_GER ? int2 { -2, 0 } : int2 { 2, 0 };
                            defender.axial += axial_retreat;
                        }
                        break;
                    case PostBattleOutcome::DEFENDER_ROUT:
                        for (Unit& defender : units_defender) {
                            const int2 axial_retreat = defender.tag == CountryTag::TAG_GER ? int2 { -3, 0 } : int2 { 3, 0 };
                            defender.axial += axial_retreat;
                        }
                        break;
                    case PostBattleOutcome::DEFENDER_SURRENDER:
                        // unit breaks
                        break;
                }

                if (attacker_won) {
                    Hex& hex_battle = hex_state.hex_map[axial_battle];
                    if (hex_battle.owner.tag != hex_state.player_tag) { hex_battle.owner = HexOwner { .tag = hex_state.player_tag, .contested = true }; } // conquer
                }

                break;
            }
            case PlayerAction::PLAYER_ACTION_ATTACK_HOVER:
                const b8 within_range = std::ranges::all_of(hex_state.pseudo_states.unit_selection->unit_handles | hex_state.units.handle_to_view(), [&](const Unit& unit) -> b8 { return HexAxialDistance(unit.axial, hex_state.pseudo_states.axial_hover.value()) == 1; });
                const b8 attacker_is_hq = std::ranges::contains(hex_state.pseudo_states.unit_selection->unit_handles | hex_state.units.handle_to_view(), UnitIcon::ICON_HQ, &Unit::icon);
                const b8 attacker_has_movement = hex_state.pseudo_states.unit_selection->move_min > MOVE_COST_ATTACK;
                const b8 can_attack = attacker_has_movement && within_range && !attacker_is_hq;

                const float2 world = HexAxialToWorld(hex_state.pseudo_states.axial_hover.value());
                const float2 screen = camera.WorldToScreen(world);

                const Color color_inner = can_attack ? colors::RUBY_RED : colors::BLACK;
                VertHexAppend(hex_state.verts, camera.scale * 0.25F, screen, colors::GRAY);
                VertHexAppend(hex_state.verts, camera.scale * 0.20F, screen, color_inner);

                if (can_attack) { VertHexAppend(hex_state.verts, camera.scale * 0.10F, screen, colors::DARK_GREEN); }
                (void)SDL_RenderGeometry(window_state.renderer, nullptr, hex_state.verts);
                hex_state.verts.clear();

                const f32 pt = camera.scale * 0.1F;
                if (static_cast<FontSize>(pt) < FONT_MIN_SIZE) { break; }
                const Font& font_attack = font_collection.GetFontBold(static_cast<FontSizes>(pt));
                TTF_SetFontWrapAlignment(font_attack, TTF_HORIZONTAL_ALIGN_CENTER);
                const Label& label = hex_state.label_pool.Get();
                (void)TTF_SetTextWrapWidth(label, static_cast<i32>(camera.scale));
                (void)TTF_SetTextFont(label, font_attack);

                if (can_attack) {
                    auto units_attacker = hex_state.pseudo_states.unit_selection->unit_handles | hex_state.units.handle_to_view();
                    auto units_defender = hex_state.units_by_axial[hex_state.pseudo_states.axial_hover.value()] | hex_state.units.handle_to_view();
                    const u32 dmg = hex_state.pseudo_states.unit_selection->dmg_sum;
                    const u32 def = std::ranges::fold_left(units_defender | std::views::transform(&Unit::def), 0U, std::plus { });
                    label.SetText(std::format("{}->{}", dmg, def));
                } else if (attacker_is_hq) {
                    label.SetText("hq cannot attack");
                } else if (!within_range) {
                    label.SetText("can only attack adjecent units");
                } else if (!attacker_has_movement) {
                    label.SetText(std::format("no movement\n{} < {}", hex_state.pseudo_states.unit_selection->move_min, MOVE_COST_ATTACK));
                }
                const float2 screen_f = static_cast<float2>(screen);
                (void)TTF_DrawRendererText(label, screen_f.x - camera.scale * 0.5F, screen_f.y - pt * 0.5F);

                break;
        }
    }
};

const char* NumberToOrdinal(u32 number) {
    switch (number % 100) {
        case 11:
        case 12:
        case 13: return "th";
        default:
            switch (number % 10) {
                case 1: return "st";
                case 2: return "nd";
                case 3: return "rd";
                default: return "th";
            }
    }
}
constexpr std::array<std::pair<u32, const char*>, 13> ROMAN_NUMERALS { { { 1000, "M" }, { 900, "CM" }, { 500, "D" }, { 400, "CD" }, { 100, "C" }, { 90, "XC" }, { 50, "L" }, { 40, "XL" }, { 10, "X" }, { 9, "IX" }, { 5, "V" }, { 4, "IV" }, { 1, "I" } } };

String NumberToRomanNumerals(u32 num) {
    if (num < 1 || num > 3999) { return std::to_string(num); }
    String romans;
    for (const auto& [value, symbol] : ROMAN_NUMERALS) {
        while (num >= value) {
            romans += symbol;
            num -= value;
        }
    }
    return romans;
}

struct UnitPreset {
    u32 move;
    u32 squad_inf;
    u32 squad_art;
    u32 squad_tank;
};
UnitPreset GetUnitPreset(UnitIcon icon) {
    switch (icon) {
        case UnitIcon::ICON_INF: return UnitPreset { .move = 16, .squad_inf = 3 * 3 * 3, .squad_art = 3, .squad_tank = 0 };
        case UnitIcon::ICON_ART: return UnitPreset { .move = 16, .squad_inf = 3, .squad_art = 4 * 3, .squad_tank = 0 };
        case UnitIcon::ICON_HQ: return UnitPreset { .move = 64, .squad_inf = 3, .squad_art = 0, .squad_tank = 0 };
        case UnitIcon::ICON_TANK: return UnitPreset { .move = 32, .squad_inf = 3 * 3, .squad_art = 6 * 3, .squad_tank = 5 * 3 };
        default: assert(false); std::unreachable();
    }
}
void HexStateUpdateUnitStats(HexState& hex_state) {
    for (Unit& unit : hex_state.units) {
        UnitPreset unit_preset = GetUnitPreset(unit.icon);
        unit.move = unit_preset.move;
        unit.squad_inf = unit_preset.squad_inf;
        unit.squad_art = unit_preset.squad_art;
        unit.squad_tank = unit_preset.squad_tank;
    }
}
HandleOptional<Unit> UnitGetParentWithEchelon(HexState& hex_state, Handle<Unit> unit_handle, Echelon echelon) {
    while (hex_state.units[unit_handle].parent.IsValid()) {
        Handle<Unit> unit_handle_parent = hex_state.units[unit_handle].parent.GetHandle();
        Unit& unit_parent = hex_state.units[unit_handle_parent];
        if (unit_parent.echelon >= echelon) { return hex_state.units[unit_handle].parent; }
        unit_handle = unit_handle_parent;
    }
    return std::nullopt;
}

void HexStateUpdateOOB(HexState& hex_state) {
    f32 hue_next = 0.0F;
    u32 number_army = 1;
    u32 number_corps = 15;
    u32 number_div = 100;

    // generate oob.
    hex_state.units_oob.clear();
    for (u32 i = 0; i < hex_state.units.size(); i++) {
        const Handle<Unit> unit_handle = hex_state.units.IndexToHandle(i);
        const Unit& unit = hex_state.units[unit_handle];
        hex_state.units_oob[unit.parent.GetHandle()].EmplaceBack(unit_handle);
    }

    // create colors and names
    auto get_unit_color = [&](const Unit& unit) -> Color {
        if (unit.icon != UnitIcon::ICON_HQ && unit.parent.IsValid()) { return hex_state.units[unit.parent.GetHandle()].color; }
        const Color color = Color::FromHsl(hue_next, 0.5F, 0.5F);
        hue_next = std::fmod(hue_next + 37.0F, 360.0F);
        return color;
    };

    const std::array letters = {
        'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'K', 'L', 'M', 'N', 'O', 'P',
    };
    auto get_unit_name = [&](const Unit& unit, u32 i) -> String {
        switch (unit.echelon) {
            case Echelon::ECHELON_COMPANY: return std::format("{}", letters[i % letters.size()]);
            case Echelon::ECHELON_BATTALION: return std::format("{}/{}", NumberToRomanNumerals(i + 1), unit.parent.IsValid() ? String(hex_state.units[unit.parent.GetHandle()].name) : "dtc");
            case Echelon::ECHELON_REGIMENT: {
                u32 regiment_number = 0;
                switch (unit.tag) {
                    case CountryTag::TAG_GER: regiment_number = Rand(200U, 500U); break;
                    case CountryTag::TAG_SOV: regiment_number = Rand(600U, 900U); break;
                    default: regiment_number = Rand(200U, 999U); break;
                }
                return std::format("{}{} Rgt", regiment_number, NumberToOrdinal(regiment_number));
            }
            case Echelon::ECHELON_DIVISION: return std::format("{}{} Div", ++number_div, NumberToOrdinal(number_div));
            case Echelon::ECHELON_CORPS: return std::format("{}{} Corps", ++number_corps, NumberToOrdinal(number_corps));
            case Echelon::ECHELON_ARMY: return std::format("{}{} Army", ++number_army, NumberToOrdinal(number_army));
            default: return "fail";
        }
    };

    Queue<Handle<Unit>> queue;
    queue.EmplaceBack(HandleOptional<Unit> { std::nullopt }.GetHandle());

    while (!queue.empty()) {
        List<Handle<Unit>> unit_handles_children = hex_state.units_oob[queue.Front()];
        queue.Pop();
        for (u32 i = 0; i < unit_handles_children.size(); i++) {
            Handle<Unit> unit_handle_child = unit_handles_children[i];
            queue.EmplaceBack(unit_handle_child);
            Unit& unit_child = hex_state.units[unit_handle_child];
            const String name = get_unit_name(unit_child, i);
            std::memcpy(unit_child.name.data(), name.c_str(), math::Min(name.size(), static_cast<u32>(unit_child.name.size())));
            unit_child.color = get_unit_color(unit_child);
        }
    }
}
} // namespace

void arcade::RunHex() {
    Singleton::Get<WindowState>().clear_color = Color::FromHsl(180.0F, 0.5F, 0.20F);

    HexState& hex_state = Singleton::Get<HexState>();
    hex_state.hex_map = GenerateTerrainType({ 40, 40 }, 3489);

    CameraState& camera = Singleton::Get<CameraState>();
    camera.map_world_min = { 0.0F, 0.0F };
    camera.map_world_max = HexAxialToWorld(static_cast<int2>(hex_state.hex_map.map_size - uint2 { 1, 1 }));

    // GER: Heeresgruppe → I.Korps → Rgt → Bn
    const Handle<Unit> ger_hgr = hex_state.units.EmplaceBack(Unit { .tag = CountryTag::TAG_GER, .echelon = Echelon::ECHELON_ARMY, .icon = UnitIcon::ICON_HQ, .axial = { 0, 2 } });
    const Handle<Unit> ger_kps = hex_state.units.EmplaceBack(Unit { .parent = ger_hgr, .tag = CountryTag::TAG_GER, .echelon = Echelon::ECHELON_CORPS, .icon = UnitIcon::ICON_HQ, .axial = { 2, 2 } });
    const Handle<Unit> ger_r7 = hex_state.units.EmplaceBack(Unit { .parent = ger_kps, .tag = CountryTag::TAG_GER, .echelon = Echelon::ECHELON_REGIMENT, .icon = UnitIcon::ICON_INF, .axial = { 4, 1 } });
    const Handle<Unit> ger_r8 = hex_state.units.EmplaceBack(Unit { .parent = ger_kps, .tag = CountryTag::TAG_GER, .echelon = Echelon::ECHELON_REGIMENT, .icon = UnitIcon::ICON_INF, .axial = { 4, 3 } });
    (void)hex_state.units.EmplaceBack(Unit { .parent = ger_kps, .tag = CountryTag::TAG_GER, .echelon = Echelon::ECHELON_BATTALION, .icon = UnitIcon::ICON_TANK, .axial = { 5, 2 } });
    (void)hex_state.units.EmplaceBack(Unit { .parent = ger_kps, .tag = CountryTag::TAG_GER, .echelon = Echelon::ECHELON_BATTALION, .icon = UnitIcon::ICON_ART, .axial = { 3, 2 } });
    (void)hex_state.units.EmplaceBack(Unit { .parent = ger_r7, .tag = CountryTag::TAG_GER, .echelon = Echelon::ECHELON_BATTALION, .icon = UnitIcon::ICON_INF, .axial = { 6, 1 } });
    (void)hex_state.units.EmplaceBack(Unit { .parent = ger_r8, .tag = CountryTag::TAG_GER, .echelon = Echelon::ECHELON_BATTALION, .icon = UnitIcon::ICON_INF, .axial = { 6, 3 } });

    // SOV: Front → I Gds Corps → Rifle Rgts → Bns
    const Handle<Unit> sov_frt = hex_state.units.EmplaceBack(Unit { .tag = CountryTag::TAG_SOV, .echelon = Echelon::ECHELON_ARMY, .icon = UnitIcon::ICON_HQ, .axial = { 14, 3 } });
    const Handle<Unit> sov_kps = hex_state.units.EmplaceBack(Unit { .parent = sov_frt, .tag = CountryTag::TAG_SOV, .echelon = Echelon::ECHELON_CORPS, .icon = UnitIcon::ICON_HQ, .axial = { 13, 3 } });
    const Handle<Unit> sov_r16 = hex_state.units.EmplaceBack(Unit { .parent = sov_kps, .tag = CountryTag::TAG_SOV, .echelon = Echelon::ECHELON_REGIMENT, .icon = UnitIcon::ICON_INF, .axial = { 11, 2 } });
    const Handle<Unit> sov_r18 = hex_state.units.EmplaceBack(Unit { .parent = sov_kps, .tag = CountryTag::TAG_SOV, .echelon = Echelon::ECHELON_REGIMENT, .icon = UnitIcon::ICON_INF, .axial = { 11, 4 } });
    (void)hex_state.units.EmplaceBack(Unit { .parent = sov_kps, .tag = CountryTag::TAG_SOV, .echelon = Echelon::ECHELON_BATTALION, .icon = UnitIcon::ICON_TANK, .axial = { 10, 3 } });
    (void)hex_state.units.EmplaceBack(Unit { .parent = sov_kps, .tag = CountryTag::TAG_SOV, .echelon = Echelon::ECHELON_BATTALION, .icon = UnitIcon::ICON_ART, .axial = { 12, 3 } });
    (void)hex_state.units.EmplaceBack(Unit { .parent = sov_r16, .tag = CountryTag::TAG_SOV, .echelon = Echelon::ECHELON_BATTALION, .icon = UnitIcon::ICON_INF, .axial = { 9, 2 } });
    (void)hex_state.units.EmplaceBack(Unit { .parent = sov_r18, .tag = CountryTag::TAG_SOV, .echelon = Echelon::ECHELON_BATTALION, .icon = UnitIcon::ICON_INF, .axial = { 9, 4 } });
    (void)hex_state.units.EmplaceBack(Unit { .parent = sov_frt, .tag = CountryTag::TAG_SOV, .echelon = Echelon::ECHELON_REGIMENT, .icon = UnitIcon::ICON_INF, .axial = { 4, 2 } }); // encircled

    // USA: 12th Army Group → V Corps → units
    const Handle<Unit> usa_hgr = hex_state.units.EmplaceBack(Unit { .tag = CountryTag::TAG_USA, .echelon = Echelon::ECHELON_ARMY, .icon = UnitIcon::ICON_HQ, .axial = { 17, 1 } });
    const Handle<Unit> usa_kps = hex_state.units.EmplaceBack(Unit { .parent = usa_hgr, .tag = CountryTag::TAG_USA, .echelon = Echelon::ECHELON_CORPS, .icon = UnitIcon::ICON_HQ, .axial = { 17, 2 } });
    (void)hex_state.units.EmplaceBack(Unit { .parent = usa_kps, .tag = CountryTag::TAG_USA, .echelon = Echelon::ECHELON_REGIMENT, .icon = UnitIcon::ICON_INF, .axial = { 17, 3 } });
    (void)hex_state.units.EmplaceBack(Unit { .parent = usa_kps, .tag = CountryTag::TAG_USA, .echelon = Echelon::ECHELON_BATTALION, .icon = UnitIcon::ICON_TANK, .axial = { 17, 4 } });

    HexStateUpdateOOB(hex_state);
    HexStateUpdateUnitStats(hex_state);

    for (u32 i = 0; i < hex_state.units.size(); i++) {
        Handle<Unit> unit_handle = hex_state.units.IndexToHandle(i);
        hex_state.units_by_axial[hex_state.units[unit_handle].axial].EmplaceBack(unit_handle);
    }
    GenerateTerritory(hex_state);
    GenerateRivers(hex_state, 3489);
    GenerateTerrainFeatures(hex_state, 3489);
    GenerateRoads(hex_state);
    hex_state.units_by_axial.clear();

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
