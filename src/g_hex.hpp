#pragma once
#include <queue>

#include "0_engine/u_collections.hpp"
#include "0_engine/u_types.hpp"
#include "1_systems/r_counter_system.hpp"
#include "1_systems/r_hex_system.hpp"

namespace pcg {
using namespace pce;
enum class CountryTag : u8 { TAG_NONE, TAG_GER, TAG_SOV, TAG_USA };
enum class TerrainType : u8 { TERRAIN_DEEP_OCEAN, TERRAIN_OCEAN, TERRAIN_BEACH, TERRAIN_GRASS, TERRAIN_FOREST, TERRAIN_MOUNTAIN, TERRAIN_SNOW };
enum class PlayerAction { PLAYER_ACTION_NONE, PLAYER_ACTION_SELECT, PLAYER_ACTION_DESELECT, PLAYER_ACTION_MOVE_CLICK, PLAYER_ACTION_MOVE_HOVER, PLAYER_ACTION_ATTACK_CLICK, PLAYER_ACTION_ATTACK_HOVER };

constexpr u32 MOVE_COST_ATTACK = 3U;

struct Hex {
    TerrainType terrain;
    CountryTag country_tag { CountryTag::TAG_NONE };
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
struct UnitGroup {
    List<Handle<Unit>> unit_handles;
    u32 dmg_sum;
    u32 def_sum;
    u32 move_min;
    u32 move_max;
};
struct HexDrawInfo {
    float2 world { };
    Color color { };
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
    u32 cost;
    int2 axial;
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
    Pool<CounterStack> counters;
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

// colorschema.md
enum class TerrainScheme : u8 { CIV_VIBRANT, SLATE_TABLE, HOI4_PAPER };
constexpr TerrainScheme TERRAIN_SCHEME = TerrainScheme::SLATE_TABLE;

[[nodiscard]] constexpr Color TerrainToColorScheme(const TerrainType terrain) {
    if constexpr (TERRAIN_SCHEME == TerrainScheme::CIV_VIBRANT) {
        return TerrainToColor(terrain); // saturated default (existing palette)
    } else if constexpr (TERRAIN_SCHEME == TerrainScheme::SLATE_TABLE) {
        // cool, desaturated - like a board game printed on slate.
        switch (terrain) {
            case TerrainType::TERRAIN_DEEP_OCEAN: return Color {  28U,  38U,  55U };
            case TerrainType::TERRAIN_OCEAN:      return Color {  52U,  72U,  96U };
            case TerrainType::TERRAIN_BEACH:      return Color { 156U, 144U, 110U };
            case TerrainType::TERRAIN_GRASS:      return Color { 102U, 122U,  86U };
            case TerrainType::TERRAIN_FOREST:     return Color {  62U,  84U,  60U };
            case TerrainType::TERRAIN_MOUNTAIN:   return Color {  92U,  92U, 100U };
            case TerrainType::TERRAIN_SNOW:       return Color { 198U, 204U, 212U };
        }
    } else { // HOI4_PAPER - warm parchment / political-map feel.
        switch (terrain) {
            case TerrainType::TERRAIN_DEEP_OCEAN: return Color {  88U, 112U, 142U };
            case TerrainType::TERRAIN_OCEAN:      return Color { 130U, 158U, 184U };
            case TerrainType::TERRAIN_BEACH:      return Color { 220U, 200U, 158U };
            case TerrainType::TERRAIN_GRASS:      return Color { 178U, 184U, 132U };
            case TerrainType::TERRAIN_FOREST:     return Color { 128U, 144U,  96U };
            case TerrainType::TERRAIN_MOUNTAIN:   return Color { 152U, 138U, 116U };
            case TerrainType::TERRAIN_SNOW:       return Color { 232U, 226U, 210U };
        }
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
        case CountryTag::TAG_NONE: return colors::WHITE;
    }
    __builtin_unreachable();
}

constexpr HexList<Hex> GenerateTerrain(const uint2 map_size, const u32 seed) {
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

inline void GenerateTerritory(HexState& hex_state) {
    std::ranges::fill(hex_state.hex_map | std::views::transform(&Hex::country_tag), CountryTag::TAG_NONE);

    Queue<int2> frontier;
    for (const auto& [axial, unit_handles] : hex_state.units_by_axial) {
        if (!hex_state.hex_map.Contains(axial)) { continue; }
        if (unit_handles.empty()) { continue; }
        hex_state.hex_map[axial].country_tag = hex_state.units[unit_handles[0]].tag;
        frontier.EmplaceBack(axial);
    }
    while (!frontier.empty()) {
        const int2 current = frontier.front();
        frontier.Pop();
        const CountryTag current_tag = hex_state.hex_map[current].country_tag;
        for (const int2 offset : HEX_AXIAL_NEIGHBOURS) {
            const int2 next = current + offset;
            if (!hex_state.hex_map.Contains(next)) { continue; }
            if (hex_state.hex_map[next].country_tag != CountryTag::TAG_NONE) { continue; }
            hex_state.hex_map[next].country_tag = current_tag;
            frontier.EmplaceBack(next);
        }
    }
}
} // namespace pcg
