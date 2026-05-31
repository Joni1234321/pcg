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
constexpr f32 ROAD_WIDTH = 0.09F;
constexpr f32 ROAD_CASING_EXTRA = 0.04F;
constexpr f32 RIVER_WIDTH = 0.13F;
constexpr f32 RIVER_CASING_EXTRA = 0.05F;
constexpr f32 RIVER_HIGHLIGHT_WIDTH = RIVER_WIDTH * 0.35F;

struct HexBitset {
    u8 value;
    [[nodiscard]] constexpr b8 None() const { return !value; }
    [[nodiscard]] constexpr b8 Any() const { return value; }
    [[nodiscard]] constexpr b8 Test(const u8 pos) const {
        assert(pos < HEX_CORNERS);
        return value & (0x1 << pos);
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

struct HexOwner {
    CountryTag tag { CountryTag::TAG_NONE };
    b8 contested { false };
};
struct Hex {
    TerrainType terrain_type;
    TerrainFeature terrain_feature;
    HexOwner owner;
    HexBitset road_edges { };
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

inline void HexSetRoadEdge(HexState& hex_state, const int2 axial, const u32 side) {
    const int2 axial_side = axial + HEX_AXIAL_NEIGHBOURS[side];
    if (hex_state.hex_map.Contains(axial) && hex_state.hex_map.Contains(axial_side)) {
        hex_state.hex_map[axial].road_edges.Set(side);
        hex_state.hex_map[axial_side].road_edges.Set((side + 3) % HEX_CORNERS);
    }
}
inline void HexSetRiverEdge(HexState& hex_state, const int2 axial, const u32 side) {
    const int2 axial_side = axial + HEX_AXIAL_NEIGHBOURS[side];
    if (hex_state.hex_map.Contains(axial) && hex_state.hex_map.Contains(axial_side)) {
        hex_state.hex_map[axial].river_edges.Set(side);
        hex_state.hex_map[axial_side].river_edges.Set((side + 3) % HEX_CORNERS);
    }
}

inline b8 SDL_RenderGeometry(SDL_Renderer* renderer, SDL_Texture* texture, const Span<const Vertex> vertices, const Span<const i32> indices) { return SDL_RenderGeometry(renderer, texture, reinterpret_cast<const SDL_Vertex*>(vertices.data()), vertices.size(), indices.data(), indices.size()); }
inline b8 SDL_RenderGeometry(SDL_Renderer* renderer, SDL_Texture* texture, const Span<const Vertex> vertices) { return SDL_RenderGeometry(renderer, texture, reinterpret_cast<const SDL_Vertex*>(vertices.data()), vertices.size(), nullptr, 0); }

// ai genrated border
//
// Pixel-perfect rationale:
//   HexAppend draws each tile as `center_int + HEX_ANGLE[i] * scale` -- the
//   centre is rounded to a pixel once and the corners are sub-pixel floats.
//   We mirror that exact convention here so the strip's geometry shares the
//   same `center_int` and the same `scale` factor as the tile it sits on,
//   which keeps the strip's inner edge pixel-perfectly aligned with the
//   tile's outer edge (when BORDER_INNER_RADIUS == the tile's draw radius,
//   currently 0.90).
//
//   For teeth across a country border we want red and blue to face each
//   other. Each country's inner edge endpoints are anchored on its own
//   centre, so the inner midpoints differ slightly between red and blue.
//   We therefore anchor every tooth's base midpoint to a *single shared*
//   screen point -- the lattice-edge midpoint rounded once in world space
//   -- and project that anchor onto each country's inner edge. Both
//   countries end up with base midpoints on the same perpendicular line
//   through the shared anchor, so their tooth tips face each other.
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
                    const f32 length_edge = math::Sqrt(length_edge_squared);
                    const float2 u = local_edge * float2(1.0F / length_edge);
                    f32 half = BORDER_TEETH_HALF * camera.scale;
                    if (half > length_edge * 0.5F) { half = length_edge * 0.5F; }
                    const float2 screen_base_a = screen_base_mid - u * float2(half);
                    const float2 screen_base_b = screen_base_mid + u * float2(half);
                    // Inward: from edge-midpoint towards own centre.
                    const float2 local_inward_raw = (local_angle_a + local_angle_b) * float2(-0.5F);
                    const f32 local_inward_raw_length = math::Hypot(local_inward_raw);
                    if (local_inward_raw_length > 0.0001F) {
                        const float2 local_inward = local_inward_raw * float2(1.0F / local_inward_raw_length);
                        const float2 screen_apex = screen_base_mid + local_inward * float2(BORDER_TEETH_DEPTH * camera.scale);
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
inline void CarveRoad(HexState& hex_state, const int2 axial_a, const int2 axial_b) {
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
                HexSetRoadEdge(hex_state, prev, s);
                break;
            }
        }
        prev = axial_current;
    }
}

// Roads connect each unit to its parent in the OOB hierarchy.
inline void GenerateRoads(HexState& hex_state) {
    for (u32 i = 0U; i < hex_state.units.size(); i++) {
        const Handle<Unit> unit_handle = hex_state.units.IndexToHandle(i);
        const Unit& unit = hex_state.units[unit_handle];
        if (!unit.parent.IsValid()) { continue; }
        const Unit& parent = hex_state.units[unit.parent.GetHandle()];
        CarveRoad(hex_state, unit.axial, parent.axial);
    }
}

// Rivers: pick high-elevation seeds and flow downhill to ocean, marking edges along the way.
inline void GenerateRivers(HexState& hex_state, const u32 seed) {
    constexpr f32 SCALE = 0.04F;
    const f32 seed_f = static_cast<f32>(seed);
    auto elevation_at = [&](const int2 axial) -> f32 {
        const float2 world = HexAxialToWorld(axial);
        return noise::Fbm(world.x * SCALE + seed_f, world.y * SCALE + seed_f);
    };

    // Guaranteed river from top to bottom along the middle, following shared hex-edge corners.
    // Pointy-top grids have no straight vertical edge column, so the river is a continuous
    // slant-then-vertical zig-zag that drifts slightly left as it descends.
    const i32 mid_x = static_cast<i32>(hex_state.hex_map.map_size.x) / 2;
    const i32 height = static_cast<i32>(hex_state.hex_map.map_size.y);
    if (height >= 1) { HexSetRiverEdge(hex_state, int2 { mid_x, 0 }, 5U); } // slant down-left
    if (height >= 2) { HexSetRiverEdge(hex_state, int2 { mid_x, 1 }, 3U); } // vertical
    for (i32 y = 2; y < height; y++) {
        const int2 axial { mid_x - (y - 1), y };
        HexSetRiverEdge(hex_state, axial, 2U); // slant down-left into row
        HexSetRiverEdge(hex_state, axial, 3U); // vertical down
    }

    UnorderedMap<int2, u8> visited;
    for (u32 i = 0U; i < hex_state.hex_map.Size(); i++) {
        const Hex& hex = hex_state.hex_map.data[i];
        if (hex.terrain_type != TerrainType::TERRAIN_TYPE_MOUNTAIN && hex.terrain_type != TerrainType::TERRAIN_TYPE_SNOW) { continue; }
        const int2 axial = hex_state.hex_map.IndexToAxial(i);
        if (visited.contains(axial)) { continue; }
        int2 cur = axial;
        for (u32 step = 0U; step < 64U; step++) {
            visited[cur] = 1U;
            if (!hex_state.hex_map.Contains(cur)) { break; }
            if (TerrainIsWater(hex_state.hex_map[cur].terrain_type)) { break; }
            // pick lowest unvisited neighbour
            u32 best_side = HEX_CORNERS;
            f32 best_elev = elevation_at(cur);
            for (u32 s = 0U; s < HEX_CORNERS; s++) {
                const int2 n = cur + HEX_AXIAL_NEIGHBOURS[s];
                if (!hex_state.hex_map.Contains(n)) { continue; }
                if (visited.contains(n)) { continue; }
                const f32 e = elevation_at(n);
                if (e < best_elev) {
                    best_elev = e;
                    best_side = s;
                }
            }
            if (best_side == HEX_CORNERS) { break; }
            HexSetRiverEdge(hex_state, cur, best_side);
            cur = cur + HEX_AXIAL_NEIGHBOURS[best_side];
        }
    }
}

inline void GenerateTerrainFeatures(HexState& hex_state, const u32 seed) {
    constexpr f32 SCALE = 0.12F;
    const f32 seed_f = static_cast<f32>(seed) + 1000.0F;
    for (u32 i = 0U; i < hex_state.hex_map.Size(); i++) {
        Hex& hex = hex_state.hex_map.data[i];
        const int2 axial = hex_state.hex_map.IndexToAxial(i);
        const float2 world = HexAxialToWorld(axial);
        const f32 n = (noise::Fbm(world.x * SCALE + seed_f, world.y * SCALE + seed_f) + 1.0F) * 0.5F;
        const u32 h = noise::Hash(axial.x + static_cast<i32>(seed), axial.y - static_cast<i32>(seed));
        hex.terrain_feature = TerrainFeature::TERRAIN_FEATURE_GRASSLAND;
        switch (hex.terrain_type) {
            case TerrainType::TERRAIN_TYPE_BEACH:
                if (hex.river_edges.Any() || n < 0.42F) { hex.terrain_feature = TerrainFeature::TERRAIN_FEATURE_MARSH; }
                break;
            case TerrainType::TERRAIN_TYPE_GRASS:
                if ((h % 64U) == 0U) {
                    hex.terrain_feature = TerrainFeature::TERRAIN_FEATURE_CITY;
                } else if ((h % 13U) == 0U) {
                    hex.terrain_feature = TerrainFeature::TERRAIN_FEATURE_VILLAGE;
                } else if (n > 0.50F) {
                    hex.terrain_feature = TerrainFeature::TERRAIN_FEATURE_FIELD;
                }
                break;
            case TerrainType::TERRAIN_TYPE_HILL:
                if (n > 0.62F) {
                    hex.terrain_feature = TerrainFeature::TERRAIN_FEATURE_WOODED_HEAVY;
                } else if (n > 0.42F) {
                    hex.terrain_feature = TerrainFeature::TERRAIN_FEATURE_WOODED_LIGHTLY;
                }
                break;
            default: break;
        }
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
                const float2 screen = camera.WorldToScreen(HexAxialToWorld(hex_state.hex_map.IndexToAxial(i)));
                const AABBF area = AABBF::FromCenter(screen, float2 { camera.scale * 1.1F });
                (void)SDL_RenderTexture(renderer, texture, nullptr, area);
            }
        }
    }
}

