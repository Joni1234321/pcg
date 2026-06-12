module;
#include <cassert>

export module hex.terrain;

import std;

import pce.collections;
import pce.std;
import pce.math;

import hex.hex;
import hex.types;
import hex.render;
import pce.logger;

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
constexpr void HexTerrainSetBorder(HexState& hex_state) {
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
constexpr void HexTerrainSetRoad(HexState& hex_state, const int2 axial, const u32 edge, const RoadLevel road_level) {
    const int2 axial_side = axial + HEX_AXIAL_NEIGHBOURS[edge];
    if (hex_state.hex_map.Contains(axial) && hex_state.hex_map.Contains(axial_side)) {
        const u32 mirror = (edge + 3) % HEX_CORNERS;
        const u8 road_level_u8 = static_cast<u8>(road_level);
        if (hex_state.hex_map[axial].roads.Test(edge) < road_level_u8) { hex_state.hex_map[axial].roads.Set(edge, road_level_u8); }
        if (hex_state.hex_map[axial_side].roads.Test(mirror) < road_level_u8) { hex_state.hex_map[axial_side].roads.Set(mirror, road_level_u8); }
    }
}
constexpr void HexTerrainSetRiver(HexState& hex_state, const int2 axial, const u32 edge) {
    const int2 axial_neighbour = axial + HEX_AXIAL_NEIGHBOURS[edge];
    if (hex_state.hex_map.Contains(axial) && hex_state.hex_map.Contains(axial_neighbour)) {
        hex_state.hex_map[axial].river_edges.Set(edge);
        hex_state.hex_map[axial_neighbour].river_edges.Set((edge + 3) % HEX_CORNERS);
    }
}

[[nodiscard]] constexpr b8 TerrainIsWater(const TerrainType terrain) { return terrain == TerrainType::TERRAIN_TYPE_DEEP_OCEAN || terrain == TerrainType::TERRAIN_TYPE_OCEAN; }

constexpr void HexTerrainCarveRoad(HexState& hex_state, const int2 axial_a, const int2 axial_b, const RoadLevel road_level) {
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
                    HexTerrainSetRoad(hex_state, axial_previous, side, road_level);
                    break;
                }
            }
            axial_previous = axial_current;
        }
    }
}

constexpr void HexTerrainSetRiverBetween(HexState& hex_state, const int2 axial_start, const int2 axial_end) {
    assert(axial_start != axial_end);
    List<int2> const axial_line = HexAxialLine(axial_start, axial_end);
    // AxialAndEdge axial_and_edge;
    // {
    //     const u8 edge = HexAxialEdgeNeighbor(axial_start, axial_line[1]);
    //     axial_and_edge = { .axial = axial_start, .edge = edge};
    //     HexTerrainSetRiver(hex_state, axial_start, edge);
    // }

    for (u32 i = 1U; i < axial_line.size() - 1U; i++) {
        const int2 axial = axial_line[i];
        const u8 edge_from = HexAxialEdgeNeighbor(axial, axial_line[i - 1U]);
        const u8 edge_to = HexAxialEdgeNeighbor(axial, axial_line[i + 1U]);
        const u8 edge_clockwise = (edge_to - edge_from + HEX_CORNERS) % HEX_CORNERS;
        const u8 step_edge = edge_clockwise <= HEX_CORNERS / 2 ? 1U : HEX_CORNERS - 1U;
        const u8 step_distance = math::Min(edge_clockwise, static_cast<u8>(HEX_CORNERS - edge_clockwise));
        for (u8 j = 1; j < step_distance; ++j) {
            const u8 edge = (edge_from + j * step_edge) % HEX_CORNERS;
            HexTerrainSetRiver(hex_state, axial, edge);
        }
    }
    // {
    //     const u8 edge = HexAxialEdgeNeighbor(axial_line[axial_line.size() - 2U], axial_end);
    //     HexTerrainSetRiver(hex_state, axial_end, edge);
    // }
}

constexpr void HexTerrainSetRoadBetween(HexState& hex_state, const int2 axial_start, const int2 axial_end,const RoadLevel road_level) {
    assert(axial_start != axial_end);
    const List<int2> axial_line = HexAxialLine(axial_start, axial_end);

    for (u32 i = 0; i < axial_line.size() - 1; ++i) {
        const int2 from = axial_line[i];
        const int2 to   = axial_line[i + 1];
        const u8 edge = HexAxialEdgeNeighbor(from, to);
        HexTerrainSetRoad(hex_state, from, edge, road_level);
    }
}

} // namespace pcg