#pragma once
#include <algorithm>
#include <queue>
#include <ranges>

#include "0_engine/u_collections.hpp"
#include "0_engine/u_types.hpp"
#include "1_systems/r_counter_system.hpp"
#include "1_systems/r_hex_system.hpp"

namespace pcg {
using namespace pce;
enum class CountryTag : u8 { TAG_NONE, TAG_GER, TAG_SOV, TAG_USA };
enum class TerrainType : u8 { TERRAIN_DEEP_OCEAN, TERRAIN_OCEAN, TERRAIN_BEACH, TERRAIN_GRASS, TERRAIN_FOREST, TERRAIN_MOUNTAIN, TERRAIN_SNOW };
enum class PlayerAction { PLAYER_ACTION_NONE, PLAYER_ACTION_SELECT, PLAYER_ACTION_DESELECT, PLAYER_ACTION_MOVE_CLICK, PLAYER_ACTION_MOVE_HOVER, PLAYER_ACTION_ATTACK_CLICK, PLAYER_ACTION_ATTACK_HOVER };
enum class TerrainScheme : u8 { CIV_VIBRANT, SLATE_TABLE, HOI4_PAPER }; // colorschema.md

constexpr u32 MOVE_COST_ATTACK = 3U;
constexpr TerrainScheme TERRAIN_SCHEME = TerrainScheme::CIV_VIBRANT;
constexpr f32 BORDER_INNER_RADIUS = 0.90F;

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

// ai genrated border
inline void AppendCountryBorders(HexState& hex_state, const CameraState& camera) {
    struct BEdge {
        float2 w0 { };
        float2 w1 { };
        float2 inward { };   // unit normal pointing into the owning hex (towards centre)
        i32    partner0 { -1 };
        i32    partner1 { -1 };
    };
    constexpr u32 COUNTRY_COUNT = 4U; // index by CountryTag value (TAG_NONE=0 skipped)
    std::array<std::vector<BEdge>, COUNTRY_COUNT> per_country { };

    for (u32 i = 0; i < hex_state.hex_map.Size(); i++) {
        const Hex& hex = hex_state.hex_map.data[i];
        if (hex.country_tag == CountryTag::TAG_NONE) { continue; }
        const int2 axial = hex_state.hex_map.IndexToAxial(i);
        const float2 center = HexAxialToWorld(axial);
        for (u32 s = 0; s < HEX_CORNERS; s++) {
            const int2 nax = axial + HEX_AXIAL_NEIGHBOURS[s];
            const CountryTag ntag = hex_state.hex_map.Contains(nax) ? hex_state.hex_map[nax].country_tag : CountryTag::TAG_NONE;
            if (ntag == hex.country_tag) { continue; }
            // visual side index from neighbour index (screen y is down vs math y-up)
            const u32 side = (HEX_CORNERS - s) % HEX_CORNERS;
            const u32 j    = (side + 1U) % HEX_CORNERS;
            // corners on the lattice at radius 1.0 - adjacent same-country hexes share them exactly
            const float2 p0 = center + HEX_ANGLE[side];
            const float2 p1 = center + HEX_ANGLE[j];
            const float2 emid { (p0.x + p1.x) * 0.5F, (p0.y + p1.y) * 0.5F };
            float2 inward { center.x - emid.x, center.y - emid.y };
            const f32 ilen = std::sqrt(inward.x * inward.x + inward.y * inward.y);
            if (ilen > 0.0001F) { inward.x /= ilen; inward.y /= ilen; }
            per_country[static_cast<u32>(hex.country_tag)].push_back(BEdge { p0, p1, inward });
        }
    }

    auto snap_key = [](const float2 p) -> u64 {
        const i32 qx = static_cast<i32>(std::round(p.x * 4096.0F));
        const i32 qy = static_cast<i32>(std::round(p.y * 4096.0F));
        return (static_cast<u64>(static_cast<u32>(qx)) << 32) | static_cast<u64>(static_cast<u32>(qy));
    };
    for (u32 tag_i = 1; tag_i < COUNTRY_COUNT; tag_i++) {
        auto& edges = per_country[tag_i];
        if (edges.empty()) { continue; }
        std::unordered_map<u64, std::array<i32, 4>> bucket;
        auto push_bucket = [&](float2 corner, i32 ei) {
            auto& slot = bucket.try_emplace(snap_key(corner), std::array<i32, 4> { -1, -1, -1, -1 }).first->second;
            for (i32 k = 0; k < 4; k++) {
                if (slot[k] == -1) { slot[k] = ei; return; }
            }
        };
        for (i32 ei = 0; ei < static_cast<i32>(edges.size()); ei++) {
            push_bucket(edges[ei].w0, ei);
            push_bucket(edges[ei].w1, ei);
        }
        auto find_partner = [&](float2 corner, i32 self) -> i32 {
            const auto it = bucket.find(snap_key(corner));
            if (it == bucket.end()) { return -1; }
            for (i32 k = 0; k < 4; k++) {
                if (it->second[k] != -1 && it->second[k] != self) { return it->second[k]; }
            }
            return -1;
        };
        for (i32 ei = 0; ei < static_cast<i32>(edges.size()); ei++) {
            edges[ei].partner0 = find_partner(edges[ei].w0, ei);
            edges[ei].partner1 = find_partner(edges[ei].w1, ei);
        }

        auto miter_dir = [&](float2 my_inward, i32 partner) -> float2 {
            if (partner < 0) { return my_inward; }
            const float2 other = edges[partner].inward;
            float2 m { my_inward.x + other.x, my_inward.y + other.y };
            const f32 mlen = std::sqrt(m.x * m.x + m.y * m.y);
            if (mlen < 0.0001F) { return my_inward; }
            m.x /= mlen; m.y /= mlen;
            const f32 d = m.x * my_inward.x + m.y * my_inward.y;
            const f32 inv = d > 0.25F ? 1.0F / d : 4.0F; // clamp so very sharp turns don't spike
            return float2 { m.x * inv, m.y * inv };
        };
        auto world_to_screen_fpoint = [&](float2 w) -> SDL_FPoint {
            const int2 s = camera.WorldToScreen(w);
            return SDL_FPoint { static_cast<f32>(s.x), static_cast<f32>(s.y) };
        };

        const ColorF col = static_cast<ColorF>(CountryTagToColor(static_cast<CountryTag>(tag_i)));
        for (const BEdge& e : edges) {
            const float2 m0 = miter_dir(e.inward, e.partner0);
            const float2 m1 = miter_dir(e.inward, e.partner1);
            const f32 inner_shift = 1.0F - BORDER_INNER_RADIUS; // positive = inset into hex
            const f32 outer_shift = BORDER_THICKNESS;
            const float2 a_in_w  { e.w0.x + m0.x * inner_shift, e.w0.y + m0.y * inner_shift };
            const float2 b_in_w  { e.w1.x + m1.x * inner_shift, e.w1.y + m1.y * inner_shift };
            const float2 a_out_w { e.w0.x - m0.x * outer_shift, e.w0.y - m0.y * outer_shift };
            const float2 b_out_w { e.w1.x - m1.x * outer_shift, e.w1.y - m1.y * outer_shift };
            const SDL_FPoint a_in  = world_to_screen_fpoint(a_in_w);
            const SDL_FPoint b_in  = world_to_screen_fpoint(b_in_w);
            const SDL_FPoint a_out = world_to_screen_fpoint(a_out_w);
            const SDL_FPoint b_out = world_to_screen_fpoint(b_out_w);
            // tri 1: a_in, b_in, b_out
            hex_state.verts.EmplaceBack(a_in,  col, SDL_FPoint { });
            hex_state.verts.EmplaceBack(b_in,  col, SDL_FPoint { });
            hex_state.verts.EmplaceBack(b_out, col, SDL_FPoint { });
            // tri 2: a_in, b_out, a_out
            hex_state.verts.EmplaceBack(a_in,  col, SDL_FPoint { });
            hex_state.verts.EmplaceBack(b_out, col, SDL_FPoint { });
            hex_state.verts.EmplaceBack(a_out, col, SDL_FPoint { });
        }
    }
}


} // namespace pcg
