#pragma once
#include "0_engine/u_collections.hpp"
#include "0_engine/u_types.hpp"
#include "1_systems/r_counter_system.hpp"
#include "1_systems/r_hex_system.hpp"

namespace pcg {
using namespace pce;
enum class CountryTag : u8 { TAG_NONE, TAG_GER, TAG_SOV, TAG_USA };
enum class TerrainType : u8 { TERRAIN_DEEP_OCEAN, TERRAIN_OCEAN, TERRAIN_BEACH, TERRAIN_GRASS, TERRAIN_FOREST, TERRAIN_MOUNTAIN, TERRAIN_SNOW };
enum class PlayerAction { PLAYER_ACTION_NONE, PLAYER_ACTION_SELECT, PLAYER_ACTION_DESELECT, PLAYER_ACTION_MOVE_CLICK, PLAYER_ACTION_MOVE_HOVER, PLAYER_ACTION_ATTACK_CLICK, PLAYER_ACTION_ATTACK_HOVER };
enum class TerrainScheme : u8 {
    PASTEL_HIGHLAND, // soft greens / blues, gentle - good general purpose
    PARCHMENT_MAP,   // warm cream/khaki, classic boardgame look
    ARCTIC_PAPER,    // very cool, very pale - winter front
    MUTED_TOPO,      // slightly more saturated, original look
    HOI4_PAPER,      // light topo-paper a la Hearts of Iron, slightly warm
    CIV_VIBRANT,     // saturated, distinct biomes (Civ-style readable hues)
    BLUEPRINT,       // dark cool background, contrasting accents
    SEPIA_RECON,     // monochrome warm, recon photo feel
    SLATE_TABLE,     // split-complementary: dark slate, sage, sapphire, ochre, plum
};
enum class HexStyle : u8 {
    SOLID,           // single hex at full scale, no gap (tessellates)
    TIGHT,           // tiny gap (0.97) - clean look, hex is dominant
    SPACED,          // visible gap (0.90) - clearly separated hexes
    AIRY,            // big gap (0.82) - very breathable boardgame feel
    DOUBLE_RING,     // outer ring at 1.00 + inner fill at 0.86 (faux border per hex)
};
enum class TerritoryStyle : u8 {
    OFF,             // no territory overlay at all
    OVERLAY,         // single uniform translucent overlay over the whole hex
    BORDER,          // Civ6-style: thin strip along each hex side that touches a different (or empty) country
};

constexpr TerrainScheme  CURRENT_TERRAIN_SCHEME   = TerrainScheme::PASTEL_HIGHLAND;
constexpr HexStyle       CURRENT_HEX_STYLE        = HexStyle::SPACED;
constexpr TerritoryStyle CURRENT_TERRITORY_STYLE  = TerritoryStyle::OFF;

// Hex sizing - the inner (terrain) hex scale and optional outer ring scale.
// HexStyle picks which pair gets used. SOLID renders only the inner at 1.0.
constexpr f32 HEX_INNER_SOLID       = 1.00F;
constexpr f32 HEX_INNER_TIGHT       = 0.97F;
constexpr f32 HEX_INNER_SPACED      = 0.90F;
constexpr f32 HEX_INNER_AIRY        = 0.82F;
constexpr f32 HEX_INNER_DOUBLE      = 0.86F; // inner fill when DOUBLE_RING
constexpr f32 HEX_OUTER_DOUBLE      = 1.00F; // outer ring when DOUBLE_RING

// Territory overlay alpha (OVERLAY style). Lower = more terrain shows through.
constexpr f32 TERRITORY_ALPHA = 0.45F;

// Resolved inner scale for the terrain fill.
[[nodiscard]] constexpr f32 HexInnerScale(const HexStyle style) {
    switch (style) {
        case HexStyle::SOLID:       return HEX_INNER_SOLID;
        case HexStyle::TIGHT:       return HEX_INNER_TIGHT;
        case HexStyle::SPACED:      return HEX_INNER_SPACED;
        case HexStyle::AIRY:        return HEX_INNER_AIRY;
        case HexStyle::DOUBLE_RING: return HEX_INNER_DOUBLE;
    }
    __builtin_unreachable();
}

[[nodiscard]] inline Color TerrainToColor(const TerrainType terrain, const TerrainScheme scheme) {
    if (scheme == TerrainScheme::PASTEL_HIGHLAND) {
        constexpr f32 L = 0.86F;
        switch (terrain) {
            case TerrainType::TERRAIN_DEEP_OCEAN: return Color::FromHsl(215.0F, 0.35F, L - 0.04F);
            case TerrainType::TERRAIN_OCEAN:      return Color::FromHsl(205.0F, 0.30F, L);
            case TerrainType::TERRAIN_BEACH:      return Color::FromHsl( 48.0F, 0.35F, L + 0.02F);
            case TerrainType::TERRAIN_GRASS:      return Color::FromHsl( 95.0F, 0.28F, L);
            case TerrainType::TERRAIN_FOREST:     return Color::FromHsl(135.0F, 0.25F, L - 0.04F);
            case TerrainType::TERRAIN_MOUNTAIN:   return Color::FromHsl( 30.0F, 0.12F, L - 0.06F);
            case TerrainType::TERRAIN_SNOW:       return Color::FromHsl(210.0F, 0.06F, L + 0.06F);
        }
    } else if (scheme == TerrainScheme::PARCHMENT_MAP) {
        constexpr f32 L = 0.84F;
        switch (terrain) {
            case TerrainType::TERRAIN_DEEP_OCEAN: return Color::FromHsl(200.0F, 0.25F, L - 0.04F);
            case TerrainType::TERRAIN_OCEAN:      return Color::FromHsl(195.0F, 0.22F, L);
            case TerrainType::TERRAIN_BEACH:      return Color::FromHsl( 42.0F, 0.40F, L + 0.04F);
            case TerrainType::TERRAIN_GRASS:      return Color::FromHsl( 70.0F, 0.28F, L);
            case TerrainType::TERRAIN_FOREST:     return Color::FromHsl( 85.0F, 0.22F, L - 0.06F);
            case TerrainType::TERRAIN_MOUNTAIN:   return Color::FromHsl( 28.0F, 0.18F, L - 0.08F);
            case TerrainType::TERRAIN_SNOW:       return Color::FromHsl( 40.0F, 0.10F, L + 0.08F);
        }
    } else if (scheme == TerrainScheme::ARCTIC_PAPER) {
        constexpr f32 L = 0.90F;
        switch (terrain) {
            case TerrainType::TERRAIN_DEEP_OCEAN: return Color::FromHsl(220.0F, 0.30F, L - 0.06F);
            case TerrainType::TERRAIN_OCEAN:      return Color::FromHsl(210.0F, 0.22F, L - 0.02F);
            case TerrainType::TERRAIN_BEACH:      return Color::FromHsl(200.0F, 0.10F, L + 0.02F);
            case TerrainType::TERRAIN_GRASS:      return Color::FromHsl(180.0F, 0.10F, L);
            case TerrainType::TERRAIN_FOREST:     return Color::FromHsl(160.0F, 0.15F, L - 0.04F);
            case TerrainType::TERRAIN_MOUNTAIN:   return Color::FromHsl(220.0F, 0.05F, L - 0.08F);
            case TerrainType::TERRAIN_SNOW:       return Color::FromHsl(210.0F, 0.02F, L + 0.04F);
        }
    } else if (scheme == TerrainScheme::MUTED_TOPO) {
        constexpr f32 L = 0.82F;
        switch (terrain) {
            case TerrainType::TERRAIN_DEEP_OCEAN: return Color::FromHsl(220.0F, 0.55F, L);
            case TerrainType::TERRAIN_OCEAN:      return Color::FromHsl(210.0F, 0.45F, L);
            case TerrainType::TERRAIN_BEACH:      return Color::FromHsl( 45.0F, 0.50F, L);
            case TerrainType::TERRAIN_GRASS:      return Color::FromHsl(100.0F, 0.50F, L);
            case TerrainType::TERRAIN_FOREST:     return Color::FromHsl(130.0F, 0.55F, L);
            case TerrainType::TERRAIN_MOUNTAIN:   return Color::FromHsl( 30.0F, 0.15F, L);
            case TerrainType::TERRAIN_SNOW:       return Color::FromHsl(200.0F, 0.10F, L);
        }
    } else if (scheme == TerrainScheme::HOI4_PAPER) {
        // Warm cream paper a la HoI4 / WWII strategic maps.
        switch (terrain) {
            case TerrainType::TERRAIN_DEEP_OCEAN: return Color::FromHsl(205.0F, 0.32F, 0.72F);
            case TerrainType::TERRAIN_OCEAN:      return Color::FromHsl(200.0F, 0.28F, 0.80F);
            case TerrainType::TERRAIN_BEACH:      return Color::FromHsl( 46.0F, 0.45F, 0.82F);
            case TerrainType::TERRAIN_GRASS:      return Color::FromHsl( 52.0F, 0.40F, 0.78F); // wheaty fields
            case TerrainType::TERRAIN_FOREST:     return Color::FromHsl( 95.0F, 0.30F, 0.62F); // darker olive
            case TerrainType::TERRAIN_MOUNTAIN:   return Color::FromHsl( 30.0F, 0.25F, 0.55F); // brown ridges
            case TerrainType::TERRAIN_SNOW:       return Color::FromHsl( 40.0F, 0.12F, 0.92F);
        }
    } else if (scheme == TerrainScheme::CIV_VIBRANT) {
        // Saturated and distinct so biomes pop. Civ-style readability.
        switch (terrain) {
            case TerrainType::TERRAIN_DEEP_OCEAN: return Color::FromHsl(220.0F, 0.70F, 0.32F);
            case TerrainType::TERRAIN_OCEAN:      return Color::FromHsl(208.0F, 0.65F, 0.50F);
            case TerrainType::TERRAIN_BEACH:      return Color::FromHsl( 48.0F, 0.70F, 0.72F);
            case TerrainType::TERRAIN_GRASS:      return Color::FromHsl(105.0F, 0.55F, 0.55F);
            case TerrainType::TERRAIN_FOREST:     return Color::FromHsl(135.0F, 0.55F, 0.35F);
            case TerrainType::TERRAIN_MOUNTAIN:   return Color::FromHsl( 25.0F, 0.30F, 0.42F);
            case TerrainType::TERRAIN_SNOW:       return Color::FromHsl(200.0F, 0.10F, 0.92F);
        }
    } else if (scheme == TerrainScheme::BLUEPRINT) {
        // Dark blue background with cyan/teal accents - tactical screen feel.
        switch (terrain) {
            case TerrainType::TERRAIN_DEEP_OCEAN: return Color::FromHsl(220.0F, 0.60F, 0.10F);
            case TerrainType::TERRAIN_OCEAN:      return Color::FromHsl(215.0F, 0.55F, 0.18F);
            case TerrainType::TERRAIN_BEACH:      return Color::FromHsl(195.0F, 0.40F, 0.40F);
            case TerrainType::TERRAIN_GRASS:      return Color::FromHsl(190.0F, 0.45F, 0.32F);
            case TerrainType::TERRAIN_FOREST:     return Color::FromHsl(180.0F, 0.50F, 0.22F);
            case TerrainType::TERRAIN_MOUNTAIN:   return Color::FromHsl(210.0F, 0.30F, 0.45F);
            case TerrainType::TERRAIN_SNOW:       return Color::FromHsl(200.0F, 0.40F, 0.78F);
        }
    } else if (scheme == TerrainScheme::SEPIA_RECON) {
        switch (terrain) {
            case TerrainType::TERRAIN_DEEP_OCEAN: return Color::FromHsl(30.0F, 0.30F, 0.30F);
            case TerrainType::TERRAIN_OCEAN:      return Color::FromHsl(32.0F, 0.30F, 0.45F);
            case TerrainType::TERRAIN_BEACH:      return Color::FromHsl(38.0F, 0.40F, 0.78F);
            case TerrainType::TERRAIN_GRASS:      return Color::FromHsl(35.0F, 0.30F, 0.65F);
            case TerrainType::TERRAIN_FOREST:     return Color::FromHsl(30.0F, 0.35F, 0.42F);
            case TerrainType::TERRAIN_MOUNTAIN:   return Color::FromHsl(25.0F, 0.30F, 0.32F);
            case TerrainType::TERRAIN_SNOW:       return Color::FromHsl(40.0F, 0.15F, 0.88F);
        }
    } else { // SLATE_TABLE - split-complementary, dark slate table background palette
        switch (terrain) {
            case TerrainType::TERRAIN_DEEP_OCEAN: return Color { 31U,  67U, 104U }; // darker sapphire
            case TerrainType::TERRAIN_OCEAN:      return Color { 43U,  92U, 143U }; // #2B5C8F deep sapphire
            case TerrainType::TERRAIN_BEACH:      return Color { 217U, 160U, 91U };  // #D9A05B warm ochre
            case TerrainType::TERRAIN_GRASS:      return Color { 74U, 139U, 106U }; // #4A8B6A sage green
            case TerrainType::TERRAIN_FOREST:     return Color { 52U,  96U,  72U }; // darker sage
            case TerrainType::TERRAIN_MOUNTAIN:   return Color { 110U, 93U,  99U };  // #6E5D63 muted plum
            case TerrainType::TERRAIN_SNOW:       return Color { 200U, 194U, 189U }; // pale warm grey
        }
    }
    __builtin_unreachable();
}

constexpr u32 MOVE_COST_ATTACK = 3U;

struct Hex {
    TerrainType terrain;
    CountryTag country_tag;
    u8 territory_distance { 255U };
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
    Color overlay { };
    TerrainType terrain { };
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
        case CountryTag::TAG_NONE: return colors::BLACK;
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
