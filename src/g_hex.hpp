#pragma once
#include <algorithm>
#include <ranges>

#include "0_engine/u_collections.hpp"
#include "0_engine/u_texture.hpp"
#include "0_engine/u_types.hpp"
#include "1_systems/r_counter_system.hpp"
#include "1_systems/r_hex_system.hpp"

namespace pcg {
using namespace pce;
enum class CountryTag : u8 { TAG_NONE, TAG_GER, TAG_SOV, TAG_USA };
enum class TerrainType : u8 { TERRAIN_DEEP_OCEAN, TERRAIN_OCEAN, TERRAIN_BEACH, TERRAIN_GRASS, TERRAIN_FOREST, TERRAIN_MOUNTAIN, TERRAIN_SNOW };
enum class PlayerAction { PLAYER_ACTION_NONE, PLAYER_ACTION_SELECT, PLAYER_ACTION_DESELECT, PLAYER_ACTION_MOVE_CLICK, PLAYER_ACTION_MOVE_HOVER, PLAYER_ACTION_ATTACK_CLICK, PLAYER_ACTION_ATTACK_HOVER };
enum class TerrainScheme : u8 { CIV_VIBRANT, SLATE_TABLE, HOI4_PAPER }; // colorschema.md

constexpr u32 MOVE_COST_ATTACK = 2U;
constexpr u32 MOVE_COST_ATTACK_PLANNED = 6U;
constexpr TerrainScheme TERRAIN_SCHEME = TerrainScheme::SLATE_TABLE;
constexpr f32 BORDER_INNER_RADIUS = 0.90F;
constexpr f32 BORDER_TEETH_DEPTH = 0.12F;
constexpr f32 BORDER_TEETH_HALF = 0.18F;
constexpr u32 BORDER_TEETH_EVERY = 1U;

struct HexOwner {
    CountryTag tag { CountryTag::TAG_NONE };
    b8 contested { false };
};
struct Hex {
    TerrainType terrain;
    HexOwner owner;
};
struct Unit {
    HandleOptional<Unit> parent;
    Array<char, 10> name { };
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

[[nodiscard]] constexpr Color TerrainToColorScheme(const TerrainType terrain) {
    if constexpr (TERRAIN_SCHEME == TerrainScheme::CIV_VIBRANT) {
        return TerrainToColor(terrain); // saturated default (existing palette)
    } else if constexpr (TERRAIN_SCHEME == TerrainScheme::SLATE_TABLE) {
        // cool, desaturated - like a board game printed on slate.
        switch (terrain) {
            case TerrainType::TERRAIN_DEEP_OCEAN: return Color { 28U, 38U, 55U };
            case TerrainType::TERRAIN_OCEAN: return Color { 52U, 72U, 96U };
            case TerrainType::TERRAIN_BEACH: return Color { 156U, 144U, 110U };
            case TerrainType::TERRAIN_GRASS: return Color { 102U, 122U, 86U };
            case TerrainType::TERRAIN_FOREST: return Color { 62U, 84U, 60U };
            case TerrainType::TERRAIN_MOUNTAIN: return Color { 92U, 92U, 100U };
            case TerrainType::TERRAIN_SNOW: return Color { 198U, 204U, 212U };
        }
    } else { // HOI4_PAPER - warm parchment / political-map feel.
        switch (terrain) {
            case TerrainType::TERRAIN_DEEP_OCEAN: return Color { 88U, 112U, 142U };
            case TerrainType::TERRAIN_OCEAN: return Color { 130U, 158U, 184U };
            case TerrainType::TERRAIN_BEACH: return Color { 220U, 200U, 158U };
            case TerrainType::TERRAIN_GRASS: return Color { 178U, 184U, 132U };
            case TerrainType::TERRAIN_FOREST: return Color { 128U, 144U, 96U };
            case TerrainType::TERRAIN_MOUNTAIN: return Color { 152U, 138U, 116U };
            case TerrainType::TERRAIN_SNOW: return Color { 232U, 226U, 210U };
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
    assert(false);
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
    std::ranges::fill(hex_state.hex_map | std::views::transform(&Hex::owner), HexOwner { .tag = CountryTag::TAG_NONE, .contested = false });

    Queue<int2> frontier;
    for (const auto& [axial, unit_handles] : hex_state.units_by_axial) {
        if (!hex_state.hex_map.Contains(axial)) { continue; }
        if (unit_handles.empty()) { continue; }
        hex_state.hex_map[axial].owner = HexOwner { .tag = hex_state.units[unit_handles[0]].tag, .contested = false };
        frontier.EmplaceBack(axial);
    }
    while (!frontier.empty()) {
        const int2 current = frontier.front();
        frontier.Pop();
        const HexOwner hex_owner = hex_state.hex_map[current].owner;
        for (const int2 offset : HEX_AXIAL_NEIGHBOURS) {
            const int2 next = current + offset;
            if (!hex_state.hex_map.Contains(next)) { continue; }
            if (hex_state.hex_map[next].owner.tag != CountryTag::TAG_NONE) { continue; }
            hex_state.hex_map[next].owner = HexOwner { .tag = hex_owner.tag, .contested = false };
            frontier.EmplaceBack(next);
        }
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
    const f32 sc = camera.scale;
    const f32 inner_r = BORDER_INNER_RADIUS;
    for (u32 i = 0; i < hex_state.hex_map.Size(); i++) {
        const Hex& hex = hex_state.hex_map.data[i];
        if (hex.owner.tag == CountryTag::TAG_NONE) { continue; }
        const int2 axial = hex_state.hex_map.IndexToAxial(i);
        const float2 own_center_w = HexAxialToWorld(axial);
        const float2 own_center_screen = camera.WorldToScreen(own_center_w);
        const ColorF col = static_cast<ColorF>(CountryTagToColor(hex.owner.tag));
        const f32 cx = static_cast<f32>(own_center_screen.x);
        const f32 cy = static_cast<f32>(own_center_screen.y);
        u32 edge_index = 0U;
        for (u32 s = 0; s < HEX_CORNERS; s++) {
            const int2 nax = axial + HEX_AXIAL_NEIGHBOURS[s];
            const CountryTag ntag = hex_state.hex_map.Contains(nax) ? hex_state.hex_map[nax].owner.tag : CountryTag::TAG_NONE;
            if (ntag == hex.owner.tag) { continue; }
            // visual side index from neighbour index (screen y is down vs math y-up)
            const u32 side = (HEX_CORNERS - s) % HEX_CORNERS;
            const u32 j = (side + 1U) % HEX_CORNERS;
            const float2 ang_a = HEX_ANGLE[side];
            const float2 ang_b = HEX_ANGLE[j];
            // Outer ring at the lattice corner (radius 1.0), inner ring at the
            // same radial position the tile uses (radius BORDER_INNER_RADIUS).
            // Same rounding policy as HexAppend -> the strip's inner edge lies
            // exactly on the tile's outer edge.
            const float2 outer_a { cx + ang_a.x * sc, cy + ang_a.y * sc };
            const float2 outer_b { cx + ang_b.x * sc, cy + ang_b.y * sc };
            const float2 inner_a { cx + ang_a.x * sc * inner_r, cy + ang_a.y * sc * inner_r };
            const float2 inner_b { cx + ang_b.x * sc * inner_r, cy + ang_b.y * sc * inner_r };
            // strip tri 1: inner_a, inner_b, outer_b
            hex_state.verts.EmplaceBack(inner_a, col);
            hex_state.verts.EmplaceBack(inner_b, col);
            hex_state.verts.EmplaceBack(outer_b, col);
            // strip tri 2: inner_a, outer_b, outer_a
            hex_state.verts.EmplaceBack(inner_a, col);
            hex_state.verts.EmplaceBack(outer_b, col);
            hex_state.verts.EmplaceBack(outer_a, col);

            if ((edge_index % BORDER_TEETH_EVERY) == 0U) {
                // Shared anchor: the lattice-edge midpoint rounded once in
                // world space. Identical for both countries that share this
                // edge -> red and blue teeth project onto the same screen
                // point and end up facing each other.
                const float2 emid_w {
                    own_center_w.x + (ang_a.x + ang_b.x) * 0.5F,
                    own_center_w.y + (ang_a.y + ang_b.y) * 0.5F,
                };
                const float2 anchor_int = camera.WorldToScreen(emid_w);
                const SDL_FPoint anchor { static_cast<f32>(anchor_int.x), static_cast<f32>(anchor_int.y) };
                // Project the shared anchor onto this country's inner edge to
                // get the tooth base midpoint.
                const f32 ex = inner_b.x - inner_a.x;
                const f32 ey = inner_b.y - inner_a.y;
                const f32 elen2 = ex * ex + ey * ey;
                if (elen2 > 0.0001F) {
                    f32 t = ((anchor.x - inner_a.x) * ex + (anchor.y - inner_a.y) * ey) / elen2;
                    if (t < 0.0F) { t = 0.0F; }
                    if (t > 1.0F) { t = 1.0F; }
                    const SDL_FPoint base_mid { inner_a.x + ex * t, inner_a.y + ey * t };
                    const f32 elen = std::sqrt(elen2);
                    const f32 ux = ex / elen;
                    const f32 uy = ey / elen;
                    f32 half = BORDER_TEETH_HALF * sc;
                    if (half > elen * 0.5F) { half = elen * 0.5F; } // clamp so wide teeth never overshoot the edge
                    const float2 base_a { base_mid.x - ux * half, base_mid.y - uy * half };
                    const float2 base_b { base_mid.x + ux * half, base_mid.y + uy * half };
                    // Inward direction in screen = from edge-midpoint towards
                    // own centre, derived in the same coord frame as the strip.
                    float2 inward { -(ang_a.x + ang_b.x) * 0.5F, -(ang_a.y + ang_b.y) * 0.5F };
                    const f32 ilen = std::sqrt(inward.x * inward.x + inward.y * inward.y);
                    if (ilen > 0.0001F) {
                        inward.x /= ilen;
                        inward.y /= ilen;
                        const float2 apex {
                            base_mid.x + inward.x * BORDER_TEETH_DEPTH * sc,
                            base_mid.y + inward.y * BORDER_TEETH_DEPTH * sc,
                        };
                        hex_state.verts.EmplaceBack(base_a, col);
                        hex_state.verts.EmplaceBack(base_b, col);
                        hex_state.verts.EmplaceBack(apex, col);
                    }
                }
            }
            edge_index++;
        }
    }
}

} // namespace pcg
