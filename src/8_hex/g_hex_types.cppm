module;

export module hex.types;

import std;

import pce.sdl;
import pce.collections;
import pce.std;
import pce.strong;
import pce.math;
import pce.globals;

import pcs.animation;

import hex.hex;
import hex.enums;

export namespace hex {
using namespace hex;
struct Counter {
    Color color_background { };
    Color color_icon { };
    Color color_border { };
};
struct CounterStack {
    int2 axial { };
    UnitIcon icon { };
    RangedType ranged_type { };
    Array<Counter, 12> stack { };
    Label label_echelon { };
    Label label_name_div { };
    Label label_name_sub { };
    Label label_icon_placeholder { };
    Label label_dmg_ranged { };
    Label label_dmg { };
    Label label_move_allowance { };
    Label label_steps { };
};

struct HexOwner {
    CountryTag tag { CountryTag::TAG_NONE };
    b8 contested { false };
};
struct Hex {
    TerrainType terrain_type { };
    TerrainFeature terrain_feature { };
    HexOwner owner { };
    HexBitset2 roads { };
    HexBitset river_edges { };
};
struct UnitToe {
    u8 move { };
    u8 dmg { };
    u8 dmg_ranged { };
    u8 steps { };
};
using UnitName = Array<char, 10>;
using UnitNameFull = Array<char, 24>;
[[nodiscard]] String UnitNameToString(const UnitName& name) { return std::string(name.begin(), std::ranges::find(name, '\0')); }
[[nodiscard]] String UnitNameToString(const UnitNameFull& name) { return std::string(name.begin(), std::ranges::find(name, '\0')); }
void UnitNameSet(UnitName& name, const String& text) {
    name = { };
    std::memcpy(name.data(), text.c_str(), math::Min(text.size(), static_cast<u32>(name.size())));
}
void UnitNameSet(UnitNameFull& name, const String& text) {
    name = { };
    std::memcpy(name.data(), text.c_str(), math::Min(text.size(), static_cast<u32>(name.size())));
}
struct UnitFlavor {
    UnitName name { };
    Color color { };
};
struct Unit;
struct UnitFormation {
    CountryTag tag { };
    UnitBranch branch { };
    Echelon echelon { };
    UnitName name { };
    UnitNameFull name_full { };
    Color color { };

    int2 axial_hq { };
    u8 command_radius;
    Stat move { };
    b8 prepared_defense { };
    i8 fatigue { };
    List<Handle<Unit>> support { };
    Stat artillery { };

    ActivationResult activation_result { };
    ActivationFatigueActions activation_fatigue_actions { };
};
struct Unit {
    int2 axial { };

    // unit formation
    Handle<UnitFormation> formation;
    Echelon echelon { };
    CountryTag tag { };
    UnitIcon icon { };
    RangedType ranged_type { };

    // unit stats
    Stat move { };
    u8 dmg { };
    u8 dmg_ranged { };
    Stat steps { };

    // unit flavour
    UnitName name_div { };
    UnitName name_sub { };
    Color color { };
};
struct UnitGroup {
    List<Handle<Unit>> unit_handles { };
    u32 steps { };
    u32 dmg_sum { };
    u32 dmg_ranged_sum { };
    u32 move_min { };
    u32 move_max { };
};
struct PseudoTarget {
    int2 axial { };
    List<Handle<Unit>> units { };
};
struct PseudoStates {
    Optional<int2> axial_hover { };
    Optional<int2> axial_select { };
    Optional<UnitGroup> unit_selection { };
    HandleOptional<UnitFormation> hq_select { };
};
struct AxialAndCost {
    int2 axial { };
    u32 cost { };
};
struct AxialAndEdge {
    int2 axial { };
    u8 edge { };
    b8 operator==(const AxialAndEdge& other) const = default;
};

struct UnitSpawnCmd {
    Handle<UnitFormation> formation;
    UnitIcon icon;
    Echelon echelon;
};
struct HexState {
    HexList<Hex> hex_map { };

    CountryTag player_tag;
    HandleList<Unit> units { };
    HandleList<UnitFormation> unit_formations { };
    Handle<ParticleEmitter> particle_emitter { globalData.Create<ParticleEmitter>(ParticleEmitter { float2 { 0.0F, -60.0F } }) };

    // per frame
    PlayerAction player_action { };
    PseudoStates pseudo_states { };
    UnorderedMap<int2, List<Handle<Unit>>> units_by_axial { };
    UnorderedMap<Handle<UnitFormation>, List<Handle<Unit>>> units_by_formation { };

    // turn state
    u8 turn_number { 0 };
    TurnState turn_state { TurnState::TURN_NONE };
    b8 turn_state_changed;
    miliseconds32 turn_state_time { 0U };
    TurnHqState turn_hq_state { TurnHqState::TURN_HQ_NONE };
    b8 turn_hq_state_changed;
    miliseconds32 turn_hq_state_time { 0U };
    b8 turn_next_pressed { false };
    HandleOptional<UnitFormation> unit_formation_active { };
    List<Handle<UnitFormation>> unit_formations_left { };
    List<int2> objective_markers_axials { };

    // per frame
    Pool<CounterStack> counters { };
    Pool<Label> label_pool { };
    List<Vertex> verts { };
};
} // namespace hex

export template <> struct std::hash<hex::AxialAndEdge> {
    usize operator()(const hex::AxialAndEdge& axial_and_edge) const noexcept {
        usize seed = std::hash<int2> { }(axial_and_edge.axial);
        HashCombine(seed, axial_and_edge.edge);
        return seed;
    }
};
