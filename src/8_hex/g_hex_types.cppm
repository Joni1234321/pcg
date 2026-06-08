module;

export module hex.types;

import std;

import pce.sdl;
import pce.collections;
import pce.std;

import hex.hex;

export namespace hex {
using namespace hex;
enum class CountryTag : u8 { TAG_NONE, TAG_GER, TAG_SOV, TAG_USA };
enum class TerrainType : u8 { TERRAIN_TYPE_DEEP_OCEAN, TERRAIN_TYPE_OCEAN, TERRAIN_TYPE_HILL, TERRAIN_TYPE_BEACH, TERRAIN_TYPE_GRASS, TERRAIN_TYPE_MOUNTAIN, TERRAIN_TYPE_SNOW };
enum class TerrainFeature : u8 { TERRAIN_FEATURE_GRASSLAND, TERRAIN_FEATURE_FIELD, TERRAIN_FEATURE_CITY, TERRAIN_FEATURE_VILLAGE, TERRAIN_FEATURE_WOODED_LIGHTLY, TERRAIN_FEATURE_WOODED_HEAVY, TERRAIN_FEATURE_MARSH };
enum class PlayerAction : u8 { PLAYER_ACTION_NONE, PLAYER_ACTION_SELECT, PLAYER_ACTION_DESELECT, PLAYER_ACTION_MOVE_CLICK, PLAYER_ACTION_MOVE_HOVER, PLAYER_ACTION_ATTACK_CLICK, PLAYER_ACTION_ATTACK_HOVER };
enum class Echelon : u8 { ECHELON_SQUAD, ECHELON_PLATOON, ECHELON_COMPANY, ECHELON_BATTALION, ECHELON_REGIMENT, ECHELON_BRIGADE, ECHELON_DIVISION, ECHELON_CORPS, ECHELON_ARMY };
enum class UnitIcon : u8 { ICON_INF, ICON_ART, ICON_HQ, ICON_TANK };
enum class MapStyle : u8 { CIV_VIBRANT, SLATE_TABLE, HOI4_PAPER };
enum class TerrainStyle : u8 { TERRAIN_STYLE_SILHOUETTES, TERRAIN_STYLE_ICONS };
enum class RoadLevel : u8 { ROAD_LEVEL_NONE, ROAD_LEVEL_SMALL, ROAD_LEVEL_MEDIUM, ROAD_LEVEL_LARGE };
constexpr MapStyle TERRAIN_SCHEME = MapStyle::SLATE_TABLE;
constexpr TerrainStyle TERRAIN_FEATURE_THEME = TerrainStyle::TERRAIN_STYLE_ICONS;

constexpr u32 MOVE_COST_ATTACK = 2U;
constexpr u32 MOVE_COST_ATTACK_PLANNED = 6U;
constexpr u32 MOVE_COST_ROAD_REDUCTION = 2U;
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

struct Counter {
    Color color_background{};
    Color color_icon{};
    Color color_border{};
};
struct CounterStack {
    int2 axial { };
    UnitIcon icon { };
    Array<Counter, 12> stack { };
    Label label_top{};
    Label label_center{};
    Label label_bottom{};
    SurfaceLabel label_vertical{};
};

struct HexOwner {
    CountryTag tag { CountryTag::TAG_NONE };
    b8 contested { false };
};
struct Hex {
    TerrainType terrain_type { };
    TerrainFeature terrain_feature { };
    HexOwner owner { };
    HexBitset2 roads { };
    HexBitset river_edges { };
};
struct Unit;
struct UnitFormation {
    HandleOptional<Unit> parent { };
    CountryTag tag {};
    UnitIcon icon {};
    Echelon echelon {};
};
struct UnitToe {
    u32 move {};
    u32 squad_inf {};
    u32 squad_art {};
    u32 squad_tank {};
    [[nodiscard]] constexpr u32 dmg() const { return squad_inf / 3 + squad_art * 2 + squad_tank / 5 * 8; }
    [[nodiscard]] constexpr u32 def() const { return squad_inf / 3 * 2 + squad_art + squad_tank / 5; }
};
using UnitName = Array<char, 10>;
struct UnitFlavor {
    UnitName name { };
    Color color { };
};
struct Unit {
    int2 axial { };

    // unit formation
    HandleOptional<Unit> parent { };
    CountryTag tag {};
    UnitIcon icon {};
    Echelon echelon {};

    // unit stats
    u32 move {};
    u32 squad_inf {};
    u32 squad_art {};
    u32 squad_tank {};
    [[nodiscard]] constexpr u32 dmg() const { return squad_inf / 3 + squad_art * 2 + squad_tank / 5 * 8; }
    [[nodiscard]] constexpr u32 def() const { return squad_inf / 3 * 2 + squad_art + squad_tank / 5; }

    // unit flavour
    UnitName name { };
    Color color { };
};
struct UnitGroup {
    List<Handle<Unit>> unit_handles{};
    u32 dmg_sum{};
    u32 def_sum{};
    u32 move_min{};
    u32 move_max{};
};
struct PseudoTarget {
    int2 axial { };
    List<Handle<Unit>> units{};
};
struct PseudoStates {
    Optional<int2> axial_hover{};
    Optional<int2> axial_select{};
    Optional<UnitGroup> unit_selection{};
};
struct AxialAndCost {
    int2 axial{};
    u32 cost{};
};
struct AxialAndEdge {
    int2 axial{};
    u8 edge {};
    b8 operator==(const AxialAndEdge& other) const = default;
};
struct HexState {
    HexList<Hex> hex_map{};
    HandleList<Unit> units{};

    CountryTag player_tag;
    PlayerAction player_action;
    PseudoStates pseudo_states;
    UnorderedMap<int2, List<Handle<Unit>>> units_by_axial;

    UnorderedMap<Handle<Unit>, List<Handle<Unit>>> units_oob;

    Pool<CounterStack> counters{};
    Pool<Label> label_pool{};
    List<Vertex> verts { };
};
} // namespace pcg

export template <> struct std::hash<hex::AxialAndEdge> {
    usize operator()(const hex::AxialAndEdge& axial_and_edge) const noexcept {
        usize seed = std::hash<int2> { }(axial_and_edge.axial);
        HashCombine(seed, axial_and_edge.edge);
        return seed;
    }
};