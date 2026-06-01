#pragma once
#include <algorithm>
#include <cassert>
#include <ranges>

#include "0_engine/u_collections.hpp"
#include "0_engine/u_texture.hpp"
#include "0_engine/u_types.hpp"
#include "1_systems/r_counter_system.hpp"
#include "1_systems/r_hex_system.hpp"

namespace pcg {
using namespace pce;
enum class CountryTag : u8 { TAG_NONE, TAG_GER, TAG_SOV, TAG_USA };
enum class TerrainType : u8 { TERRAIN_TYPE_DEEP_OCEAN, TERRAIN_TYPE_OCEAN, TERRAIN_TYPE_HILL, TERRAIN_TYPE_BEACH, TERRAIN_TYPE_GRASS, TERRAIN_TYPE_MOUNTAIN, TERRAIN_TYPE_SNOW };
enum class TerrainFeature : u8 { TERRAIN_FEATURE_GRASSLAND, TERRAIN_FEATURE_FIELD, TERRAIN_FEATURE_CITY, TERRAIN_FEATURE_VILLAGE, TERRAIN_FEATURE_WOODED_LIGHTLY, TERRAIN_FEATURE_WOODED_HEAVY, TERRAIN_FEATURE_MARSH };

enum class PlayerAction : u8 { PLAYER_ACTION_NONE, PLAYER_ACTION_SELECT, PLAYER_ACTION_DESELECT, PLAYER_ACTION_MOVE_CLICK, PLAYER_ACTION_MOVE_HOVER, PLAYER_ACTION_ATTACK_CLICK, PLAYER_ACTION_ATTACK_HOVER };

enum class MapStyle : u8 { CIV_VIBRANT, SLATE_TABLE, HOI4_PAPER }; // colorschema.md
enum class TerrainStyle : u8 { TERRAIN_STYLE_SILHOUETTES, TERRAIN_STYLE_ICONS };

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
constexpr u8 ROAD_NONE = 0U;
constexpr u8 ROAD_SMALL = 1U;
constexpr u8 ROAD_BIG = 2U;

[[nodiscard]] inline float2 HexTileJitter(const int2 axial, const f32 amount) {
    const u32 h = noise::Hash(axial.x, axial.y);
    const f32 fx = (static_cast<f32>(h & 0xFFFFU) / 65535.0F) * 2.0F - 1.0F;
    const f32 fy = (static_cast<f32>((h >> 16) & 0xFFFFU) / 65535.0F) * 2.0F - 1.0F;
    return float2 { fx * amount, fy * amount };
}
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
    u32 cost;
    int2 axial;
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
[[nodiscard]] constexpr TerrainType FloatToTerrainType(const f32 terrain) {
    if (terrain < 0.25F) { return TerrainType::TERRAIN_TYPE_DEEP_OCEAN; }
    if (terrain < 0.38F) { return TerrainType::TERRAIN_TYPE_OCEAN; }
    if (terrain < 0.43F) { return TerrainType::TERRAIN_TYPE_BEACH; }
    if (terrain < 0.60F) { return TerrainType::TERRAIN_TYPE_GRASS; }
    if (terrain < 0.72F) { return TerrainType::TERRAIN_TYPE_HILL; }
    if (terrain < 0.85F) { return TerrainType::TERRAIN_TYPE_MOUNTAIN; }
    return TerrainType::TERRAIN_TYPE_SNOW;
}
[[nodiscard]] constexpr Color TerrainToColor(const TerrainType terrain) {
    switch (terrain) {
        case TerrainType::TERRAIN_TYPE_DEEP_OCEAN: return Color { 20U, 60U, 120U };
        case TerrainType::TERRAIN_TYPE_OCEAN: return Color { 50U, 100U, 180U };
        case TerrainType::TERRAIN_TYPE_BEACH: return colors::KHAKI;
        case TerrainType::TERRAIN_TYPE_GRASS: return Color { 100U, 190U, 80U };
        case TerrainType::TERRAIN_TYPE_HILL: return colors::FOREST_GREEN;
        case TerrainType::TERRAIN_TYPE_MOUNTAIN: return colors::GRAY;
        case TerrainType::TERRAIN_TYPE_SNOW: return colors::WHITE;
    }
    std::unreachable();
}

[[nodiscard]] constexpr Color TerrainToColorScheme(const TerrainType terrain) {
    if constexpr (TERRAIN_SCHEME == MapStyle::CIV_VIBRANT) {
        return TerrainToColor(terrain); // saturated default (existing palette)
    } else if constexpr (TERRAIN_SCHEME == MapStyle::SLATE_TABLE) {
        // cool, desaturated - like a board game printed on slate.
        switch (terrain) {
            case TerrainType::TERRAIN_TYPE_DEEP_OCEAN: return Color { 28U, 38U, 55U };
            case TerrainType::TERRAIN_TYPE_OCEAN: return Color { 52U, 72U, 96U };
            case TerrainType::TERRAIN_TYPE_BEACH: return Color { 156U, 144U, 110U };
            case TerrainType::TERRAIN_TYPE_GRASS: return Color { 102U, 122U, 86U };
            case TerrainType::TERRAIN_TYPE_HILL: return Color { 62U, 84U, 60U };
            case TerrainType::TERRAIN_TYPE_MOUNTAIN: return Color { 92U, 92U, 100U };
            case TerrainType::TERRAIN_TYPE_SNOW: return Color { 198U, 204U, 212U };
        }
    } else { // HOI4_PAPER - warm parchment / political-map feel.
        switch (terrain) {
            case TerrainType::TERRAIN_TYPE_DEEP_OCEAN: return Color { 88U, 112U, 142U };
            case TerrainType::TERRAIN_TYPE_OCEAN: return Color { 130U, 158U, 184U };
            case TerrainType::TERRAIN_TYPE_BEACH: return Color { 220U, 200U, 158U };
            case TerrainType::TERRAIN_TYPE_GRASS: return Color { 178U, 184U, 132U };
            case TerrainType::TERRAIN_TYPE_HILL: return Color { 128U, 144U, 96U };
            case TerrainType::TERRAIN_TYPE_MOUNTAIN: return Color { 152U, 138U, 116U };
            case TerrainType::TERRAIN_TYPE_SNOW: return Color { 232U, 226U, 210U };
        }
    }
    std::unreachable();
}
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
[[nodiscard]] constexpr Color CountryTagToColor(const CountryTag tag) {
    switch (tag) {
        case CountryTag::TAG_GER: return colors::WG_GER_BG;
        case CountryTag::TAG_SOV: return colors::WG_SOV_BG;
        case CountryTag::TAG_USA: return colors::WG_USA_BG;
        case CountryTag::TAG_NONE: return colors::WHITE;
    }
    assert(false);
    std::unreachable();
}

struct TerrainFeatureTextures {
    HandleOptional<Texture> grassland;
    HandleOptional<Texture> field;
    HandleOptional<Texture> city;
    HandleOptional<Texture> village;
    HandleOptional<Texture> wooded_lightly;
    HandleOptional<Texture> wooded_heavy;
    HandleOptional<Texture> marsh;

