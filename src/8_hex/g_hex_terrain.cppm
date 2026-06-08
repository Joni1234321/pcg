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

// ai
inline void GenerateRivers(HexState& hex_state, const u32 seed) {
    constexpr f32 SCALE = 0.04F;
    constexpr u32 RIVER_THRESHOLD = 24U; // min upstream cells before an edge becomes a river
    const f32 seed_f = static_cast<f32>(seed);
    HexList<Hex>& map = hex_state.hex_map;
    const u32 count = map.Size();
    if (count == 0U) { return; }

    // 1. Elevation per hex (same field the terrain uses, so rivers follow the relief).
    List<f32> elevation;
    elevation.resize(count);
    for (u32 i = 0U; i < count; i++) {
        const float2 world = HexAxialToWorld(map.IndexToAxial(i));
        elevation[i] = noise::Fbm(world.x * SCALE + seed_f, world.y * SCALE + seed_f);
    }

    // 2. Priority-Flood: fill depressions and assign a downhill flow direction toward the sea.
    constexpr u8 FLOW_NONE = HEX_CORNERS; // sink: ocean or map edge
    List<u8> flow_side;
    flow_side.resize(count);
    std::ranges::fill(flow_side, FLOW_NONE);
    List<u8> visited;
    visited.resize(count);
    std::ranges::fill(visited, u8 { 0U });
    List<u32> drain_order; // pop order == ascending filled elevation
    drain_order.reserve(count);

    struct Node {
        f32 elevation;
        u32 index;
    };
    const auto higher = [](const Node& a, const Node& b) { return a.elevation > b.elevation; };
    std::priority_queue<Node, std::vector<Node>, decltype(higher)> open(higher);

    // Seed with water tiles and border tiles (water can drain off the map there).
    for (u32 i = 0U; i < count; i++) {
        const int2 axial = map.IndexToAxial(i);
        b8 is_edge = false;
        for (const int2 offset : HEX_AXIAL_NEIGHBOURS) {
            if (!map.Contains(axial + offset)) {
                is_edge = true;
                break;
            }
        }
        if (TerrainIsWater(map.data[i].terrain_type) || is_edge) {
            open.push(Node { elevation[i], i });
            visited[i] = 1U;
        }
    }

    while (!open.empty()) {
        const Node node = open.top();
        open.pop();
        drain_order.EmplaceBack(node.index);
        const int2 axial = map.IndexToAxial(node.index);
        for (u8 side = 0U; side < HEX_CORNERS; side++) {
            const int2 neighbour = axial + HEX_AXIAL_NEIGHBOURS[side];
            if (!map.Contains(neighbour)) { continue; }
            const u32 ni = map.AxialToIndex(neighbour);
            if (visited[ni]) { continue; }
            visited[ni] = 1U;
            elevation[ni] = std::max(elevation[ni], node.elevation); // depression fill
            flow_side[ni] = (side + 3U) % HEX_CORNERS;                // points back to 'node' (downhill)
            open.push(Node { elevation[ni], ni });
        }
    }

    // 3. Flow accumulation: 1 unit of rainfall per cell, pushed downstream high -> low.
    List<u32> accumulation;
    accumulation.resize(count);
    std::ranges::fill(accumulation, u32 { 1U });
    for (u32 k = drain_order.size(); k-- > 0;) {
        const u32 i = drain_order[k];
        if (flow_side[i] == FLOW_NONE) { continue; }
        const int2 downstream = map.IndexToAxial(i) + HEX_AXIAL_NEIGHBOURS[flow_side[i]];
        accumulation[map.AxialToIndex(downstream)] += accumulation[i];
    }

    // 4. Carve river edges where accumulated flow crosses the threshold.
    for (u32 i = 0U; i < count; i++) {
        if (flow_side[i] == FLOW_NONE) { continue; }
        if (accumulation[i] < RIVER_THRESHOLD) { continue; }
        if (TerrainIsWater(map.data[i].terrain_type)) { continue; }
        const int2 axial = map.IndexToAxial(i);
        const u8 side = flow_side[i];
        HexSetRiver(hex_state, axial, (side + 1U) % HEX_CORNERS);
        HexSetRiver(hex_state, axial, (side + 2U) % HEX_CORNERS);
    }
}

