module;

#include <cassert>

export module hex.scenarios;

import std;

import pce.std;
import pce.collections;

import hex.hex;
import hex.types;
import hex.terrain;
import hex.terrain.generation;

export namespace hex {
UnitToe UnitGetToe(UnitIcon icon) {
    switch (icon) {
        case UnitIcon::ICON_INF: return UnitToe { .move = 4 * MOVE_POINT, .dmg = 3, .dmg_ranged = 0, .steps = 6 };
        case UnitIcon::ICON_ART: return UnitToe { .move = 4 * MOVE_POINT, .dmg = 3, .dmg_ranged = 0, .steps = 255U };
        case UnitIcon::ICON_HQ: return UnitToe { .move = 8 * MOVE_POINT, .dmg = 0, .dmg_ranged = 0, .steps = 255U };
        case UnitIcon::ICON_TANK: return UnitToe { .move = 16 * MOVE_POINT, .dmg = 5, .dmg_ranged = 3, .steps = 4 };
        case UnitIcon::ICON_ENGINEER: return UnitToe { .move = 4 * MOVE_POINT, .dmg = 3, .dmg_ranged = 0, .steps = 0 };
        default: assert(false); std::unreachable();
    }
}

Handle<UnitFormation> HexStateSpawnFormation(HexState& hex_state, CountryTag tag, UnitBranch branch, Echelon echelon, const String& name) {
    UnitFormation formation { .tag = tag, .branch = branch, .echelon = echelon };
    UnitNameSet(formation.name, name);
    return hex_state.unit_formations.EmplaceBack(std::move(formation));
}

Handle<Unit> HexStateSpawnUnit(HexState& hex_state, const UnitSpawnCmd& unit_spawn_cmd, int2 offset) {
    const UnitToe unit_toe = UnitGetToe(unit_spawn_cmd.icon);
    const int2 axial = HexOffsetToAxial(offset);
    return hex_state.units.EmplaceBack(Unit { .axial = axial,
                                              .formation = unit_spawn_cmd.formation,
                                              .echelon = unit_spawn_cmd.echelon,
                                              .tag = hex_state.unit_formations[unit_spawn_cmd.formation].tag,
                                              .icon = unit_spawn_cmd.icon,
                                              .ranged_type = RangedTypeUnitIcon(unit_spawn_cmd.icon),
                                              .move = Stat { .current = unit_toe.move, .max = unit_toe.move },
                                              .dmg = unit_toe.dmg,
                                              .dmg_ranged = unit_toe.dmg_ranged,
                                              .steps = { .current = unit_toe.steps, .max = unit_toe.steps},
                                              .name_div = { },
                                              .color = { } });
}

void HexScenarioAi(HexState& hex_state) {
    constexpr u32 SEED = 3489;

    hex_state.hex_map.Resize({ 20, 8 });
    HexTerrainGenerateFeatures(hex_state, SEED);
    HexTerrainGenerateRiversWalk(hex_state, SEED);
    HexTerrainGenerateFeatures(hex_state, SEED);
    HexTerrainGenerateRoads(hex_state);

    // GER: Heeresgruppe, I.Korps
    const Handle<UnitFormation> ger_hgr = HexStateSpawnFormation(hex_state, CountryTag::TAG_GER, UnitBranch::BRANCH_INFANTRY, Echelon::ECHELON_ARMY, "6AOK");
    (void)HexStateSpawnUnit(hex_state, UnitSpawnCmd { .formation = ger_hgr, .icon = UnitIcon::ICON_HQ, .echelon = Echelon::ECHELON_ARMY }, { 1, 2 });

    const Handle<UnitFormation> ger_kps = HexStateSpawnFormation(hex_state, CountryTag::TAG_GER, UnitBranch::BRANCH_INFANTRY, Echelon::ECHELON_CORPS, "16AK");
    (void)HexStateSpawnUnit(hex_state, UnitSpawnCmd { .formation = ger_kps, .icon = UnitIcon::ICON_HQ, .echelon = Echelon::ECHELON_CORPS }, { 3, 2 });
    (void)HexStateSpawnUnit(hex_state, UnitSpawnCmd { .formation = ger_kps, .icon = UnitIcon::ICON_INF, .echelon = Echelon::ECHELON_REGIMENT }, { 4, 1 });
    (void)HexStateSpawnUnit(hex_state, UnitSpawnCmd { .formation = ger_kps, .icon = UnitIcon::ICON_INF, .echelon = Echelon::ECHELON_REGIMENT }, { 5, 3 });
    (void)HexStateSpawnUnit(hex_state, UnitSpawnCmd { .formation = ger_kps, .icon = UnitIcon::ICON_TANK, .echelon = Echelon::ECHELON_BATTALION }, { 6, 2 });
    (void)HexStateSpawnUnit(hex_state, UnitSpawnCmd { .formation = ger_kps, .icon = UnitIcon::ICON_ART, .echelon = Echelon::ECHELON_BATTALION }, { 4, 2 });
    (void)HexStateSpawnUnit(hex_state, UnitSpawnCmd { .formation = ger_kps, .icon = UnitIcon::ICON_INF, .echelon = Echelon::ECHELON_BATTALION }, { 6, 1 });
    (void)HexStateSpawnUnit(hex_state, UnitSpawnCmd { .formation = ger_kps, .icon = UnitIcon::ICON_INF, .echelon = Echelon::ECHELON_BATTALION }, { 7, 3 });

    // SOV: Front, I Gds Corps
    const Handle<UnitFormation> sov_frt = HexStateSpawnFormation(hex_state, CountryTag::TAG_SOV, UnitBranch::BRANCH_GUARD, Echelon::ECHELON_ARMY, "2GA");
    (void)HexStateSpawnUnit(hex_state, UnitSpawnCmd { .formation = sov_frt, .icon = UnitIcon::ICON_HQ, .echelon = Echelon::ECHELON_ARMY }, { 15, 3 });
    (void)HexStateSpawnUnit(hex_state, UnitSpawnCmd { .formation = sov_frt, .icon = UnitIcon::ICON_INF, .echelon = Echelon::ECHELON_REGIMENT }, { 5, 2 }); // encircled

    const Handle<UnitFormation> sov_kps = HexStateSpawnFormation(hex_state, CountryTag::TAG_SOV, UnitBranch::BRANCH_ARMOR, Echelon::ECHELON_CORPS, "1TK");
    (void)HexStateSpawnUnit(hex_state, UnitSpawnCmd { .formation = sov_kps, .icon = UnitIcon::ICON_HQ, .echelon = Echelon::ECHELON_CORPS }, { 14, 3 });
    (void)HexStateSpawnUnit(hex_state, UnitSpawnCmd { .formation = sov_kps, .icon = UnitIcon::ICON_INF, .echelon = Echelon::ECHELON_REGIMENT }, { 12, 2 });
    (void)HexStateSpawnUnit(hex_state, UnitSpawnCmd { .formation = sov_kps, .icon = UnitIcon::ICON_INF, .echelon = Echelon::ECHELON_REGIMENT }, { 13, 4 });
    (void)HexStateSpawnUnit(hex_state, UnitSpawnCmd { .formation = sov_kps, .icon = UnitIcon::ICON_TANK, .echelon = Echelon::ECHELON_BATTALION }, { 11, 3 });
    (void)HexStateSpawnUnit(hex_state, UnitSpawnCmd { .formation = sov_kps, .icon = UnitIcon::ICON_ART, .echelon = Echelon::ECHELON_BATTALION }, { 13, 3 });
    (void)HexStateSpawnUnit(hex_state, UnitSpawnCmd { .formation = sov_kps, .icon = UnitIcon::ICON_INF, .echelon = Echelon::ECHELON_BATTALION }, { 10, 2 });
    (void)HexStateSpawnUnit(hex_state, UnitSpawnCmd { .formation = sov_kps, .icon = UnitIcon::ICON_INF, .echelon = Echelon::ECHELON_BATTALION }, { 11, 4 });

    // USA: 12th Army Group, V Corps
    const Handle<UnitFormation> usa_hgr = HexStateSpawnFormation(hex_state, CountryTag::TAG_USA, UnitBranch::BRANCH_INFANTRY, Echelon::ECHELON_ARMY, "12AG");
    (void)HexStateSpawnUnit(hex_state, UnitSpawnCmd { .formation = usa_hgr, .icon = UnitIcon::ICON_HQ, .echelon = Echelon::ECHELON_ARMY }, { 17, 1 });

    const Handle<UnitFormation> usa_kps = HexStateSpawnFormation(hex_state, CountryTag::TAG_USA, UnitBranch::BRANCH_INFANTRY, Echelon::ECHELON_CORPS, "5CORPS");
    (void)HexStateSpawnUnit(hex_state, UnitSpawnCmd { .formation = usa_kps, .icon = UnitIcon::ICON_HQ, .echelon = Echelon::ECHELON_CORPS }, { 18, 2 });
    (void)HexStateSpawnUnit(hex_state, UnitSpawnCmd { .formation = usa_kps, .icon = UnitIcon::ICON_INF, .echelon = Echelon::ECHELON_REGIMENT }, { 18, 3 });
    (void)HexStateSpawnUnit(hex_state, UnitSpawnCmd { .formation = usa_kps, .icon = UnitIcon::ICON_TANK, .echelon = Echelon::ECHELON_BATTALION }, { 19, 4 });
}

// 22x8 map: two independent divisions per side, div HQ over battalions
void HexScenarioDivisionClash(HexState& hex_state) {
    constexpr u32 SEED = 3489;
    hex_state.hex_map.Resize({ 20, 8 });
    HexTerrainGenerateType(hex_state, SEED);
    HexTerrainSetRiverBetween(hex_state, HexOffsetToAxial({ 4, 0 }), HexOffsetToAxial({ 5, 9 }));

    constexpr int2 cities[] = {
        HexOffsetToAxial({ 10, 2 }),
        HexOffsetToAxial({ 4, 6 }),
        HexOffsetToAxial({ 3, 3 }),
        HexOffsetToAxial({ 2, 2 }),
    };

    constexpr int2 villages[] = {
        HexOffsetToAxial({ 6, 3 }),
        HexOffsetToAxial({ 6, 7 }),
    };

    for (const int2 axial : cities) { hex_state.hex_map[axial].terrain_feature = TerrainFeature::TERRAIN_FEATURE_CITY; }
    for (const int2 axial : villages) { hex_state.hex_map[axial].terrain_feature = TerrainFeature::TERRAIN_FEATURE_VILLAGE; }

    HexTerrainSetRoadBetween(hex_state, cities[0], cities[1], RoadLevel::ROAD_LEVEL_TRACK);
    HexTerrainSetRoadBetween(hex_state, cities[1], cities[2], RoadLevel::ROAD_LEVEL_SECONDARY);
    HexTerrainSetRoadBetween(hex_state, cities[2], cities[3], RoadLevel::ROAD_LEVEL_SECONDARY);
    HexTerrainSetRoadBetween(hex_state, cities[0], cities[3], RoadLevel::ROAD_LEVEL_PRIMARY);
    HexTerrainSetRoadBetween(hex_state, cities[1], villages[0], RoadLevel::ROAD_LEVEL_TRACK);
    HexTerrainSetRoadBetween(hex_state, cities[1], villages[1], RoadLevel::ROAD_LEVEL_TRACK);

    // GER (attacker): pushed up against the left edge, two independent divisions
    const Handle<UnitFormation> ger_div1 = HexStateSpawnFormation(hex_state, CountryTag::TAG_GER, UnitBranch::BRANCH_INFANTRY, Echelon::ECHELON_DIVISION, "336ID");
    (void)HexStateSpawnUnit(hex_state, UnitSpawnCmd { .formation = ger_div1, .icon = UnitIcon::ICON_HQ, .echelon = Echelon::ECHELON_DIVISION }, { 1, 3 });
    (void)HexStateSpawnUnit(hex_state, UnitSpawnCmd { .formation = ger_div1, .icon = UnitIcon::ICON_INF, .echelon = Echelon::ECHELON_BATTALION }, { 3, 0 });
    (void)HexStateSpawnUnit(hex_state, UnitSpawnCmd { .formation = ger_div1, .icon = UnitIcon::ICON_INF, .echelon = Echelon::ECHELON_BATTALION }, { 3, 2 });
    (void)HexStateSpawnUnit(hex_state, UnitSpawnCmd { .formation = ger_div1, .icon = UnitIcon::ICON_INF, .echelon = Echelon::ECHELON_BATTALION }, { 5, 3 });
    (void)HexStateSpawnUnit(hex_state, UnitSpawnCmd { .formation = ger_div1, .icon = UnitIcon::ICON_ART, .echelon = Echelon::ECHELON_BATTALION }, { 2, 3 });
    (void)HexStateSpawnUnit(hex_state, UnitSpawnCmd { .formation = ger_div1, .icon = UnitIcon::ICON_TANK, .echelon = Echelon::ECHELON_BATTALION }, { 4, 4 });

    const Handle<UnitFormation> ger_div2 = HexStateSpawnFormation(hex_state, CountryTag::TAG_GER, UnitBranch::BRANCH_ARMOR, Echelon::ECHELON_DIVISION, "11PZ");
    (void)HexStateSpawnUnit(hex_state, UnitSpawnCmd { .formation = ger_div2, .icon = UnitIcon::ICON_HQ, .echelon = Echelon::ECHELON_DIVISION }, { 4, 6 });
    (void)HexStateSpawnUnit(hex_state, UnitSpawnCmd { .formation = ger_div2, .icon = UnitIcon::ICON_INF, .echelon = Echelon::ECHELON_BATTALION }, { 6, 4 });
    (void)HexStateSpawnUnit(hex_state, UnitSpawnCmd { .formation = ger_div2, .icon = UnitIcon::ICON_INF, .echelon = Echelon::ECHELON_BATTALION }, { 4, 5 });
    (void)HexStateSpawnUnit(hex_state, UnitSpawnCmd { .formation = ger_div2, .icon = UnitIcon::ICON_INF, .echelon = Echelon::ECHELON_BATTALION }, { 6, 7 });
    (void)HexStateSpawnUnit(hex_state, UnitSpawnCmd { .formation = ger_div2, .icon = UnitIcon::ICON_ENGINEER, .echelon = Echelon::ECHELON_BATTALION }, { 5, 4 });
    (void)HexStateSpawnUnit(hex_state, UnitSpawnCmd { .formation = ger_div2, .icon = UnitIcon::ICON_ENGINEER, .echelon = Echelon::ECHELON_BATTALION }, { 6, 6 });

    // SOV (defender): dug in close to the attackers, two independent divisions
    const Handle<UnitFormation> sov_div1 = HexStateSpawnFormation(hex_state, CountryTag::TAG_SOV, UnitBranch::BRANCH_INFANTRY, Echelon::ECHELON_DIVISION, "119RD");
    (void)HexStateSpawnUnit(hex_state, UnitSpawnCmd { .formation = sov_div1, .icon = UnitIcon::ICON_HQ, .echelon = Echelon::ECHELON_DIVISION }, { 10, 3 });
    (void)HexStateSpawnUnit(hex_state, UnitSpawnCmd { .formation = sov_div1, .icon = UnitIcon::ICON_INF, .echelon = Echelon::ECHELON_BATTALION }, { 6, 0 });
    (void)HexStateSpawnUnit(hex_state, UnitSpawnCmd { .formation = sov_div1, .icon = UnitIcon::ICON_INF, .echelon = Echelon::ECHELON_BATTALION }, { 8, 2 });
    (void)HexStateSpawnUnit(hex_state, UnitSpawnCmd { .formation = sov_div1, .icon = UnitIcon::ICON_INF, .echelon = Echelon::ECHELON_BATTALION }, { 5, 1 });
    (void)HexStateSpawnUnit(hex_state, UnitSpawnCmd { .formation = sov_div1, .icon = UnitIcon::ICON_INF, .echelon = Echelon::ECHELON_BATTALION }, { 7, 3 });
    (void)HexStateSpawnUnit(hex_state, UnitSpawnCmd { .formation = sov_div1, .icon = UnitIcon::ICON_ART, .echelon = Echelon::ECHELON_BATTALION }, { 11, 5 });

    const Handle<UnitFormation> sov_div2 = HexStateSpawnFormation(hex_state, CountryTag::TAG_SOV, UnitBranch::BRANCH_INFANTRY, Echelon::ECHELON_DIVISION, "333RD");
    (void)HexStateSpawnUnit(hex_state, UnitSpawnCmd { .formation = sov_div2, .icon = UnitIcon::ICON_HQ, .echelon = Echelon::ECHELON_DIVISION }, { 11, 6 });
    (void)HexStateSpawnUnit(hex_state, UnitSpawnCmd { .formation = sov_div2, .icon = UnitIcon::ICON_INF, .echelon = Echelon::ECHELON_BATTALION }, { 9, 5 });
    (void)HexStateSpawnUnit(hex_state, UnitSpawnCmd { .formation = sov_div2, .icon = UnitIcon::ICON_INF, .echelon = Echelon::ECHELON_BATTALION }, { 9, 7 });
}
} // namespace hex
