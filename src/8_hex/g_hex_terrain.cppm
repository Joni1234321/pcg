module;

#include <algorithm>
#include <limits>
#include <ranges>
#include <utility>

export module pcg.hex.terrain;
import pce.u_util;


import pce.collections;
import pce.std;
import pcg.hex.core;
import pcg.hex.types;
import pcg.hex.render;

export namespace pcg {
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
constexpr HexList<Hex> GenerateTerrainType(const uint2 map_size, const u32 seed) {
    HexList<Hex> hexes;
    hexes.Resize(map_size);
    constexpr f32 SCALE = 0.04F;
    const f32 seed_f = static_cast<f32>(seed);
    for (u32 i = 0; i < hexes.Size(); i++) {
        const int2 axial = hexes.IndexToAxial(i);
        const float2 world = HexAxialToWorld(axial);
        hexes[axial].terrain_type = FloatToTerrainType((noise::Fbm(world.x * SCALE + seed_f, world.y * SCALE + seed_f) + 1.0F) * 0.5F);
    }
    return hexes;
}

inline void GenerateTerritory(HexState& hex_state) {
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

inline void HexSetRoad(HexState& hex_state, const int2 axial, const u32 side, const RoadLevel road_level) {
    const int2 axial_side = axial + HEX_AXIAL_NEIGHBOURS[side];
    if (hex_state.hex_map.Contains(axial) && hex_state.hex_map.Contains(axial_side)) {
        const u32 mirror = (side + 3) % HEX_CORNERS;
        const u8 road_level_u8 = static_cast<u8>(road_level);
        if (hex_state.hex_map[axial].roads.Test(side) < road_level_u8) { hex_state.hex_map[axial].roads.Set(side, road_level_u8); }
        if (hex_state.hex_map[axial_side].roads.Test(mirror) < road_level_u8) { hex_state.hex_map[axial_side].roads.Set(mirror, road_level_u8); }
    }
}
inline void HexSetRiver(HexState& hex_state, const int2 axial, const u32 side) {
    const int2 axial_neighbour = axial + HEX_AXIAL_NEIGHBOURS[side];
    if (hex_state.hex_map.Contains(axial) && hex_state.hex_map.Contains(axial_neighbour)) {
        hex_state.hex_map[axial].river_edges.Set(side);
        hex_state.hex_map[axial_neighbour].river_edges.Set((side + 3) % HEX_CORNERS);
    }
}

[[nodiscard]] constexpr b8 TerrainIsWater(const TerrainType terrain) { return terrain == TerrainType::TERRAIN_TYPE_DEEP_OCEAN || terrain == TerrainType::TERRAIN_TYPE_OCEAN; }

inline void CarveRoad(HexState& hex_state, const int2 axial_a, const int2 axial_b, const b8 big) {
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
                    HexSetRoad(hex_state, axial_previous, side, RoadLevel::ROAD_LEVEL_MEDIUM);
                    break;
                }
            }
            axial_previous = axial_current;
        }
    }
}

inline void GenerateRoads(HexState& hex_state) {
    List<int2> cities;
    List<int2> villages;
    for (u32 i = 0U; i < hex_state.hex_map.Size(); i++) {
        const Hex& hex = hex_state.hex_map.data[i];
        const int2 axial = hex_state.hex_map.IndexToAxial(i);
        if (hex.terrain_feature == TerrainFeature::TERRAIN_FEATURE_CITY) {
            cities.EmplaceBack(axial);
        } else if (hex.terrain_feature == TerrainFeature::TERRAIN_FEATURE_VILLAGE) {
            villages.EmplaceBack(axial);
        }
    }

    if (cities.size() >= 2U) {
        const u32 n = cities.size();
        List<u8> in_tree;
        in_tree.resize(n);
        std::ranges::fill(in_tree, u8 { 0U });
        List<u32> best_cost;
        best_cost.resize(n);
        std::ranges::fill(best_cost, std::numeric_limits<u32>::max());
        List<u32> best_parent;
        best_parent.resize(n);
        std::ranges::fill(best_parent, u32 { 0U });
        best_cost[0] = 0U;
        for (u32 step = 0U; step < n; step++) {
            u32 next = n;
            u32 next_cost = std::numeric_limits<u32>::max();
            for (u32 i = 0U; i < n; i++) {
                if (in_tree[i] == 0U && best_cost[i] < next_cost) {
                    next_cost = best_cost[i];
                    next = i;
                }
            }
            if (next == n) { break; }
            in_tree[next] = 1U;
            if (step > 0U) { CarveRoad(hex_state, cities[best_parent[next]], cities[next], true); }
            for (u32 i = 0U; i < n; i++) {
                if (in_tree[i] == 0U) {
                    const u32 d = HexAxialDistance(cities[next], cities[i]);
                    if (d < best_cost[i]) {
                        best_cost[i] = d;
                        best_parent[i] = next;
                    }
                }
            }
        }
    }

    auto connect_k_nearest = [&](const List<int2>& sources, const List<int2>& targets, const u32 big_count, const u32 small_count) {
        for (u32 i = 0U; i < sources.size(); i++) {
            List<u32> dist;
            List<u32> idx;
            dist.resize(targets.size());
            idx.resize(targets.size());
            u32 valid = 0U;
            for (u32 j = 0U; j < targets.size(); j++) {
                if (sources[i] == targets[j]) { continue; }
                dist[valid] = HexAxialDistance(sources[i], targets[j]);
                idx[valid] = j;
                valid++;
            }
            const u32 want = big_count + small_count;
            const u32 take = valid < want ? valid : want;
            for (u32 k = 0U; k < take; k++) {
                u32 best = k;
                for (u32 m = k + 1U; m < valid; m++) {
                    if (dist[m] < dist[best]) { best = m; }
                }
                std::swap(dist[k], dist[best]);
                std::swap(idx[k], idx[best]);
            }
            for (u32 k = 0U; k < take; k++) {
                const b8 big = k < big_count;
                CarveRoad(hex_state, sources[i], targets[idx[k]], big);
            }
        }
    };

    if (cities.size() >= 2U) { connect_k_nearest(cities, cities, 1U, 2U); }
    if (villages.size() >= 2U) { connect_k_nearest(villages, villages, 0U, 2U); }
    if (!cities.empty() && !villages.empty()) { connect_k_nearest(villages, cities, 0U, 1U); }
}