inline void GenerateRiversWalk(HexState& hex_state, const u32 seed) {
    constexpr f32 SCALE = 0.04F;
    const f32 seed_f = static_cast<f32>(seed);
    HexList<Hex>& map = hex_state.hex_map;
    auto elevation_at_axial = [&](const int2 axial) -> f32 {
        const float2 world = HexAxialToWorld(axial);
        return noise::Fbm(world.x * SCALE + seed_f, world.y * SCALE + seed_f);
    };
    auto dir_between = [&](const int2 from, const int2 to) -> u8 {
        for (u8 side = 0U; side < HEX_CORNERS; side++) {
            if (from + HEX_AXIAL_NEIGHBOURS[side] == to) { return side; }
        }
        return HEX_CORNERS;
    };
    // Edge walking: set the perimeter edges connecting the inflow edge to the outflow edge,
    // following the shorter arc around the hex so the river stays a single connected channel.
    auto set_arc = [&](const int2 axial, const u8 from_side, const u8 to_side) {
        const u8 cw = (to_side + HEX_CORNERS - from_side) % HEX_CORNERS;
        const u8 ccw = (from_side + HEX_CORNERS - to_side) % HEX_CORNERS;
        const u8 step = cw <= ccw ? 1U : HEX_CORNERS - 1U;
        for (u8 side = (from_side + step) % HEX_CORNERS; side != to_side; side = (side + step) % HEX_CORNERS) {
            HexSetRiver(hex_state, axial, side);
        }
    };

    UnorderedSet<int2> visited;
    for (u32 i = 0U; i < map.Size(); i++) {
        const Hex& hex = map.data[i];
        if (hex.terrain_type != TerrainType::TERRAIN_TYPE_MOUNTAIN && hex.terrain_type != TerrainType::TERRAIN_TYPE_SNOW) { continue; }
        const int2 source = map.IndexToAxial(i);
        if (visited.contains(source)) { continue; }

        // Pass 1: walk downhill (steepest descent) and collect every hex the river runs through.
        List<int2> path;
        int2 current = source;
        for (u32 step = 0U; step < 64U; step++) {
            if (visited.contains(current)) { break; }
            visited.emplace(current);
            path.EmplaceBack(current);
            if (!map.Contains(current) || TerrainIsWater(map[current].terrain_type)) { break; }
            u8 side_best = HEX_CORNERS;
            f32 elevation_min = elevation_at_axial(current);
            for (u8 side = 0U; side < HEX_CORNERS; side++) {
                const int2 neighbour = current + HEX_AXIAL_NEIGHBOURS[side];
                if (map.Contains(neighbour) && !visited.contains(neighbour)) {
                    const f32 elevation = elevation_at_axial(neighbour);
                    if (elevation < elevation_min) {
                        elevation_min = elevation;
                        side_best = side;
                    }
                }
            }
            if (side_best == HEX_CORNERS) { break; }
            current = current + HEX_AXIAL_NEIGHBOURS[side_best];
        }

        // Pass 2: edge-walk the collected path, drawing the correct channel edges per hex.
        for (u32 p = 0U; p < path.size(); p++) {
            const int2 axial = path[p];
            const u8 in_side = p > 0U ? dir_between(axial, path[p - 1U]) : HEX_CORNERS;
            const u8 out_side = p + 1U < path.size() ? dir_between(axial, path[p + 1U]) : HEX_CORNERS;
            if (in_side != HEX_CORNERS && out_side != HEX_CORNERS) {
                set_arc(axial, in_side, out_side);
            } else if (out_side != HEX_CORNERS) { // source: cap the two edges flanking the outflow
                HexSetRiver(hex_state, axial, (out_side + 1U) % HEX_CORNERS);
                HexSetRiver(hex_state, axial, (out_side + HEX_CORNERS - 1U) % HEX_CORNERS);
            } else if (in_side != HEX_CORNERS) { // mouth: cap the two edges flanking the inflow
                HexSetRiver(hex_state, axial, (in_side + 1U) % HEX_CORNERS);
                HexSetRiver(hex_state, axial, (in_side + HEX_CORNERS - 1U) % HEX_CORNERS);
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