    explicit TerrainFeatureTextures(const RelativePath& dir) {
        grassland = globalData.Create<Texture>(Asset(dir / "grassland.png"));
        field = globalData.Create<Texture>(Asset(dir / "field.png"));
        city = globalData.Create<Texture>(Asset(dir / "city.png"));
        village = globalData.Create<Texture>(Asset(dir / "village.png"));
        wooded_lightly = globalData.Create<Texture>(Asset(dir / "wooded-lightly.png"));
        wooded_heavy = globalData.Create<Texture>(Asset(dir / "wooded-heavy.png"));
        marsh = globalData.Create<Texture>(Asset(dir / "marsh.png"));
    }
    [[nodiscard]] HandleOptional<Texture> ForFeature(const TerrainFeature feature) const {
        switch (feature) {
            case TerrainFeature::TERRAIN_FEATURE_GRASSLAND: return grassland;
            case TerrainFeature::TERRAIN_FEATURE_FIELD: return field;
            case TerrainFeature::TERRAIN_FEATURE_CITY: return city;
            case TerrainFeature::TERRAIN_FEATURE_VILLAGE: return village;
            case TerrainFeature::TERRAIN_FEATURE_WOODED_LIGHTLY: return wooded_lightly;
            case TerrainFeature::TERRAIN_FEATURE_WOODED_HEAVY: return wooded_heavy;
            case TerrainFeature::TERRAIN_FEATURE_MARSH: return marsh;
        }
        std::unreachable();
    }
};
struct TerrainFeatureTextureStack {
    TerrainFeatureTextures terrain_features_silhouettes { "terrain/terrain-silhouettes" };
    TerrainFeatureTextures terrain_features_icons { "terrain/terrain-icons" };
};

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

inline void HexSetRoad(HexState& hex_state, const int2 axial, const u32 side, const b8 big) {
    const int2 axial_side = axial + HEX_AXIAL_NEIGHBOURS[side];
    if (hex_state.hex_map.Contains(axial) && hex_state.hex_map.Contains(axial_side)) {
        const u32 mirror = (side + 3) % HEX_CORNERS;
        const u8 val = big ? ROAD_BIG : ROAD_SMALL;
        if (hex_state.hex_map[axial].roads.Test(side) < val) { hex_state.hex_map[axial].roads.Set(side, val); }
        if (hex_state.hex_map[axial_side].roads.Test(mirror) < val) { hex_state.hex_map[axial_side].roads.Set(mirror, val); }
    }
}
inline void HexSetRiver(HexState& hex_state, const int2 axial, const u32 side) {
    const int2 axial_side = axial + HEX_AXIAL_NEIGHBOURS[side];
    if (hex_state.hex_map.Contains(axial) && hex_state.hex_map.Contains(axial_side)) {
        hex_state.hex_map[axial].river_edges.Set(side);
        hex_state.hex_map[axial_side].river_edges.Set((side + 3) % HEX_CORNERS);
    }
}

inline b8 SDL_RenderGeometry(SDL_Renderer* renderer, SDL_Texture* texture, const Span<const Vertex> vertices, const Span<const i32> indices) { return SDL_RenderGeometry(renderer, texture, reinterpret_cast<const SDL_Vertex*>(vertices.data()), vertices.size(), indices.data(), indices.size()); }
inline b8 SDL_RenderGeometry(SDL_Renderer* renderer, SDL_Texture* texture, const Span<const Vertex> vertices) { return SDL_RenderGeometry(renderer, texture, reinterpret_cast<const SDL_Vertex*>(vertices.data()), vertices.size(), nullptr, 0); }

// ai genrated border
inline void AppendCountryBorders(HexState& hex_state, const CameraState& camera) {
    for (u32 i = 0; i < hex_state.hex_map.Size(); i++) {
        const Hex& hex = hex_state.hex_map.data[i];
        if (hex.owner.tag == CountryTag::TAG_NONE) { continue; }
        const int2 axial = hex_state.hex_map.IndexToAxial(i);
        const float2 world_center = HexAxialToWorld(axial);
        const float2 screen_center = camera.WorldToScreen(world_center);
        const ColorF color = static_cast<ColorF>(CountryTagToColor(hex.owner.tag));
        u32 edge_index = 0U;
        for (u32 s = 0; s < HEX_CORNERS; s++) {
            const int2 axial_neighbour = axial + HEX_AXIAL_NEIGHBOURS[s];
            if (!hex_state.hex_map.Contains(axial_neighbour) || hex_state.hex_map[axial_neighbour].owner.tag != hex.owner.tag) {
                const u32 side = (HEX_CORNERS - s) % HEX_CORNERS;
                const float2 local_angle_a = HEX_ANGLE[side];
                const float2 local_angle_b = HEX_ANGLE[(side + 1U) % HEX_CORNERS];
                const float2 screen_outer_a = screen_center + local_angle_a * float2(camera.scale);
                const float2 screen_outer_b = screen_center + local_angle_b * float2(camera.scale);
                const float2 screen_inner_a = screen_center + local_angle_a * float2(camera.scale * BORDER_INNER_RADIUS);
                const float2 screen_inner_b = screen_center + local_angle_b * float2(camera.scale * BORDER_INNER_RADIUS);
                hex_state.verts.EmplaceBack(screen_inner_a, color);
                hex_state.verts.EmplaceBack(screen_inner_b, color);
                hex_state.verts.EmplaceBack(screen_outer_b, color);
                hex_state.verts.EmplaceBack(screen_inner_a, color);
                hex_state.verts.EmplaceBack(screen_outer_b, color);
                hex_state.verts.EmplaceBack(screen_outer_a, color);

                // tooth
                const float2 screen_anchor = camera.WorldToScreen(world_center + (local_angle_a + local_angle_b) * float2(0.5F));
                const float2 local_edge = screen_inner_b - screen_inner_a;
                const f32 length_edge_squared = math::LengthSq(local_edge);
                if (length_edge_squared > 0.0001F) {
                    const float2 local_anchor = screen_anchor - screen_inner_a;
                    const f32 t = math::Clamp((local_anchor.x * local_edge.x + local_anchor.y * local_edge.y) / length_edge_squared, 0.0F, 1.0F);
                    const float2 screen_base_mid = screen_inner_a + local_edge * float2(t);
                    const float2 local_inward = (local_angle_a + local_angle_b) * float2(-0.5F);
                    const f32 local_inward_length = math::Hypot(local_inward);
                    if (local_inward_length > 0.0001F) {
                        const f32 length_edge = math::Sqrt(length_edge_squared);
                        const float2 local_edge_normalized = local_edge * float2(1.0F / length_edge);
                        const f32 width_teeth = std::min(BORDER_TEETH_HALF * camera.scale, length_edge * 0.5F);
                        const float2 screen_base_a = screen_base_mid - local_edge_normalized * float2(width_teeth);
                        const float2 screen_base_b = screen_base_mid + local_edge_normalized * float2(width_teeth);

                        const float2 local_inward_normalized = local_inward * float2(1.0F / local_inward_length);
                        const f32 height_teeth = BORDER_TEETH_DEPTH * camera.scale;
                        const float2 screen_apex = screen_base_mid + local_inward_normalized * float2(height_teeth);
                        hex_state.verts.EmplaceBack(screen_base_a, color);
                        hex_state.verts.EmplaceBack(screen_base_b, color);
                        hex_state.verts.EmplaceBack(screen_apex, color);
                    }
                }
                edge_index++;
            }
        }
    }
}

[[nodiscard]] constexpr b8 TerrainIsWater(const TerrainType terrain) { return terrain == TerrainType::TERRAIN_TYPE_DEEP_OCEAN || terrain == TerrainType::TERRAIN_TYPE_OCEAN; }

// Carve a road from axial_a to axial_b along the hex line, skipping water tiles.
inline void CarveRoad(HexState& hex_state, const int2 axial_a, const int2 axial_b, const b8 big) {
    const u32 dist = HexAxialDistance(axial_a, axial_b);
    if (dist == 0U) { return; }
    const int3 cube_a = HexAxialToCube(axial_a);
    const int3 cube_b = HexAxialToCube(axial_b);
    int2 prev = axial_a;
    for (u32 i = 1U; i <= dist; i++) {
        const f32 t = static_cast<f32>(i) / static_cast<f32>(dist);
        const int2 axial_current = HexCubeToAxial(HexCubeRound(HexCubeLerp(cube_a, cube_b, t)));
        if (axial_current == prev) { continue; }
        if (hex_state.hex_map.Contains(axial_current) && TerrainIsWater(hex_state.hex_map[axial_current].terrain_type)) {
            prev = axial_current;
            continue;
        }
        for (u32 s = 0U; s < HEX_CORNERS; s++) {
            if (prev + HEX_AXIAL_NEIGHBOURS[s] == axial_current) {
                HexSetRoad(hex_state, prev, s, big);
                break;
            }
        }
        prev = axial_current;
    }
}

inline void GenerateRoads(HexState& hex_state) {
    List<int2> cities;
    List<int2> villages;
    for (u32 i = 0U; i < hex_state.hex_map.Size(); i++) {
        const Hex& hex = hex_state.hex_map.data[i];
        const int2 axial = hex_state.hex_map.IndexToAxial(i);
        if (hex.terrain_feature == TerrainFeature::TERRAIN_FEATURE_CITY) { cities.EmplaceBack(axial); }
        else if (hex.terrain_feature == TerrainFeature::TERRAIN_FEATURE_VILLAGE) { villages.EmplaceBack(axial); }
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

    // K-nearest neighbour redundancy: each city gains 1 extra big road and 2 small roads.
    auto connect_k_nearest = [&](const List<int2>& sources, const List<int2>& targets,
                                  const u32 big_count, const u32 small_count) {
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

    if (cities.size() >= 2U) {
        connect_k_nearest(cities, cities, 1U, 2U);
    }

    if (villages.size() >= 2U) {
        connect_k_nearest(villages, villages, 0U, 2U);
    }

    if (!cities.empty() && !villages.empty()) {
        connect_k_nearest(villages, cities, 0U, 1U);
    }
}

inline void GenerateRivers(HexState& hex_state, const u32 seed) {
    constexpr f32 SCALE = 0.04F;
    const f32 seed_f = static_cast<f32>(seed);
    auto elevation_at = [&](const int2 axial) -> f32 {
        const float2 world = HexAxialToWorld(axial);
        return noise::Fbm(world.x * SCALE + seed_f, world.y * SCALE + seed_f);
    };

    UnorderedSet<int2> visited;
    for (u32 i = 0U; i < hex_state.hex_map.Size(); i++) {
        const Hex& hex = hex_state.hex_map.data[i];
        const int2 axial = hex_state.hex_map.IndexToAxial(i);
        if (hex.terrain_type == TerrainType::TERRAIN_TYPE_MOUNTAIN || hex.terrain_type == TerrainType::TERRAIN_TYPE_SNOW && !visited.contains(axial)) {
            int2 axial_current = axial;
            for (u32 step = 0U; step < 64U; step++) {
                visited.emplace(axial_current);
                if (!hex_state.hex_map.Contains(axial_current) || TerrainIsWater(hex_state.hex_map[axial_current].terrain_type)) { break; }
                constexpr Array SLANTED_SIDES { 0U, 1U, 2U, 3U, 4U, 5U };
                u8 side_best = HEX_CORNERS;
                f32 elevation_min = elevation_at(axial_current);
                for (const u32 side : SLANTED_SIDES) {
                    const int2 axial_neighbour = axial_current + HEX_AXIAL_NEIGHBOURS[side];
                    if (!hex_state.hex_map.Contains(axial_neighbour) || visited.contains(axial_neighbour)) { continue; }
                    const f32 elevation = elevation_at(axial_neighbour);
                    if (elevation < elevation_min) {
                        elevation_min = elevation;
                        side_best = side;
                    }
                }
                if (side_best == HEX_CORNERS) { break; } // if we didnt find any
                HexSetRiver(hex_state, axial_current, side_best);
                axial_current = axial_current + HEX_AXIAL_NEIGHBOURS[side_best];
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

[[nodiscard]] constexpr Color TerrainFeatureToTint(const TerrainFeature feature) {
    switch (feature) {
        case TerrainFeature::TERRAIN_FEATURE_CITY:           return colors::FEATURE_CITY;
        case TerrainFeature::TERRAIN_FEATURE_VILLAGE:        return colors::FEATURE_VILLAGE;
        case TerrainFeature::TERRAIN_FEATURE_WOODED_LIGHTLY: return colors::FEATURE_WOODED_LIGHT;
        case TerrainFeature::TERRAIN_FEATURE_WOODED_HEAVY:   return colors::FEATURE_WOODED_HEAVY;
        case TerrainFeature::TERRAIN_FEATURE_FIELD:          return colors::FEATURE_FIELD;
        case TerrainFeature::TERRAIN_FEATURE_MARSH:          return colors::FEATURE_MARSH;
        default:                                             return Color { 255U, 255U, 255U };
    }
}

inline void AppendTerrainFeatures(HexState& hex_state, const CameraState& camera) {
    SDL_Renderer* renderer = Singleton::Get<WindowState>().renderer;
    const TerrainFeatureTextureStack& stack = Singleton::Get<TerrainFeatureTextureStack>();
    const TerrainFeatureTextures& textures = TERRAIN_FEATURE_THEME == TerrainStyle::TERRAIN_STYLE_ICONS ? stack.terrain_features_icons : stack.terrain_features_silhouettes;
    for (u32 i = 0U; i < hex_state.hex_map.Size(); i++) {
        const Hex& hex = hex_state.hex_map.data[i];
        if (hex.terrain_feature != TerrainFeature::TERRAIN_FEATURE_GRASSLAND) {
            const HandleOptional<Texture> texture_handle = textures.ForFeature(hex.terrain_feature);
            if (texture_handle.IsValid()) {
                SDL_Texture* texture = globalData[texture_handle.GetHandle()];
                const Color tint = TerrainFeatureToTint(hex.terrain_feature);
                (void)SDL_SetTextureColorMod(texture, tint.r, tint.g, tint.b);
                const int2 axial = hex_state.hex_map.IndexToAxial(i);
                const float2 jitter = HexTileJitter(axial, FEATURE_POSITION_JITTER * camera.scale);
                const float2 screen = camera.WorldToScreen(HexAxialToWorld(axial)) + jitter;
                const AABBF area = AABBF::FromCenter(screen, float2 { camera.scale * 1.1F });
                (void)SDL_RenderTexture(renderer, texture, nullptr, area);
            }
        }
    }
}

inline void AppendRoadMesh(HexState& hex_state, const CameraState& camera) {
    constexpr ColorF color = static_cast<ColorF>(colors::ROAD_GREY);
    const f32 small_w = ROAD_WIDTH * camera.scale;
    const f32 big_w = ROAD_BIG_WIDTH * camera.scale;
    for (u32 i = 0U; i < hex_state.hex_map.Size(); i++) {
        const Hex& hex = hex_state.hex_map.data[i];
        if (!hex.roads.Any()) { continue; }
        const int2 axial = hex_state.hex_map.IndexToAxial(i);
        const float2 own_center_w = HexAxialToWorld(axial);
        const float2 center_unjittered = camera.WorldToScreen(own_center_w);
        const float2 center = center_unjittered + HexTileJitter(axial, ROAD_CENTER_JITTER * camera.scale);
        for (u32 side = 0U; side < HEX_CORNERS; side++) {
            const u8 v = hex.roads.Test(side);
            if (v == ROAD_NONE) { continue; }
            const float2 mid = camera.WorldToScreen(own_center_w + HexAxialToWorld(HEX_AXIAL_NEIGHBOURS[side]) * float2 { 0.5F });
            const f32 width = (v == ROAD_BIG) ? big_w : small_w;
            const float2 bend = mid + (center_unjittered - mid) * float2 { 0.5F };
            VertObbAppend(hex_state.verts, OBB::BetweenPoints(mid, bend, width), color);
            VertObbAppend(hex_state.verts, OBB::BetweenPoints(bend, center, width), color);
        }
    }
}

inline void AppendRiverMesh(HexState& hex_state, const CameraState& camera) {
    auto append_pass = [&](const f32 width, const ColorF color) {
        for (u32 i = 0U; i < hex_state.hex_map.Size(); i++) {
            const Hex& hex = hex_state.hex_map.data[i];
            if (!hex.river_edges.Any()) { continue; }
            const float2 center = camera.WorldToScreen(HexAxialToWorld(hex_state.hex_map.IndexToAxial(i)));
            for (u32 side = 0U; side < 3U; side++) {
                if (!hex.river_edges.Test(side)) { continue; }
                const float2 corner_a = center + HEX_ANGLE[(HEX_CORNERS - side) % HEX_CORNERS] * float2 { camera.scale };
                const float2 corner_b = center + HEX_ANGLE[(HEX_CORNERS + 1U - side) % HEX_CORNERS] * float2 { camera.scale };
                VertObbAppend(hex_state.verts, OBB::BetweenPoints(corner_a, corner_b, width), color);
            }
        }
    };

    append_pass((RIVER_WIDTH + RIVER_CASING_EXTRA) * camera.scale, colors::RIVER_DEEP_BLUE);
    append_pass(RIVER_WIDTH * camera.scale, colors::RIVER_BLUE);
    append_pass(RIVER_HIGHLIGHT_WIDTH * camera.scale, colors::RIVER_HIGHLIGHT_BLUE);
}

} // namespace pcg