inline void GenerateRivers(HexState& hex_state, const u32 seed) {
    constexpr f32 SCALE = 0.04F;
    const f32 seed_f = static_cast<f32>(seed);
    auto elevation_at_axial = [&](const int2 axial) -> f32 {
        const float2 world = HexAxialToWorld(axial);
        return noise::Fbm(world.x * SCALE + seed_f, world.y * SCALE + seed_f);
    };

    UnorderedSet<AxialAndEdge> visited;
    for (u32 i = 0U; i < hex_state.hex_map.Size(); i++) {
        const Hex& hex = hex_state.hex_map.data[i];
        AxialAndEdge axial_and_side_current { .axial = hex_state.hex_map.IndexToAxial(i), .edge = 0 };
        if ((hex.terrain_type == TerrainType::TERRAIN_TYPE_MOUNTAIN || hex.terrain_type == TerrainType::TERRAIN_TYPE_SNOW) && !visited.contains(axial_and_side_current)) {
            for (u32 step = 0U; step < 64U; step++) {
                visited.emplace(axial_and_side_current);
                if (!hex_state.hex_map.Contains(axial_and_side_current.axial) || TerrainIsWater(hex_state.hex_map[axial_and_side_current.axial].terrain_type)) { break; }
                u8 side_best = HEX_CORNERS;
                f32 elevation_min = elevation_at_axial(axial_and_side_current.axial);
                for (u8 edge = 0; edge < HEX_CORNERS; edge++) {
                    const int2 axial_neighbour = axial_and_side_current.axial + HEX_AXIAL_NEIGHBOURS[edge];
                    if (hex_state.hex_map.Contains(axial_neighbour) && !visited.contains(AxialAndEdge { .axial = axial_neighbour, .edge = edge })) {
                        const f32 elevation = elevation_at_axial(axial_neighbour);
                        if (elevation < elevation_min) {
                            elevation_min = elevation;
                            side_best = edge;
                        }
                    }
                }
                if (side_best == HEX_CORNERS) { break; }
                HexSetRiver(hex_state, axial_and_side_current.axial, (side_best + 1) % HEX_CORNERS);
                HexSetRiver(hex_state, axial_and_side_current.axial, (side_best + 2) % HEX_CORNERS);
                axial_and_side_current.axial = axial_and_side_current.axial + HEX_AXIAL_NEIGHBOURS[side_best];
            }
        }
    }
}

inline void GenerateTerrainFeatures(HexState& hex_state, const u32 seed) {
    constexpr f32 SCALE = 0.12F;
    const f32 forest_seed = static_cast<f32>(seed) + 1000.0F;
    const f32 field_seed = static_cast<f32>(seed) + 4717.0F;
    for (u32 i = 0U; i < hex_state.hex_map.Size(); i++) {
        Hex& hex = hex_state.hex_map.data[i];
        const int2 axial = hex_state.hex_map.IndexToAxial(i);
        const float2 world = HexAxialToWorld(axial);
        const f32 forest_n = (noise::Fbm(world.x * SCALE + forest_seed, world.y * SCALE + forest_seed) + 1.0F) * 0.5F;
        const f32 field_n = (noise::Fbm(world.x * SCALE + field_seed, world.y * SCALE + field_seed) + 1.0F) * 0.5F;
        const u32 h = noise::Hash(axial.x + static_cast<i32>(seed), axial.y - static_cast<i32>(seed));
        hex.terrain_feature = TerrainFeature::TERRAIN_FEATURE_GRASSLAND;
        switch (hex.terrain_type) {
            case TerrainType::TERRAIN_TYPE_BEACH:
                if (hex.river_edges.Any() || forest_n < 0.42F) { hex.terrain_feature = TerrainFeature::TERRAIN_FEATURE_MARSH; }
                break;
            case TerrainType::TERRAIN_TYPE_GRASS:
                if ((h % 64U) == 0U) {
                    hex.terrain_feature = TerrainFeature::TERRAIN_FEATURE_CITY;
                } else if ((h % 13U) == 0U) {
                    hex.terrain_feature = TerrainFeature::TERRAIN_FEATURE_VILLAGE;
                } else if (forest_n > 0.68F) {
                    hex.terrain_feature = TerrainFeature::TERRAIN_FEATURE_WOODED_HEAVY;
                } else if (forest_n > 0.52F) {
                    hex.terrain_feature = TerrainFeature::TERRAIN_FEATURE_WOODED_LIGHTLY;
                } else if (field_n > 0.55F) {
                    hex.terrain_feature = TerrainFeature::TERRAIN_FEATURE_FIELD;
                }
                break;
            case TerrainType::TERRAIN_TYPE_HILL:
                if (forest_n > 0.58F) {
                    hex.terrain_feature = TerrainFeature::TERRAIN_FEATURE_WOODED_HEAVY;
                } else if (forest_n > 0.40F) {
                    hex.terrain_feature = TerrainFeature::TERRAIN_FEATURE_WOODED_LIGHTLY;
                }
                break;
            default: break;
        }
    }
}
} // namespace pcg