export module hex.terrain;

import std;

import pce.collections;
import pce.std;
import pce.math;

import hex.hex;
import hex.types;
import hex.render;

export namespace hex {
[[nodiscard]] constexpr u32 TerrainToMovementCost(const TerrainType terrain) {
    switch (terrain) {
        case TerrainType::TERRAIN_TYPE_DEEP_OCEAN: return 255U;
        case TerrainType::TERRAIN_TYPE_OCEAN: return 255U;
        case TerrainType::TERRAIN_TYPE_BEACH: return 2U;
        case TerrainType::TERRAIN_TYPE_GRASS: return 1U;
        case TerrainType::TERRAIN_TYPE_HILL: return 3U;
        case TerrainType::TERRAIN_TYPE_MOUNTAIN: return 5U;
        case TerrainType::TERRAIN_TYPE_SNOW: return 4U;
    }
    std::unreachable();
}
constexpr void GenerateTerritory(HexState& hex_state) {
    std::ranges::fill(hex_state.hex_map | std::views::transform(&Hex::owner), HexOwner { .tag = CountryTag::TAG_NONE, .contested = false });

    Queue<int2> frontier;
    for (const auto& [axial, unit_handles] : hex_state.units_by_axial) {
        if (hex_state.hex_map.Contains(axial) && !unit_handles.empty()) {
            hex_state.hex_map[axial].owner = HexOwner { .tag = hex_state.units[unit_handles[0]].tag, .contested = false };
            frontier.EmplaceBack(axial);
        }
    }
    while (!frontier.empty()) {
        const int2 current = frontier.front();
        frontier.Pop();
        const HexOwner hex_owner = hex_state.hex_map[current].owner;
        for (const int2 offset : HEX_AXIAL_NEIGHBOURS) {
            const int2 next = current + offset;
            if (hex_state.hex_map.Contains(next) && hex_state.hex_map[next].owner.tag == CountryTag::TAG_NONE) {
                hex_state.hex_map[next].owner = HexOwner { .tag = hex_owner.tag, .contested = false };
                frontier.EmplaceBack(next);
            }
        }
    }
}
constexpr void HexSetRoad(HexState& hex_state, const int2 axial, const u32 edge, const RoadLevel road_level) {
    const int2 axial_side = axial + HEX_AXIAL_NEIGHBOURS[edge];
    if (hex_state.hex_map.Contains(axial) && hex_state.hex_map.Contains(axial_side)) {
        const u32 mirror = (edge + 3) % HEX_CORNERS;
        const u8 road_level_u8 = static_cast<u8>(road_level);
        if (hex_state.hex_map[axial].roads.Test(edge) < road_level_u8) { hex_state.hex_map[axial].roads.Set(edge, road_level_u8); }
        if (hex_state.hex_map[axial_side].roads.Test(mirror) < road_level_u8) { hex_state.hex_map[axial_side].roads.Set(mirror, road_level_u8); }
    }
}
constexpr void HexSetRiver(HexState& hex_state, const int2 axial, const u32 edge) {
    const int2 axial_neighbour = axial + HEX_AXIAL_NEIGHBOURS[edge];
    if (hex_state.hex_map.Contains(axial) && hex_state.hex_map.Contains(axial_neighbour)) {
        hex_state.hex_map[axial].river_edges.Set(edge);
        hex_state.hex_map[axial_neighbour].river_edges.Set((edge + 3) % HEX_CORNERS);
    }
}

[[nodiscard]] constexpr b8 TerrainIsWater(const TerrainType terrain) { return terrain == TerrainType::TERRAIN_TYPE_DEEP_OCEAN || terrain == TerrainType::TERRAIN_TYPE_OCEAN; }

constexpr void CarveRoad(HexState& hex_state, const int2 axial_a, const int2 axial_b, const RoadLevel road_level) {
    const u32 distance = HexAxialDistance(axial_a, axial_b);
    if (distance) {
        const int3 cube_a = HexAxialToCube(axial_a);
        const int3 cube_b = HexAxialToCube(axial_b);
        int2 axial_previous = axial_a;
        for (u32 i = 1U; i <= distance; i++) {
            const f32 t = static_cast<f32>(i) / static_cast<f32>(distance);
            const int2 axial_current = HexCubeToAxial(HexCubeRound(HexCubeLerp(cube_a, cube_b, t)));
            if (axial_current == axial_previous) { continue; }
            if (hex_state.hex_map.Contains(axial_current) && TerrainIsWater(hex_state.hex_map[axial_current].terrain_type)) {
                axial_previous = axial_current;
                continue;
            }
            for (u32 side = 0U; side < HEX_CORNERS; side++) {
                if (axial_previous + HEX_AXIAL_NEIGHBOURS[side] == axial_current) {
                    HexSetRoad(hex_state, axial_previous, side, road_level);
                    break;
                }
            }
            axial_previous = axial_current;
        }
    }
}
} // namespace pcg