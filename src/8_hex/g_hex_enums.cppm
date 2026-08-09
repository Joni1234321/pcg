module;

export module hex.enums;

import std;

import pce.collections;
import pce.std;
import pce.strong;

export namespace hex {
using namespace hex;
enum class CountryTag : u8 { TAG_NONE, TAG_GER, TAG_SOV, TAG_USA };
enum class TerrainType : u8 { TERRAIN_TYPE_DEEP_OCEAN, TERRAIN_TYPE_OCEAN, TERRAIN_TYPE_HILL, TERRAIN_TYPE_BEACH, TERRAIN_TYPE_GRASS, TERRAIN_TYPE_MOUNTAIN, TERRAIN_TYPE_SNOW };
enum class TerrainFeature : u8 { TERRAIN_FEATURE_GRASSLAND, TERRAIN_FEATURE_FIELD, TERRAIN_FEATURE_CITY, TERRAIN_FEATURE_VILLAGE, TERRAIN_FEATURE_WOODED_LIGHTLY, TERRAIN_FEATURE_WOODED_HEAVY, TERRAIN_FEATURE_MARSH };

enum class Echelon : u8 { ECHELON_SQUAD, ECHELON_PLATOON, ECHELON_COMPANY, ECHELON_BATTALION, ECHELON_REGIMENT, ECHELON_BRIGADE, ECHELON_DIVISION, ECHELON_CORPS, ECHELON_ARMY };
enum class UnitIcon : u8 { ICON_INF, ICON_ART, ICON_HQ, ICON_TANK, ICON_ENGINEER };
enum class UnitBranch : u8 { BRANCH_INFANTRY, BRANCH_ARMOR, BRANCH_GUARD };
enum class RoadLevel : u8 { ROAD_LEVEL_NONE, ROAD_LEVEL_TRACK, ROAD_LEVEL_SECONDARY, ROAD_LEVEL_PRIMARY };
enum class MoveType : u8 { MOVE_LEG, MOVE_TAC, MOVE_TRUCK };
enum class RangedType : u8 { RANGED_NONE, RANGED_DEFENSE, RANGED_ATTACK };

enum class PlayerAction : u8 { PLAYER_ACTION_NONE, PLAYER_ACTION_SELECT, PLAYER_ACTION_DESELECT, PLAYER_ACTION_MOVE_CLICK, PLAYER_ACTION_MOVE_HOVER, PLAYER_ACTION_ATTACK_CLICK, PLAYER_ACTION_ATTACK_HOVER };
enum class TurnState : u8 { TURN_NONE, TURN_START, TURN_REINFORCEMENT, TURN_ASSIGNMENT, TURN_HQ_ACTIVATE, TURN_END };
enum class TurnHqState : u8 { TURN_HQ_NONE, TURN_HQ_START, TURN_HQ_ACTIVATE, TURN_HQ_LOGISTIC, TURN_HQ_OBJ_PLACEMENT, TURN_HQ_EXECUTE, TURN_HQ_CLEAN, TURN_HQ_ISOLATION, TURN_HQ_END };
enum class ExecutePhase : u8 { EXECUTE_IDLE, EXECUTE_MOVE, EXECUTE_ATTACK };
enum class DefenderRetreat : u8 { DEFENDER_HOLDS, DEFENDER_RETREAT, DEFENDER_ROUT };
enum class ActivationResult : u8 { ACTIVATE_NONE, ACTIVATE_HALF, ACTIVATE_FULL };

enum class CounterStyle : u8 { COUNTER_STYLE_NIEHORSTER, COUNTER_STYLE_NIEHORSTER_BIG, COUNTER_STYLE_REAL };
enum class MapStyle : u8 { CIV_VIBRANT, SLATE_TABLE, HOI4_PAPER, FADED_LINEN };
enum class TerrainStyle : u8 { TERRAIN_STYLE_SILHOUETTES, TERRAIN_STYLE_ICONS };
enum class TableStyle : u8 { TABLE_STYLE_TEXTURE, TABLE_STYLE_SEA_GREEN, TABLE_STYLE_LINEN };

constexpr MapStyle TERRAIN_SCHEME = MapStyle::FADED_LINEN;
constexpr TableStyle TABLE_THEME = TableStyle::TABLE_STYLE_LINEN;
constexpr TerrainStyle TERRAIN_FEATURE_THEME = TerrainStyle::TERRAIN_STYLE_ICONS;
constexpr CounterStyle COUNTER_THEME = CounterStyle::COUNTER_STYLE_NIEHORSTER;

constexpr miliseconds32 TURN_STATE_DELAY { 100U };
constexpr miliseconds32 TURN_HQ_DRAW_DURATION { 200U };
constexpr miliseconds32 TURN_HQ_SHOW_DURATION { 1500U };
constexpr u32 HQ_DRAW_REEL_LANDING = 24U;
constexpr u32 HQ_DRAW_REEL_SIZE = HQ_DRAW_REEL_LANDING + 21U; // 20 counters visible past the landing so the wheel never looks empty

constexpr u8 MOVE_POINT = 15U;
constexpr u8 MOVE_COST_ATTACK = 1U;
constexpr u8 MOVE_COST_ATTACK_PLANNED = 2U;
constexpr u8 MOVE_COST_STOP = 100U;       // enter, then movement ends
constexpr u8 MOVE_COST_PROHIBITED = 200U; // stays above STOP + any hexside cost

constexpr f32 BORDER_INNER_RADIUS = 0.90F;
constexpr f32 BORDER_TEETH_DEPTH = 0.12F;
constexpr f32 BORDER_TEETH_HALF = 0.18F;
constexpr f32 ROAD_WIDTH = 0.035F;
constexpr f32 ROAD_BIG_WIDTH = 0.075F;
constexpr f32 ROAD_CENTER_JITTER = 0.2F;
constexpr f32 FEATURE_POSITION_JITTER = 0.2F;

constexpr f32 RIVER_WIDTH = 0.13F;
constexpr f32 RIVER_CASING_EXTRA = 0.05F;
constexpr f32 RIVER_HIGHLIGHT_WIDTH = RIVER_WIDTH * 0.35F;

struct MoveCost {
    u8 leg { };
    u8 tac { };
    u8 truck { };
};
constexpr MoveCost MOVE_COST_RIVER { .leg = 2U, .tac = 6U, .truck = 8U };
} // namespace hex

