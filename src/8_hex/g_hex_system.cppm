module;

#include <cassert>

#include "SDL3/SDL_keycode.h"

export module hex.system;

import std;

import pce.std;
import pce.math;
import pce.globals;
import pce.window_state;
import pce.font;
import pce.sdl;
import pce.collections;
import pce.colors;
import pce.logger;
import pce.strong;
import pce.assets;
import pce.ui;

import pcs.input;
import pcs.camera;
import pcs.animation;
import pcs.easing;

import hex.hex;
import hex.enums;
import hex.types;
import hex.terrain;
import hex.counter;
import hex.render;

namespace hex {
using namespace hex::ui;
using namespace hex::math;

[[nodiscard]] b8 AxialIsEnemy(const HexState& hex_state, const int2 axial) { return hex_state.units_by_axial.contains(axial) && !std::ranges::contains(hex_state.units_by_axial.at(axial) | hex_state.units.handle_to_view(), hex_state.player_tag, &Unit::tag); }
[[nodiscard]] b8 AxialIsEnemyZoc(const HexState& hex_state, const int2 axial) {
    return std::ranges::any_of(HEX_AXIAL_NEIGHBOURS, [&](const int2 axial_offset) -> b8 { return AxialIsEnemy(hex_state, axial + axial_offset); });
}
[[nodiscard]] u8 AxialAdjecentEnemyControl(const HexState& hex_state, const int2 axial) {
    return std::ranges::count_if(HEX_AXIAL_NEIGHBOURS, [&](const int2 axial_offset) -> b8 { return hex_state.hex_map.Contains(axial + axial_offset) && hex_state.hex_map[axial + axial_offset].owner.tag != hex_state.player_tag; });
}
List<AxialAndCost> HexAxialPathAStar(HexState& hex_state, const int2 axial_start, const int2 axial_end, const MoveType movement, const u32 move_allowance) {
    List<AxialAndCost> axial_path;
    auto cmp = [](const AxialAndCost& a, const AxialAndCost& b) -> ::b8 { return a.cost > b.cost; };
    std::priority_queue<AxialAndCost, std::vector<AxialAndCost>, decltype(cmp)> frontier(cmp);
    UnorderedMap<int2, int2> came_from;
    UnorderedMap<int2, u32> cost_at_axial;

    frontier.push(AxialAndCost { .axial = axial_start, .cost = 0 });
    came_from[axial_start] = axial_start;
    cost_at_axial[axial_start] = 0;

    while (!frontier.empty()) {
        const AxialAndCost current = frontier.top();
        frontier.pop();

        if (current.axial == axial_end) { break; } // finished

        for (u32 edge = 0; edge < HEX_CORNERS; edge++) {
            const int2 axial_neighbour_offset = HEX_AXIAL_NEIGHBOURS[edge];
            const int2 axial_next = current.axial + axial_neighbour_offset;
            if (!hex_state.hex_map.Contains(axial_next) || AxialIsEnemy(hex_state, axial_next)) { continue; }

            const Hex& hex = hex_state.hex_map[axial_next];
            const u32 cost_conquer = (hex.owner.tag != hex_state.player_tag) + hex.owner.contested + AxialIsEnemyZoc(hex_state, axial_next) + (AxialAdjecentEnemyControl(hex_state, axial_next) > 0) * 1U;
            const RoadLevel road_level = static_cast<RoadLevel>(hex_state.hex_map[current.axial].roads.Test(edge));
            const b8 has_road = road_level != RoadLevel::ROAD_LEVEL_NONE;
            const MoveCost move_cost_terrain = has_road ? MovementCostRoad(road_level) : MoveCostTerrain(hex.terrain_type, hex.terrain_feature);
            const u32 crosses_river = !has_road && hex_state.hex_map[current.axial].river_edges.Test(edge); // roads carry a bridge
            u32 cost_terrain = 0;
            switch (movement) {
                case MoveType::MOVE_LEG: cost_terrain = move_cost_terrain.leg + crosses_river * MOVE_COST_RIVER.leg; break;
                case MoveType::MOVE_TAC: cost_terrain = move_cost_terrain.tac + crosses_river * MOVE_COST_RIVER.tac; break;
                case MoveType::MOVE_TRUCK: cost_terrain = move_cost_terrain.truck + crosses_river * MOVE_COST_RIVER.truck; break;
            }
            if (cost_terrain >= MOVE_COST_PROHIBITED) { continue; }

            const u32 cost_new = cost_terrain >= MOVE_COST_STOP ? move_allowance : cost_at_axial[current.axial] + cost_terrain + cost_conquer;
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
        u32 dmg_ranged = 0;
        u32 steps = 0;
        u32 move = 0;
        counter.stack = { };
        const u32 counters_on_hex = math::Min<u32>(counter.stack.size(), unit_handles.size());
        b8 axial_is_selected = hex_state.pseudo_states.unit_selection.has_value() && hex_state.pseudo_states.axial_select == axial_unit;
        if (axial_is_selected) {
            dmg = hex_state.pseudo_states.unit_selection->dmg_sum;
            dmg_ranged = hex_state.pseudo_states.unit_selection->dmg_ranged_sum;
            move = hex_state.pseudo_states.unit_selection->move_min;
            steps = hex_state.pseudo_states.unit_selection->steps;
            // drawing selected
            u32 i = 0;
            for (; i < hex_state.pseudo_states.unit_selection->unit_handles.size(); i++) {
                const Handle<Unit> unit_handle_selected = hex_state.pseudo_states.unit_selection->unit_handles[i];
                const Unit& unit_selected = hex_state.units[unit_handle_selected];
                counter.stack[i] = Counter { .color_background = CountryBranchToColor(unit_selected.tag, hex_state.unit_formations[unit_selected.formation].branch), .color_icon = unit_selected.color, .color_border = colors::COLOR_HEX_SELECT };
            }

            // only color for rest
            for (u32 j = 0; j < counters_on_hex; j++) {
                const Handle<Unit> unit_handle = unit_handles[j];
                if (hex_state.pseudo_states.unit_selection->unit_handles.Contains(unit_handle)) { continue; }
                const Unit& unit = hex_state.units[unit_handle];
                counter.stack[i++] = Counter { .color_background = CountryBranchToColor(unit.tag, hex_state.unit_formations[unit.formation].branch), .color_icon = unit.color, .color_border = colors::COLOR_YELLOW };
            }
        } else {
            // drawing plain
            for (u32 i = 0; i < counters_on_hex; i++) {
                const Handle<Unit> unit_handle = unit_handles[i];
                const Unit& unit = hex_state.units[unit_handle];
                counter.stack[i] = Counter { .color_background = CountryBranchToColor(unit.tag, hex_state.unit_formations[unit.formation].branch), .color_icon = unit.color, .color_border = colors::COLOR_BLACK };
                dmg += unit.dmg;
                dmg_ranged += unit.dmg_ranged;
                steps += unit.steps.current;
                move = Max(static_cast<u32>(unit.move.current), move);
            }
        }

        Handle<Unit> unit_handle_largest_echelon = std::ranges::max(axial_is_selected ? hex_state.pseudo_states.unit_selection->unit_handles : unit_handles, { }, [&](Handle<Unit> unit_handle) -> Echelon { return hex_state.units[unit_handle].echelon; });

        const Unit& unit_largest_echelon = hex_state.units[unit_handle_largest_echelon];

        counter.icon = unit_largest_echelon.icon;
        counter.ranged_type = unit_largest_echelon.ranged_type;
        counter.label_echelon.SetText(std::format("{}", EchelonToString(unit_largest_echelon.echelon)));
        counter.label_icon_placeholder.SetText(UnitIconToString(unit_largest_echelon.icon));
        counter.label_dmg.SetText(std::format("{}", dmg));
        counter.label_dmg_ranged.SetText(std::format("+{}", dmg_ranged));
        counter.label_move_allowance.SetText(std::format("{}", move));
        counter.label_steps.SetText(std::format("{}", steps));

        counter.label_name_div.SetText(UnitNameToString(unit_largest_echelon.name_div));
        counter.label_name_sub.SetText(UnitNameToString(unit_largest_echelon.name_sub));
    }
}
PlayerAction GetPlayerAction(const HexState& hex_state) {
    const InputState& input_state = Singleton::Get<InputState>();

    if (hex_state.turn_hq_state == TurnHqState::TURN_HQ_OBJ_PLACEMENT) { return PlayerAction::PLAYER_ACTION_NONE; }
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

[[nodiscard]] TurnHqState HexSystemTurnHqLogic (HexState& hex_state) {
    const CameraState& camera = Singleton::Get<CameraState>();
    const InputState& input_state = Singleton::Get<InputState>();
    const WindowState& window_state = Singleton::Get<WindowState>();
    const FontCollection& font_collection = Singleton::Get<FontCollection>();

    b8 initalize = hex_state.turn_hq_state_changed;

    // hq phase title, top center, with the active formation counter under it
    if (hex_state.turn_hq_state != TurnHqState::TURN_HQ_NONE) {
        const Font& font_title = font_collection.GetFontBoldCompact(FontSizes::title);
        const Label& label_title = hex_state.label_pool.Get();
        label_title.SetText(std::format("{}", hex_state.turn_hq_state));
        const AABB area_title = AABB::FromPoint(float2 { 0.0F, static_cast<f32>(window_state.screen_size.y) * 0.06F }, float2 { static_cast<f32>(window_state.screen_size.x), static_cast<f32>(FontSizes::title) });
        DrawText(font_title, label_title, area_title.WithOffset(float2 { 3.0F }), colors::COLOR_BLACK, TextAlignment::CENTER);
        DrawText(font_title, label_title, area_title, colors::COLOR_WHITE, TextAlignment::CENTER);

        if (hex_state.unit_formation_active.IsValid()) {
            constexpr f32 COUNTER_ACTIVE_SIZE = 120.0F;
            const AABB area_counter_active = AABB::FromCenter(float2 { static_cast<f32>(window_state.screen_size.x) * 0.5F, area_title.point.y + area_title.size.y + static_cast<f32>(FontSizes::h1) * 2.0F + COUNTER_ACTIVE_SIZE * 0.5F + 20.0F }, float2 { COUNTER_ACTIVE_SIZE });
            DrawFormationCounter(window_state, hex_state.unit_formations[hex_state.unit_formation_active.GetHandle()], area_counter_active, hex_state.label_pool);
        }
    }

    switch (hex_state.turn_hq_state) {
        case TurnHqState::TURN_HQ_NONE: {
            return TurnHqState::TURN_HQ_START;
        }
        case TurnHqState::TURN_HQ_START: {
            static List<Handle<UnitFormation>> unit_formation_reel;
            constexpr f32 CARD_SIZE = 180.0F;
            // hq draw reel, case-opening style: counters scroll past a center marker and decelerate onto the drawn formation, then it is shown big center screen
            static Handle<AnimationChain> reel_animation_chain = AnimationSystem::Register({
                AnimationDesc {
                    .action = [&hex_state, &window_state](const f32 t) -> void {
                        const float2 screen_size { static_cast<f32>(window_state.screen_size.x), static_cast<f32>(window_state.screen_size.y) };
                        DrawRect(window_state, AABB { .point = float2 { 0.0F }, .size = screen_size }, colors::COLOR_BLACK.WithAlpha(0.55F));

                        constexpr f32 CARD_ADVANCE = CARD_SIZE + 18.0F;
                        constexpr f32 STRIP_HEIGHT = CARD_SIZE + 70.0F;
                        const float2 strip_center { screen_size.x * 0.5F, screen_size.y * 0.4F };
                        DrawRect(window_state, AABB::FromCenter(strip_center, float2 { screen_size.x, STRIP_HEIGHT }), colors::COLOR_BLACK.WithAlpha(0.85F));

                        const f32 scroll = EaseOutQuart(t) * CARD_ADVANCE * static_cast<f32>(HQ_DRAW_REEL_LANDING);

                        for (u32 i = 0; i < unit_formation_reel.size(); i++) {
                            const f32 card_x = strip_center.x + CARD_ADVANCE * static_cast<f32>(i) - scroll;
                            if (Abs(card_x - strip_center.x) > screen_size.x * 0.5F + CARD_SIZE) { continue; }
                            const UnitFormation& formation = hex_state.unit_formations[unit_formation_reel[i]];
                            const f32 proximity = Max(0.0F, 1.0F - Abs(card_x - strip_center.x) / CARD_ADVANCE);
                            DrawFormationCounter(window_state, formation, AABB::FromCenter(float2 { card_x, strip_center.y }, float2 { CARD_SIZE * (1.0F + 0.2F * proximity) }), hex_state.label_pool);
                        }

                        DrawRect(window_state, AABB::FromCenter(strip_center, float2 { 20.0F, STRIP_HEIGHT }), colors::COLOR_YELLOW.WithAlpha(0.25F));
                        DrawRect(window_state, AABB::FromCenter(strip_center, float2 { 4.0F, STRIP_HEIGHT }), colors::COLOR_YELLOW);
                    },
                    .duration = TURN_HQ_DRAW_DURATION,
                    .state = AnimationState::persistent_stopped },
                AnimationDesc {
                    .action = [&hex_state, &window_state](const f32 t) -> void {
                        const float2 screen_size { static_cast<f32>(window_state.screen_size.x), static_cast<f32>(window_state.screen_size.y) };
                        DrawRect(window_state, AABB { .point = float2 { 0.0F }, .size = screen_size }, colors::COLOR_BLACK.WithAlpha(0.55F));
                        const f32 t_pop = Min(t * static_cast<f32>(TURN_HQ_SHOW_DURATION.value) / 200.0F, 1.0F);
                        const f32 showcase_size = CARD_SIZE * (1.2F + 1.3F * EaseOutCubic(t_pop));
                        const AABB area_showcase = AABB::FromCenter(screen_size * float2 { 0.5F }, float2 { showcase_size });
                        DrawRect(window_state, area_showcase.WithPadding(float2 { -8.0F }), colors::COLOR_YELLOW);
                        DrawFormationCounter(window_state, hex_state.unit_formations[hex_state.unit_formation_active.GetHandle()], area_showcase, hex_state.label_pool);
                    },
                    .duration = TURN_HQ_SHOW_DURATION,
                    .state = AnimationState::persistent_stopped },
            });
            if (initalize) {
                const u32 drawn_index = Rand(hex_state.unit_formations_left.size());
                hex_state.unit_formation_active = hex_state.unit_formations_left[drawn_index];
                Logger().Log("[STATE] Turn HQ: Drawn {}", UnitNameToString(hex_state.unit_formations[hex_state.unit_formation_active.GetHandle()].name));

                unit_formation_reel.clear();
                for (u32 i = 0; i < HQ_DRAW_REEL_SIZE; i++) { unit_formation_reel.push_back(hex_state.unit_formations_left[Rand(hex_state.unit_formations_left.size())]); }
                unit_formation_reel[HQ_DRAW_REEL_LANDING] = hex_state.unit_formation_active.GetHandle();
                hex_state.unit_formations_left.swap_back(drawn_index);
                AnimationSystem::StartAnimation(reel_animation_chain);
            }
            return AnimationSystem::IsRunning(reel_animation_chain) ? TurnHqState::TURN_HQ_START : TurnHqState::TURN_HQ_ACTIVATE;
        }
        case TurnHqState::TURN_HQ_ACTIVATE: {
            return TurnHqState::TURN_HQ_LOGISTIC;
        }
        case TurnHqState::TURN_HQ_LOGISTIC: {
            static i8 activation_roll_mod;
            static i8 activation_modifiers;
            UnitFormation& unit_formation = hex_state.unit_formations[hex_state.unit_formation_active.GetHandle()];
            static Handle<Animation> snafu_roll_animation = AnimationSystem::Register(AnimationDesc {
                .action = [&hex_state, &window_state](const f32) -> void {
                    const FontCollection& font_collection = Singleton::Get<FontCollection>();
                    const float2 screen_size { static_cast<f32>(window_state.screen_size.x), static_cast<f32>(window_state.screen_size.y) };
                    constexpr f32 COUNTER_SHOWCASE_SIZE = 300.0F;
                    const AABB area_counter_showcase = AABB::FromCenter(float2 { screen_size.x * 0.5F, screen_size.y * 0.4F }, float2 { COUNTER_SHOWCASE_SIZE });
                    const AABB area_snafu_roll = AABB::FromPoint(float2 { 0.0F, area_counter_showcase.point.y + area_counter_showcase.size.y + 20.0F }, float2 { screen_size.x, static_cast<f32>(FontSizes::massive) * 2.0F });

                    DrawRect(window_state, AABB { .point = float2 { 0.0F }, .size = screen_size }, colors::COLOR_BLACK.WithAlpha(0.55F));
                    const f32 focus_top = area_counter_showcase.point.y - 35.0F;
                    const f32 focus_bottom = area_snafu_roll.point.y + area_snafu_roll.size.y + 35.0F;
                    DrawRect(window_state, AABB::FromPoint(float2 { 0.0F, focus_top }, float2 { screen_size.x, focus_bottom - focus_top }), colors::COLOR_BLACK.WithAlpha(0.85F));

                    const UnitFormation& unit_formation = hex_state.unit_formations[hex_state.unit_formation_active.GetHandle()];
                    DrawFormationCounter(window_state, unit_formation, area_counter_showcase, hex_state.label_pool);

                    const Font& font_snafu_roll = font_collection.GetFontBoldCourier(FontSizes::massive);
                    const Label& label_snafu_roll = hex_state.label_pool.Get();
                    label_snafu_roll.SetText(std::format("{}\n{} [{:+}]", unit_formation.activation_result, activation_roll_mod, activation_modifiers));
                    DrawText(font_snafu_roll, label_snafu_roll, area_snafu_roll.WithOffset(float2 { 3.0F }), colors::COLOR_BLACK, TextAlignment::CENTER);
                    DrawText(font_snafu_roll, label_snafu_roll, area_snafu_roll, colors::COLOR_WHITE, TextAlignment::CENTER);
                },
                .duration = TURN_HQ_SHOW_DURATION,
                .state = AnimationState::persistent_stopped });
            if (initalize) {
                activation_modifiers = - unit_formation.fatigue - unit_formation.prepared_defense;
                const i8 activation_roll = RandD6() + RandD6();
                activation_roll_mod = activation_roll + activation_modifiers;

                if (activation_roll_mod <= 2) {
                    unit_formation.activation_result = ActivationResult::ACTIVATE_NONE;
                }
                else if (activation_roll_mod <= 6) {
                    unit_formation.activation_result = ActivationResult::ACTIVATE_HALF;
                }
                else {
                    unit_formation.activation_result = ActivationResult::ACTIVATE_FULL;
                }

                switch (unit_formation.activation_result) {
                    case ActivationResult::ACTIVATE_NONE: {
                        if (unit_formation.fatigue > 0) {
                            unit_formation.fatigue--;
                        }
                        break;
                    }
                    case ActivationResult::ACTIVATE_HALF: {
                        unit_formation.artillery.current = unit_formation.artillery.max;
                        for (Unit& unit_member : hex_state.units_by_formation[hex_state.unit_formation_active.GetHandle()] | hex_state.units.handle_to_view()) { unit_member.move.current = Ceil(unit_member.move.max / 2.0F); }
                        break;
                    }
                    case ActivationResult::ACTIVATE_FULL: {
                        unit_formation.artillery.current = Ceil(unit_formation.artillery.max / 2.0F);
                        for (Unit& unit_member : hex_state.units_by_formation[hex_state.unit_formation_active.GetHandle()] | hex_state.units.handle_to_view()) { unit_member.move.current = unit_member.move.max; }
                        break;
                    }
                    default: std::unreachable();
                }
                AnimationSystem::StartAnimation(snafu_roll_animation);
            }
            if (AnimationSystem::IsRunning(snafu_roll_animation)) { return TurnHqState::TURN_HQ_LOGISTIC; }
            return unit_formation.activation_result == ActivationResult::ACTIVATE_NONE ? TurnHqState::TURN_HQ_END : TurnHqState::TURN_HQ_OBJ_PLACEMENT;
        }
        case TurnHqState::TURN_HQ_OBJ_PLACEMENT: {
            static u8 obj_markers_left;
            if (initalize) {
                const UnitFormation& unit_formation = hex_state.unit_formations[hex_state.unit_formation_active.GetHandle()];
                assert(unit_formation.activation_result != ActivationResult::ACTIVATE_NONE);
                obj_markers_left = unit_formation.activation_result == ActivationResult::ACTIVATE_FULL ? 2 : 1;
            }

            u32 hover_marker_count = 0;
            if (obj_markers_left > 0 && hex_state.pseudo_states.axial_hover.has_value()) {
                const int2 axial_hover = hex_state.pseudo_states.axial_hover.value();
                hover_marker_count = std::ranges::count(hex_state.objective_markers_axials, axial_hover);
                VertObjectiveMarkerAreaAppend(hex_state, camera, axial_hover, hover_marker_count > 0 ? colors::COLOR_ORANGE : colors::COLOR_YELLOW);
                if (input_state.left_mouse_down) {
                    hex_state.objective_markers_axials.push_back(axial_hover);
                    obj_markers_left--;
                }
            }
            (void)SDL_RenderGeometry(window_state.renderer, nullptr, hex_state.verts);
            hex_state.verts.clear();

            // hovering an existing marker upgrades it
            if (hover_marker_count > 0) {
                const float2 screen = camera.WorldToScreen(HexAxialToWorld(hex_state.pseudo_states.axial_hover.value()));
                const f32 pt = camera.scale * 0.35F;
                if (static_cast<FontSize>(pt) >= FONT_MIN_SIZE) {
                    const Font& font_upgrade = font_collection.GetFontBoldCompact(static_cast<FontSizes>(pt));
                    font_upgrade.SetWrapAlignment(TextAlignment::CENTER);
                    const Label& label_upgrade = hex_state.label_pool.Get();
                    label_upgrade.SetWrapWidth(camera.scale * 2.0F);
                    label_upgrade.SetFont(font_upgrade);
                    label_upgrade.SetColor(colors::COLOR_ORANGE);
                    label_upgrade.SetText(std::format("obj {:+}", hover_marker_count));
                    label_upgrade.Draw(float2 { screen.x - camera.scale, screen.y - camera.scale * 0.9F - pt });
                }
            }

            return obj_markers_left == 0 ? TurnHqState::TURN_HQ_EXECUTE : TurnHqState::TURN_HQ_OBJ_PLACEMENT;
        }
        case TurnHqState::TURN_HQ_EXECUTE: {
            return TurnHqState::TURN_HQ_CLEAN;
        }
        case TurnHqState::TURN_HQ_CLEAN: {
            return TurnHqState::TURN_HQ_ISOLATION;
        }
        case TurnHqState::TURN_HQ_ISOLATION: {
            return TurnHqState::TURN_HQ_END;
        }
        case TurnHqState::TURN_HQ_END: {
            hex_state.unit_formation_active.Reset();
            hex_state.objective_markers_axials.clear();

            if (hex_state.unit_formations_left.empty()) {
                return TurnHqState::TURN_HQ_NONE;
            }
            // hex_state
            return TurnHqState::TURN_HQ_START;
        }
        default: std::unreachable();
    }
}

[[nodiscard]] TurnState HexSystemTurnLogic (HexState& hex_state) {
    b8 initalize = hex_state.turn_state_changed;
    switch (hex_state.turn_state) {
        case TurnState::TURN_NONE: {
            return TurnState::TURN_START;
        }
        case TurnState::TURN_START: {
            if (initalize) {
                hex_state.turn_number++;
                assert(hex_state.unit_formations_left.empty());
                hex_state.unit_formations_left.clear();
                for (u32 i = 0; i < hex_state.unit_formations.size(); i++) { hex_state.unit_formations_left.push_back(hex_state.unit_formations.IndexToHandle(i)); }
            }
            return TurnState::TURN_REINFORCEMENT;
        }
        case TurnState::TURN_REINFORCEMENT: {
            // weather
            // air points
            // repl
            // rein
            return TurnState::TURN_ASSIGNMENT;
        }
        case TurnState::TURN_ASSIGNMENT: {
            // support
            return TurnState::TURN_HQ_ACTIVATE;
        }
        case TurnState::TURN_HQ_ACTIVATE: {
            if (TimeNowMS() - hex_state.turn_hq_state_time < TURN_STATE_DELAY) { return TurnState::TURN_HQ_ACTIVATE; }
            TurnHqState turn_hq_state_next = HexSystemTurnHqLogic(hex_state);
            if (turn_hq_state_next != hex_state.turn_hq_state) {
                Logger().Log("[STATE] Turn HQ: {} -> {}", hex_state.turn_hq_state, turn_hq_state_next);
                hex_state.turn_hq_state = turn_hq_state_next;
                hex_state.turn_hq_state_changed = true;
                hex_state.turn_hq_state_time = TimeNowMS();
            }
            else {
                hex_state.turn_hq_state_changed = false;
            }
            return hex_state.turn_hq_state == TurnHqState::TURN_HQ_NONE ? TurnState::TURN_END : TurnState::TURN_HQ_ACTIVATE;
        }
        case TurnState::TURN_END: {
            return TurnState::TURN_START;
        }
        default: std::unreachable();
    }
}
} // namespace hex

export namespace hex {
struct HexSystem {
    Handle<ParticleEmitter> particle_emitter { globalData.Create<ParticleEmitter>(ParticleEmitter { float2 { 0.0F, -60.0F } }) };
    Handle<Texture> table_texture { globalData.Create<Texture>(Asset("other/table.jpg")) };
    HandleOptional<Texture> marble_texture { globalData.Create<Texture>(Asset("other/marble.jpg")) };

    void operator()() const {
        const WindowState& window_state = Singleton::Get<WindowState>();
        const CameraState& camera = Singleton::Get<CameraState>();
        const InputState& input_state = Singleton::Get<InputState>();
        const FontCollection& font_collection = Singleton::Get<FontCollection>();
        HexState& hex_state = Singleton::Get<HexState>();

        // per frame compute
        hex_state.label_pool.Clear();
        hex_state.units_by_axial.clear();
        for (u32 i = 0; i < hex_state.units.size(); i++) {
            Handle<Unit> unit_handle = hex_state.units.IndexToHandle(i);
            hex_state.units_by_axial[hex_state.units[unit_handle].axial].EmplaceBack(unit_handle);
        }

        // pseudo states set
        const float2 mouse_world = camera.ScreenToWorld(input_state.mouse_position);
        if (hex_state.pseudo_states.unit_selection.has_value()) { hex_state.pseudo_states.axial_select = hex_state.units[hex_state.pseudo_states.unit_selection->unit_handles[0]].axial; } // update selection
        hex_state.pseudo_states.axial_hover = hex_state.hex_map.Contains(HexWorldToAxial(mouse_world)) ? Optional { HexWorldToAxial(mouse_world) } : std::nullopt;
        hex_state.player_tag = hex_state.pseudo_states.unit_selection.has_value() ? hex_state.units[hex_state.pseudo_states.unit_selection->unit_handles[0]].tag : CountryTag::TAG_GER;
        hex_state.player_action = PlayerAction::PLAYER_ACTION_NONE;
        if (hex_state.pseudo_states.unit_selection.has_value()) {
            const auto unit_selection = hex_state.pseudo_states.unit_selection->unit_handles | hex_state.units.handle_to_view();
            hex_state.pseudo_states.unit_selection->dmg_sum = std::ranges::fold_left(unit_selection | std::views::transform(&Unit::dmg), u32 { 0 }, std::plus { });
            hex_state.pseudo_states.unit_selection->dmg_ranged_sum = std::ranges::fold_left(unit_selection | std::views::transform(&Unit::dmg_ranged), u32 { 0 }, std::plus { });
            hex_state.pseudo_states.unit_selection->steps = std::ranges::fold_left(unit_selection | std::views::transform(&Unit::steps) | std::views::transform(&Stat::current), u32 { 0 }, std::plus { });
            const auto [move_min, move_max] = std::ranges::minmax(unit_selection | std::views::transform(&Unit::move) | std::views::transform(&Stat::current), std::less { });
            hex_state.pseudo_states.unit_selection->move_min = move_min;
            hex_state.pseudo_states.unit_selection->move_max = move_max;
        }


        // render table background, world-anchored so it pans and zooms with the map
        if constexpr (TABLE_THEME == TableStyle::TABLE_STYLE_TEXTURE) {
            constexpr f32 TABLE_MARGIN = 6.0F;
            const float2 screen_min = camera.WorldToScreen(camera.map_world_min - float2 { TABLE_MARGIN });
            const float2 screen_max = camera.WorldToScreen(camera.map_world_max + float2 { TABLE_MARGIN });
            DrawTexture(window_state, table_texture, AABB::FromPoint(screen_min, screen_max - screen_min), colors::COLOR_WHITE);
        }

        // render out-of-map filler hexes, a fixed border ring around the map
        {
            constexpr Color COLOR_OUT_OF_MAP = Color::FromHsl(42.0F, 0.05F, 0.74F);
            constexpr i32 FILLER_RING = 10;
            const int2 map_size = static_cast<int2>(hex_state.hex_map.map_size);
            for (i32 y = -FILLER_RING; y < map_size.y + FILLER_RING; y++) {
                for (i32 x = -FILLER_RING; x < map_size.x + FILLER_RING; x++) {
                    const int2 axial = HexOffsetToAxial({ x, y });
                    if (hex_state.hex_map.Contains(axial)) { continue; }
                    VertHexAppend(hex_state.verts, camera.scale * 0.97F, camera.WorldToScreen(HexAxialToWorld(axial)), COLOR_OUT_OF_MAP);
                }
            }
        }

        // render map
        for (u32 i = 0; i < hex_state.hex_map.Size(); i++) {
            const Hex& hex = hex_state.hex_map.data[i];
            const int2 axial = hex_state.hex_map.IndexToAxial(i);
            const float2 world = HexAxialToWorld(axial);
            Color color = TerrainToColorScheme(hex.terrain_type);
            VertHexAppend(hex_state.verts, camera.scale * 0.97F, camera.WorldToScreen(world), color);
            if (hex.owner.contested) { VertHexAppend(hex_state.verts, camera.scale * 0.70F, camera.WorldToScreen(world), color.Mul(0.80F)); }
        }

        AppendCountryBorders(hex_state, camera);
        AppendRiverMesh(hex_state, camera);
        AppendRoadMesh(hex_state, camera);
        AppendTerrainFeatures(hex_state, camera);

        // render pseudo states
        if (hex_state.pseudo_states.axial_hover) { VertHexRingAppend(hex_state.verts, camera.scale, camera.scale * 0.88F, camera.WorldToScreen(HexAxialToWorld(hex_state.pseudo_states.axial_hover.value())), colors::COLOR_HEX_HOVER); }
        if (hex_state.pseudo_states.unit_selection) {
            // draw formation
            const Handle<UnitFormation> formation = hex_state.units[hex_state.pseudo_states.unit_selection->unit_handles[0]].formation;
            for (const Unit& unit_member : hex_state.units_by_formation[formation] | hex_state.units.handle_to_view()) { VertHexRingAppend(hex_state.verts, camera.scale * 0.98F, camera.scale * 0.86F, camera.WorldToScreen(HexAxialToWorld(unit_member.axial)), hex_state.unit_formations[formation].color); }
        }
        if (hex_state.pseudo_states.axial_select) { VertHexRingAppend(hex_state.verts, camera.scale * 0.88F, camera.scale * 0.76F, camera.WorldToScreen(HexAxialToWorld(hex_state.pseudo_states.axial_select.value())), colors::COLOR_HEX_SELECT); }

        // render formation
        if (hex_state.pseudo_states.unit_selection) {
            auto draw_line = [&](const Color color, const int2 axial_a, const int2 axial_b) {
                const float2 screen_a = camera.WorldToScreen(HexAxialToWorld(axial_a));
                const float2 screen_b = camera.WorldToScreen(HexAxialToWorld(axial_b));
                VertObbAppend(hex_state.verts, OBB::BetweenPoints(screen_a, screen_b, camera.scale * 0.15F), colors::COLOR_BLACK.WithAlpha(0.7F));
                VertObbAppend(hex_state.verts, OBB::BetweenPoints(screen_a, screen_b, camera.scale * 0.1F), color);
            };
            for (const Handle<Unit>& unit_handle : hex_state.pseudo_states.unit_selection.value().unit_handles) {
                const Unit& unit = hex_state.units[unit_handle];
                for (const Unit& unit_member : hex_state.units_by_formation[unit.formation] | hex_state.units.handle_to_view()) {
                    if (unit_member.icon != UnitIcon::ICON_HQ || unit_member.axial == unit.axial) { continue; }
                    draw_line(colors::COLOR_RED, unit.axial, unit_member.axial);
                }
            }
        }

        // render objective markers
        for (const int2 axial_marker : hex_state.objective_markers_axials) {
            const b8 is_double = std::ranges::count(hex_state.objective_markers_axials, axial_marker) > 1;
            VertObjectiveMarkerAreaAppend(hex_state, camera, axial_marker, is_double ? colors::COLOR_ORANGE : colors::COLOR_YELLOW);
        }

        // render it all
        (void)SDL_RenderGeometry(window_state.renderer, nullptr, hex_state.verts);
        hex_state.verts.clear();

        // render units
        hex_state.counters.Clear();
        UnitToCounterAppend(hex_state);
        RenderCounters(hex_state.counters);

        // logic and render
        if (TimeNowMS() - hex_state.turn_state_time >= TURN_STATE_DELAY) {
            TurnState turn_state_next = HexSystemTurnLogic(hex_state);
            if (turn_state_next != hex_state.turn_state) {
                Logger().Log("[STATE] Turn: {} -> {}", hex_state.turn_state, turn_state_next);
                hex_state.turn_state = turn_state_next;
                hex_state.turn_state_changed = true;
                hex_state.turn_state_time = TimeNowMS();
            }
            else {
                hex_state.turn_state_changed = false;
            }
        }

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
                const u32 move_allowance = hex_state.pseudo_states.unit_selection->move_min;
                List<AxialAndCost> axial_path = HexAxialPathAStar(hex_state, axial_start, axial_hover, MoveType::MOVE_LEG, move_allowance);
                const List<AxialAndCost>::iterator it = std::ranges::upper_bound(axial_path, move_allowance, std::less { }, &AxialAndCost::cost);
                if (it != axial_path.begin()) {
                    for (List<AxialAndCost>::iterator step = axial_path.begin(); step != it; ++step) {
                        globalData[particle_emitter].particles.items.EmplaceBack(
                            Particle { .position = camera.WorldToScreen(HexAxialToWorld(step->axial)), .text = Label { font_collection.GetFontBoldCourier(FontSizes::h5), MoveTypeToString(MoveTypeUnitIcon(units_selected[0].icon)) }, .duration = miliseconds32 { 1000U } });
                        Hex& hex_center = hex_state.hex_map[step->axial];
                        if (hex_center.owner.tag != hex_state.player_tag) { hex_center.owner = HexOwner { .tag = hex_state.player_tag, .contested = true }; } // conquer
                        // zone of control
                        for (const int2 offset : HEX_AXIAL_NEIGHBOURS) {
                            const int2 axial_neighbour = step->axial + offset;
                            if (!hex_state.hex_map.Contains(axial_neighbour)) { continue; }
                            if (!AxialIsEnemy(hex_state, axial_neighbour) && !AxialIsEnemyZoc(hex_state, axial_neighbour)) {
                                Hex& hex_neighbour = hex_state.hex_map[axial_neighbour];
                                if (hex_neighbour.owner.tag != hex_state.player_tag) { hex_neighbour.owner = HexOwner { .tag = hex_state.player_tag, .contested = true }; } // conquer
                            }
                        }
                    }

                    const AxialAndCost& cost_and_axial_end = *std::prev(it);
                    for (Unit& unit : units_selected) {
                        unit.axial = cost_and_axial_end.axial;
                        unit.move.current -= cost_and_axial_end.cost;
                    }
                    hex_state.pseudo_states.axial_select = cost_and_axial_end.axial;
                }
                break;
            }
            case PlayerAction::PLAYER_ACTION_MOVE_HOVER: {
                auto units_selected = hex_state.pseudo_states.unit_selection->unit_handles | hex_state.units.handle_to_view();
                const u32 units_selected_movement = std::ranges::min(units_selected | std::views::transform(&Unit::move) | std::views::transform(&Stat::current));
                const List<AxialAndCost> axial_path = HexAxialPathAStar(hex_state, hex_state.pseudo_states.axial_select.value(), hex_state.pseudo_states.axial_hover.value(), MoveType::MOVE_LEG, units_selected_movement);
                for (const AxialAndCost cost_and_axial : axial_path) {
                    const float2 world = HexAxialToWorld(cost_and_axial.axial);
                    const float2 screen = camera.WorldToScreen(world);

                    const Color movement_color = cost_and_axial.cost > units_selected_movement ? colors::COLOR_BLACK : colors::COLOR_RUBY_RED;
                    VertHexAppend(hex_state.verts, camera.scale * 0.25F, screen, movement_color);
                }
                (void)SDL_RenderGeometry(window_state.renderer, nullptr, hex_state.verts);
                hex_state.verts.clear();

                const f32 pt = camera.scale * 0.3F;
                if (static_cast<FontSize>(pt) < FONT_MIN_SIZE) { break; }
                const Font& font_movement = font_collection.GetFontBoldCompact(static_cast<FontSizes>(pt));
                font_movement.SetWrapAlignment(TextAlignment::CENTER);
                for (AxialAndCost cost_and_axial : axial_path) {
                    const float2 world = HexAxialToWorld(cost_and_axial.axial);
                    const float2 screen = camera.WorldToScreen(world);

                    const Label& label = hex_state.label_pool.Get();
                    const String string_distance = std::format("{}", cost_and_axial.cost);
                    label.SetWrapWidth(camera.scale);
                    label.SetFont(font_movement);
                    label.SetColor(colors::COLOR_WHITE);
                    label.SetText(string_distance);
                    label.Draw(float2 { screen.x - camera.scale * 0.5F, screen.y - pt * 0.5F });
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

                struct BattleOutcome {
                    DefenderRetreat defender_retreat;
                    u8 attacker_step_loss;
                    u8 defender_step_loss;
                } battle_outcome;

                int2 axial_battle = units_defender[0].axial;
                Hex& hex_battle = hex_state.hex_map[axial_battle];
                Unit& unit_attacker = units_attacker[0];
                Unit& unit_defender = units_defender[0];
                UnitFormation& formation_attacker = hex_state.unit_formations[unit_attacker.formation];
                UnitFormation& formation_defender = hex_state.unit_formations[unit_defender.formation];
                u8 dmg_attacker = unit_attacker.dmg + std::ranges::contains(formation_attacker.support | hex_state.units.handle_to_view(), RangedType::RANGED_ATTACK, &Unit::ranged_type) - formation_attacker.prepared_defense + (unit_attacker.ranged_type == RangedType::RANGED_ATTACK);
                u8 dmg_defender = unit_defender.dmg + !std::ranges::contains(formation_defender.support | hex_state.units.handle_to_view(), RangedType::RANGED_NONE, &Unit::ranged_type) + (units_defender.size() > 2 ? -2 : units_defender.size() == 2) + (hex_battle.terrain_type != TerrainType::TERRAIN_TYPE_GRASS) +
                                  (unit_defender.ranged_type == RangedType::RANGED_ATTACK) * (hex_battle.terrain_feature == TerrainFeature::TERRAIN_FEATURE_CITY ? -3 : (hex_battle.terrain_type == TerrainType::TERRAIN_TYPE_GRASS) * 2) +
                                  hex_battle.river_edges.Test(HexAxialEdgeNeighbor(axial_battle, units_attacker[0].axial));
                i8 diff = dmg_attacker - dmg_defender;
                i8 roll = RandD6() + RandD6();
                i8 roll_mod = roll + diff;

                // 50 / 50 whether they retreat. grouped in 2 after
                if (roll_mod <= 4) {
                    battle_outcome.defender_retreat = DefenderRetreat::DEFENDER_HOLDS;
                    battle_outcome.attacker_step_loss = 2;
                } else if (roll_mod <= 6) {
                    battle_outcome.defender_retreat = DefenderRetreat::DEFENDER_HOLDS;
                    battle_outcome.attacker_step_loss = 1;
                } else if (roll_mod <= 8) {
                    battle_outcome.defender_retreat = DefenderRetreat::DEFENDER_RETREAT;
                    battle_outcome.attacker_step_loss = 1;
                } else if (roll_mod <= 10) {
                    battle_outcome.defender_retreat = DefenderRetreat::DEFENDER_RETREAT;
                    battle_outcome.attacker_step_loss = 1;
                } else if (roll_mod <= 12) {
                    battle_outcome.defender_retreat = DefenderRetreat::DEFENDER_ROUT;
                    battle_outcome.defender_step_loss = 1;
                } else {
                    battle_outcome.defender_retreat = DefenderRetreat::DEFENDER_ROUT;
                    battle_outcome.defender_step_loss = 2;
                }

                unit_attacker.steps.current = SaturatingSub(unit_attacker.steps.current, battle_outcome.attacker_step_loss);
                unit_defender.steps.current = SaturatingSub(unit_defender.steps.current, battle_outcome.defender_step_loss);
                unit_attacker.move.current = SaturatingSub(unit_attacker.move.current, MOVE_COST_ATTACK);

                globalData[particle_emitter].particles.items.EmplaceBack(
                    Particle { .position = camera.WorldToScreen(HexAxialToWorld(axial_battle)),
                               .text = Label { font_collection.GetFontBoldCourier(FontSizes::h1), std::format("{} [{:+}]\nA{} D{}\n{}", roll_mod, dmg_attacker - dmg_defender, battle_outcome.attacker_step_loss, battle_outcome.defender_step_loss, battle_outcome.defender_retreat) },
                               .duration = miliseconds32 { 1500U } });

                switch (battle_outcome.defender_retreat) {
                    case DefenderRetreat::DEFENDER_HOLDS: break;
                    case DefenderRetreat::DEFENDER_RETREAT:
                        for (Unit& defender : units_defender) {
                            const int2 axial_retreat = defender.tag == CountryTag::TAG_GER ? int2 { -3, 0 } : int2 { 3, 0 };
                            defender.axial += axial_retreat;
                        }
                        break;
                    case DefenderRetreat::DEFENDER_ROUT: {
                        for (Unit& defender : units_defender) {
                            const int2 axial_retreat = defender.tag == CountryTag::TAG_GER ? int2 { -6, 0 } : int2 { 6, 0 };
                            defender.axial += axial_retreat;
                        }
                    }
                }

                if (battle_outcome.defender_retreat != DefenderRetreat::DEFENDER_HOLDS) {
                    if (hex_state.hex_map[axial_battle].owner.tag != hex_state.player_tag) { hex_state.hex_map[axial_battle].owner = HexOwner { .tag = hex_state.player_tag, .contested = true }; } // conquer
                    units_attacker[0].axial = axial_battle;
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

                const Color color_inner = can_attack ? colors::COLOR_RUBY_RED : colors::COLOR_BLACK;
                VertHexAppend(hex_state.verts, camera.scale * 0.25F, screen, colors::COLOR_GRAY);
                VertHexAppend(hex_state.verts, camera.scale * 0.20F, screen, color_inner);

                if (can_attack) { VertHexAppend(hex_state.verts, camera.scale * 0.10F, screen, colors::COLOR_DARK_GREEN); }
                (void)SDL_RenderGeometry(window_state.renderer, nullptr, hex_state.verts);
                hex_state.verts.clear();

                const f32 pt = camera.scale * 0.2F;
                if (static_cast<FontSize>(pt) < FONT_MIN_SIZE) { break; }
                const Font& font_attack = font_collection.GetFontBoldCompact(static_cast<FontSizes>(pt));
                font_attack.SetWrapAlignment(TextAlignment::CENTER);
                const Label& label = hex_state.label_pool.Get();
                label.SetWrapWidth(camera.scale);
                label.SetFont(font_attack);
                label.SetColor(colors::COLOR_WHITE);

                if (can_attack) {
                    auto units_attacker = hex_state.pseudo_states.unit_selection->unit_handles | hex_state.units.handle_to_view();
                    auto units_defender = hex_state.units_by_axial[hex_state.pseudo_states.axial_hover.value()] | hex_state.units.handle_to_view();
                    const i32 dmg = hex_state.pseudo_states.unit_selection->dmg_sum;
                    const i32 def = std::ranges::fold_left(units_defender | std::views::transform(&Unit::dmg), 0U, std::plus { });

                    label.SetText(std::format("{:+}", dmg - def));
                } else if (attacker_is_hq) {
                    label.SetText("hq cannot attack");
                } else if (!within_range) {
                    label.SetText("can only attack adjecent units");
                } else if (!attacker_has_movement) {
                    label.SetText(std::format("no movement\n{} < {}", hex_state.pseudo_states.unit_selection->move_min, MOVE_COST_ATTACK));
                }
                label.Draw(float2 { screen.x - camera.scale * 0.5F, screen.y - pt * 0.5F });

                break;
        }

        // turn status ui, bottom right, with the active formation counter above it
        {
            const Font& font_status = font_collection.GetFontNormalCourier(FontSizes::body);
            constexpr f32 STATUS_CELL_WIDTH = 200.0F;
            constexpr f32 STATUS_LINE_HEIGHT = 20.0F;
            constexpr f32 STATUS_PADDING = 10.0F;
            const AABB area_status = AABB::FromPoint(float2 { static_cast<f32>(window_state.screen_size.x) - STATUS_CELL_WIDTH * 2.0F - STATUS_PADDING * 2.0F, static_cast<f32>(window_state.screen_size.y) - STATUS_LINE_HEIGHT - STATUS_PADDING * 2.0F }, float2 { STATUS_CELL_WIDTH * 2.0F + STATUS_PADDING * 2.0F, STATUS_LINE_HEIGHT + STATUS_PADDING * 2.0F });
            const auto cell_area = [&](const u32 cell) -> AABB { return AABB::FromPoint(area_status.point + float2 { STATUS_PADDING + STATUS_CELL_WIDTH * static_cast<f32>(cell), STATUS_PADDING }, float2 { STATUS_CELL_WIDTH, STATUS_LINE_HEIGHT }); };
            DrawRect(window_state, area_status, CountryBranchToColor(CountryTag::TAG_GER, UnitBranch::BRANCH_INFANTRY));

            const Label& label_turn_number = hex_state.label_pool.Get();
            label_turn_number.SetText(std::format("Turn {}", hex_state.turn_number));
            DrawText(font_status, label_turn_number, cell_area(0), colors::COLOR_BLACK, TextAlignment::LEFT);

            const Label& label_turn_state = hex_state.label_pool.Get();
            label_turn_state.SetText(std::format("{}", hex_state.turn_state));
            DrawText(font_status, label_turn_state, cell_area(1), colors::COLOR_BLACK, TextAlignment::LEFT);
        }

        // debug
        if (hex_state.pseudo_states.axial_hover.has_value() && input_state.keys.at(SDLK_LALT) && input_state.left_mouse_down) {
            const int2 axial = hex_state.pseudo_states.axial_hover.value();
            const int2 offset = HexAxialToOffset(axial);
            Logger().Log("[Hex] Offset ({},{}) Axial ({},{})", offset.x, offset.y, axial.x, axial.y);
        }
    }
};
} // namespace hex
