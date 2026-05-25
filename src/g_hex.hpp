#pragma once
#include "0_engine/u_types.hpp"
#include "1_systems/r_counter_system.hpp"

namespace pcg {
using namespace pce;
enum class CountryTag : u8 { TAG_GER, TAG_SOV, TAG_USA };
enum class TerrainType : u8 { TERRAIN_DEEP_OCEAN, TERRAIN_OCEAN, TERRAIN_BEACH, TERRAIN_GRASS, TERRAIN_FOREST, TERRAIN_MOUNTAIN, TERRAIN_SNOW };
enum class PlayerAction { PLAYER_ACTION_NONE, PLAYER_ACTION_SELECT, PLAYER_ACTION_DESELECT, PLAYER_ACTION_MOVE_CLICK, PLAYER_ACTION_MOVE_HOVER, PLAYER_ACTION_ATTACK_CLICK, PLAYER_ACTION_ATTACK_HOVER };

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
struct HexDrawInfo {
    float2 world { };
    Color color { };
};
struct PseudoTarget {
    int2 axial;
    List<Handle<Unit>> units;
};
struct PseudoStates {
    Optional<int2> axial_hover;
    Optional<int2> axial_select;
    List<Handle<Unit>> unit_handles_select;
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
} // namespace pcg