template <> struct std::formatter<hex::TurnState> : std::formatter<hex::String> {
    auto format(const hex::TurnState turn_state, std::format_context& ctx) const {
        hex::String name;
        switch (turn_state) {
            case hex::TurnState::TURN_NONE: name = "TURN_NONE"; break;
            case hex::TurnState::TURN_START: name = "TURN_START"; break;
            case hex::TurnState::TURN_REINFORCEMENT: name = "TURN_REINFORCEMENT"; break;
            case hex::TurnState::TURN_ASSIGNMENT: name = "TURN_ASSIGNMENT"; break;
            case hex::TurnState::TURN_HQ_ACTIVATE: name = "TURN_HQ_ACTIVATE"; break;
            case hex::TurnState::TURN_END: name = "TURN_END"; break;
        }
        return std::formatter<hex::String>::format(name, ctx);
    }
};
template <> struct std::formatter<hex::TurnHqState> : std::formatter<hex::String> {
    auto format(const hex::TurnHqState turn_hq_state, std::format_context& ctx) const {
        hex::String name;
        switch (turn_hq_state) {
            case hex::TurnHqState::TURN_HQ_NONE: name = "None"; break;
            case hex::TurnHqState::TURN_HQ_START: name = "Formation Draw"; break;
            case hex::TurnHqState::TURN_HQ_ACTIVATE: name = "Activation"; break;
            case hex::TurnHqState::TURN_HQ_LOGISTIC: name = "Logistics"; break;
            case hex::TurnHqState::TURN_HQ_OBJ_PLACEMENT: name = "Objective Placement"; break;
            case hex::TurnHqState::TURN_HQ_EXECUTE: name = "Execution"; break;
            case hex::TurnHqState::TURN_HQ_CLEAN: name = "Clean Up"; break;
            case hex::TurnHqState::TURN_HQ_ISOLATION: name = "Isolation"; break;
            case hex::TurnHqState::TURN_HQ_END: name = "End"; break;
        }
        return std::formatter<hex::String>::format(name, ctx);
    }
};
template <> struct std::formatter<hex::DefenderRetreat> : std::formatter<hex::String> {
    auto format(const hex::DefenderRetreat defender_retreat, std::format_context& ctx) const {
        hex::String name;
        switch (defender_retreat) {
            case hex::DefenderRetreat::DEFENDER_HOLDS: name = "DEFENDER_HOLDS"; break;
            case hex::DefenderRetreat::DEFENDER_RETREAT: name = "DEFENDER_RETREAT"; break;
            case hex::DefenderRetreat::DEFENDER_ROUT: name = "DEFENDER_ROUT"; break;
        }
        return std::formatter<hex::String>::format(name, ctx);
    }
};
template <> struct std::formatter<hex::ActivationResult> : std::formatter<hex::String> {
    auto format(const hex::ActivationResult activation_result, std::format_context& ctx) const {
        hex::String name;
        switch (activation_result) {
            case hex::ActivationResult::ACTIVATE_NONE: name = "NONE"; break;
            case hex::ActivationResult::ACTIVATE_HALF: name = "HALF"; break;
            case hex::ActivationResult::ACTIVATE_FULL: name = "FULL"; break;
        }
        return std::formatter<hex::String>::format(name, ctx);
    }
};