inline void AppendRoadMesh(HexState& hex_state, const CameraState& camera) {
    auto append_pass = [&](const f32 width, const ColorF color) {
        for (u32 i = 0U; i < hex_state.hex_map.Size(); i++) {
            const Hex& hex = hex_state.hex_map.data[i];
            if (hex.road_edges.Any()) {
                const float2 own_center_w = HexAxialToWorld(hex_state.hex_map.IndexToAxial(i));
                const float2 center = camera.WorldToScreen(own_center_w);
                for (u32 side = 0U; side < HEX_CORNERS; side++) {
                    if (hex.road_edges.Test(side)) {
                        const float2 mid = camera.WorldToScreen(own_center_w + HexAxialToWorld(HEX_AXIAL_NEIGHBOURS[side]) * float2 { 0.5F });
                        VertObbAppend(hex_state.verts, OBB::BetweenPoints(center, mid, width), color);
                    }
                }
            }
        }
    };

    append_pass((ROAD_WIDTH + ROAD_CASING_EXTRA) * camera.scale, colors::ROAD_CASING_BROWN);
    append_pass(ROAD_WIDTH * camera.scale, colors::ROAD_TAN);
}

inline void AppendRiverMesh(HexState& hex_state, const CameraState& camera) {
    auto append_pass = [&](const f32 width, const ColorF color) {
        for (u32 i = 0U; i < hex_state.hex_map.Size(); i++) {
            const Hex& hex = hex_state.hex_map.data[i];
            if (hex.river_edges.Any()) {
                const float2 center = camera.WorldToScreen(HexAxialToWorld(hex_state.hex_map.IndexToAxial(i)));
                for (u32 s = 0U; s < 3U; s++) {
                    if (hex.river_edges.Test(s)) {
                        // mirror screen-y vs math-y convention used by AppendCountryBorders
                        const u32 side = (HEX_CORNERS - s) % HEX_CORNERS;
                        const u32 j = (side + 1U) % HEX_CORNERS;
                        const float2 outer_a = center + HEX_ANGLE[side] * float2(camera.scale);
                        const float2 outer_b = center + HEX_ANGLE[j] * float2(camera.scale);
                        VertObbAppend(hex_state.verts, OBB::BetweenPoints(outer_a, outer_b, width), color);
                    }
                }
            }
        }
    };

    append_pass((RIVER_WIDTH + RIVER_CASING_EXTRA) * camera.scale, colors::RIVER_DEEP_BLUE);
    append_pass(RIVER_WIDTH * camera.scale, colors::RIVER_BLUE);
    append_pass(RIVER_HIGHLIGHT_WIDTH * camera.scale, colors::RIVER_HIGHLIGHT_BLUE);
}

} // namespace pcg
