#pragma once
#include <assert.h>

#include "0_engine/u_collections.hpp"
#include "0_engine/u_types.hpp"
#include "8_hex/u_counter.hpp"
#include "8_hex/u_hex.hpp"

namespace pcg {
using namespace pce;
enum class CountryTag : u8 { TAG_NONE, TAG_GER, TAG_SOV, TAG_USA };
enum class TerrainType : u8 { TERRAIN_TYPE_DEEP_OCEAN, TERRAIN_TYPE_OCEAN, TERRAIN_TYPE_HILL, TERRAIN_TYPE_BEACH, TERRAIN_TYPE_GRASS, TERRAIN_TYPE_MOUNTAIN, TERRAIN_TYPE_SNOW };
enum class TerrainFeature : u8 { TERRAIN_FEATURE_GRASSLAND, TERRAIN_FEATURE_FIELD, TERRAIN_FEATURE_CITY, TERRAIN_FEATURE_VILLAGE, TERRAIN_FEATURE_WOODED_LIGHTLY, TERRAIN_FEATURE_WOODED_HEAVY, TERRAIN_FEATURE_MARSH };
enum class PlayerAction : u8 { PLAYER_ACTION_NONE, PLAYER_ACTION_SELECT, PLAYER_ACTION_DESELECT, PLAYER_ACTION_MOVE_CLICK, PLAYER_ACTION_MOVE_HOVER, PLAYER_ACTION_ATTACK_CLICK, PLAYER_ACTION_ATTACK_HOVER };
enum class MapStyle : u8 { CIV_VIBRANT, SLATE_TABLE, HOI4_PAPER }; // colorschema.md
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

struct HexBitset {
    u8 value;
    [[nodiscard]] constexpr b8 None() const { return !value; }
    [[nodiscard]] constexpr b8 Any() const { return value; }
    [[nodiscard]] constexpr b8 Test(const u8 pos) const {
        assert(pos < HEX_CORNERS);
        return value & 0x1 << pos;
    }
    constexpr void Clear() { value = 0U; }
    constexpr void Clear(const u8 pos) {
        assert(pos < HEX_CORNERS);
        value &= ~(0x1 << pos);
    }
    constexpr void Set() { value = 0x3F; }
    constexpr void Set(const u8 pos) {
        assert(pos < HEX_CORNERS);
        value |= 0x1 << pos;
    }
};
struct HexBitset2 {
    static constexpr u8 BITS_PER_HEX = 2;
    static constexpr u16 MASK = 0b11;
    u16 value;
    [[nodiscard]] constexpr b8 None() const { return !value; }
    [[nodiscard]] constexpr b8 Any() const { return value; }
    [[nodiscard]] constexpr u8 Test(const u8 pos) const {
        assert(pos < HEX_CORNERS);
        const u16 shift = pos * BITS_PER_HEX;
        return value >> shift & MASK;
    }
    constexpr void Clear() { value = 0U; }
    constexpr void Clear(const u8 pos) {
        assert(pos < HEX_CORNERS);
        const u16 shift = pos * BITS_PER_HEX;
        value &= ~(MASK << shift);
    }
    constexpr void Set(const u8 pos, const u8 val) {
        assert(pos < HEX_CORNERS);
        assert(val <= MASK);
        const u16 shift = pos * BITS_PER_HEX;
        value &= ~(MASK << shift);
        value |= val << shift;
    }
};

// efficient 30 bits used. 2 unused. 2^5 = 32 values per side
struct HexBitset5 {
    static constexpr u8 BITS_PER_HEX = 5;
    static constexpr u32 MASK = 0b11111;
    u32 value;
    [[nodiscard]] constexpr b8 None() const { return !value; }
    [[nodiscard]] constexpr b8 Any() const { return value; }
    [[nodiscard]] constexpr u8 Test(const u8 pos) const {
        assert(pos < HEX_CORNERS);
        const u32 shift = pos * BITS_PER_HEX;
        return value >> shift & MASK;
    }
    constexpr void Clear() { value = 0U; }
    constexpr void Clear(const u8 pos) {
        assert(pos < HEX_CORNERS);
        const u32 shift = pos * BITS_PER_HEX;
        value &= ~(MASK << shift);
    }
    constexpr void Set(const u8 pos, const u8 val) {
        assert(pos < HEX_CORNERS);
        assert(val <= MASK);
        const u32 shift = pos * BITS_PER_HEX;
        value &= ~(MASK << shift);
        value |= val << shift;
    }
};

struct HexOwner {
    CountryTag tag { CountryTag::TAG_NONE };
    b8 contested { false };
};
struct Hex {
    TerrainType terrain_type;
    TerrainFeature terrain_feature;
    HexOwner owner;
    HexBitset2 roads { };
    HexBitset river_edges { };
};
struct Unit {
    HandleOptional<Unit> parent;
    Array<char, 10> name { };
    CountryTag tag;
    Echelon echelon;
    UnitIcon icon;
    Color color;
    int2 axial;
    u32 move;
    u32 squad_inf;
    u32 squad_tank;
    u32 squad_art;

    [[nodiscard]] constexpr u32 dmg() const { return squad_inf / 3 + squad_art * 2 + squad_tank / 5 * 8; }
    [[nodiscard]] constexpr u32 def() const { return squad_inf / 3 * 2 + squad_art + squad_tank / 5; }
};
struct UnitGroup {
    List<Handle<Unit>> unit_handles;
    u32 dmg_sum;
    u32 def_sum;
    u32 move_min;
    u32 move_max;
};
struct PseudoTarget {
    int2 axial { };
    List<Handle<Unit>> units;
};
struct PseudoStates {
    Optional<int2> axial_hover;
    Optional<int2> axial_select;
    Optional<UnitGroup> unit_selection;
};
struct AxialAndCost {
    int2 axial;
    u32 cost;
};
struct AxialAndEdge {
    int2 axial;
    u8 edge;
    b8 operator==(const AxialAndEdge& other) const = default;
};
struct HexState {
    HexList<Hex> hex_map;
    HandleList<Unit> units;

    // cache logic
    CountryTag player_tag;
    PlayerAction player_action;
    PseudoStates pseudo_states;
    UnorderedMap<int2, List<Handle<Unit>>> units_by_axial;

    // cache oob
    UnorderedMap<Handle<Unit>, List<Handle<Unit>>> units_oob; // roots are stored as optional

    // cache drawing
    Pool<CounterStack> counters;
    Pool<Label> label_pool;
    List<Vertex> verts { };
};
} // namespace pcg
