module;

#include <cassert>

export module hex.scenarios;

import std;

import pce.sdl;
import pce.collections;
import pce.std;

import hex.hex;
import hex.types;
import hex.terrain;

export namespace hex {
    UnitToe GetUnitToe(UnitIcon icon) {
        switch (icon) {
            case UnitIcon::ICON_INF: return UnitToe { .move = 16, .squad_inf = 3 * 3 * 3, .squad_art = 3, .squad_tank = 0 };
            case UnitIcon::ICON_ART: return UnitToe { .move = 16, .squad_inf = 3, .squad_art = 4 * 3, .squad_tank = 0 };
            case UnitIcon::ICON_HQ: return UnitToe { .move = 64, .squad_inf = 3, .squad_art = 0, .squad_tank = 0 };
            case UnitIcon::ICON_TANK: return UnitToe { .move = 32, .squad_inf = 3 * 3, .squad_art = 6 * 3, .squad_tank = 5 * 3 };
            case UnitIcon::ICON_ENGINEER: return UnitToe { .move = 16, .squad_inf = 3 * 3 * 3, .squad_art = 3, .squad_tank = 0 };
            default: assert(false); std::unreachable();
        }
    }

    Handle<Unit> HexStateSpawnUnit(HexState& hex_state, const UnitFormation& unit_formation, int2 axial) {
        UnitToe unit_toe = GetUnitToe(unit_formation.icon);
        return hex_state.units.EmplaceBack(
            Unit { .axial = axial, .parent = unit_formation.parent, .tag = unit_formation.tag, .icon = unit_formation.icon, .echelon = unit_formation.echelon, .move = unit_toe.move, .squad_inf = unit_toe.squad_inf, .squad_art = unit_toe.squad_art, .squad_tank = unit_toe.squad_tank, .name = { }, .color = { } });
    }

    void HexScenarioAi(HexState& hex_state) {
        hex_state.hex_map = GenerateTerrainType({ 40, 40 }, 3489);
        // GER: Heeresgruppe → I.Korps → Rgt → Bn
        const Handle<Unit> ger_hgr = HexStateSpawnUnit(hex_state, UnitFormation { .tag = CountryTag::TAG_GER, .icon = UnitIcon::ICON_HQ, .echelon = Echelon::ECHELON_ARMY }, { 0, 2 });
        const Handle<Unit> ger_kps = HexStateSpawnUnit(hex_state, UnitFormation { .parent = ger_hgr, .tag = CountryTag::TAG_GER, .icon = UnitIcon::ICON_HQ, .echelon = Echelon::ECHELON_CORPS }, { 2, 2 });
        const Handle<Unit> ger_r7 = HexStateSpawnUnit(hex_state, UnitFormation { .parent = ger_kps, .tag = CountryTag::TAG_GER, .icon = UnitIcon::ICON_INF, .echelon = Echelon::ECHELON_REGIMENT }, { 4, 1 });
        const Handle<Unit> ger_r8 = HexStateSpawnUnit(hex_state, UnitFormation { .parent = ger_kps, .tag = CountryTag::TAG_GER, .icon = UnitIcon::ICON_INF, .echelon = Echelon::ECHELON_REGIMENT }, { 4, 3 });
        (void)HexStateSpawnUnit(hex_state, UnitFormation { .parent = ger_kps, .tag = CountryTag::TAG_GER, .icon = UnitIcon::ICON_TANK, .echelon = Echelon::ECHELON_BATTALION }, { 5, 2 });
        (void)HexStateSpawnUnit(hex_state, UnitFormation { .parent = ger_kps, .tag = CountryTag::TAG_GER, .icon = UnitIcon::ICON_ART, .echelon = Echelon::ECHELON_BATTALION }, { 3, 2 });
        (void)HexStateSpawnUnit(hex_state, UnitFormation { .parent = ger_r7, .tag = CountryTag::TAG_GER, .icon = UnitIcon::ICON_INF, .echelon = Echelon::ECHELON_BATTALION }, { 6, 1 });
        (void)HexStateSpawnUnit(hex_state, UnitFormation { .parent = ger_r8, .tag = CountryTag::TAG_GER, .icon = UnitIcon::ICON_INF, .echelon = Echelon::ECHELON_BATTALION }, { 6, 3 });

        // SOV: Front → I Gds Corps → Rifle Rgts → Bns
        const Handle<Unit> sov_frt = HexStateSpawnUnit(hex_state, UnitFormation { .tag = CountryTag::TAG_SOV, .icon = UnitIcon::ICON_HQ, .echelon = Echelon::ECHELON_ARMY }, { 14, 3 });
        const Handle<Unit> sov_kps = HexStateSpawnUnit(hex_state, UnitFormation { .parent = sov_frt, .tag = CountryTag::TAG_SOV, .icon = UnitIcon::ICON_HQ, .echelon = Echelon::ECHELON_CORPS }, { 13, 3 });
        const Handle<Unit> sov_r16 = HexStateSpawnUnit(hex_state, UnitFormation { .parent = sov_kps, .tag = CountryTag::TAG_SOV, .icon = UnitIcon::ICON_INF, .echelon = Echelon::ECHELON_REGIMENT }, { 11, 2 });
        const Handle<Unit> sov_r18 = HexStateSpawnUnit(hex_state, UnitFormation { .parent = sov_kps, .tag = CountryTag::TAG_SOV, .icon = UnitIcon::ICON_INF, .echelon = Echelon::ECHELON_REGIMENT }, { 11, 4 });
        (void)HexStateSpawnUnit(hex_state, UnitFormation { .parent = sov_kps, .tag = CountryTag::TAG_SOV, .icon = UnitIcon::ICON_TANK, .echelon = Echelon::ECHELON_BATTALION }, { 10, 3 });
        (void)HexStateSpawnUnit(hex_state, UnitFormation { .parent = sov_kps, .tag = CountryTag::TAG_SOV, .icon = UnitIcon::ICON_ART, .echelon = Echelon::ECHELON_BATTALION }, { 12, 3 });
        (void)HexStateSpawnUnit(hex_state, UnitFormation { .parent = sov_r16, .tag = CountryTag::TAG_SOV, .icon = UnitIcon::ICON_INF, .echelon = Echelon::ECHELON_BATTALION }, { 9, 2 });
        (void)HexStateSpawnUnit(hex_state, UnitFormation { .parent = sov_r18, .tag = CountryTag::TAG_SOV, .icon = UnitIcon::ICON_INF, .echelon = Echelon::ECHELON_BATTALION }, { 9, 4 });
        (void)HexStateSpawnUnit(hex_state, UnitFormation { .parent = sov_frt, .tag = CountryTag::TAG_SOV, .icon = UnitIcon::ICON_INF, .echelon = Echelon::ECHELON_REGIMENT }, { 4, 2 }); // encircled

        // USA: 12th Army Group → V Corps → units
        const Handle<Unit> usa_hgr = HexStateSpawnUnit(hex_state, UnitFormation { .tag = CountryTag::TAG_USA, .icon = UnitIcon::ICON_HQ, .echelon = Echelon::ECHELON_ARMY }, { 17, 1 });
        const Handle<Unit> usa_kps = HexStateSpawnUnit(hex_state, UnitFormation { .parent = usa_hgr, .tag = CountryTag::TAG_USA, .icon = UnitIcon::ICON_HQ, .echelon = Echelon::ECHELON_CORPS }, { 17, 2 });
        (void)HexStateSpawnUnit(hex_state, UnitFormation { .parent = usa_kps, .tag = CountryTag::TAG_USA, .icon = UnitIcon::ICON_INF, .echelon = Echelon::ECHELON_REGIMENT }, { 17, 3 });
        (void)HexStateSpawnUnit(hex_state, UnitFormation { .parent = usa_kps, .tag = CountryTag::TAG_USA, .icon = UnitIcon::ICON_TANK, .echelon = Echelon::ECHELON_BATTALION }, { 17, 4 });
    }

    // 22x8 map: two divisions clashing. Only battalions (and SOV companies) are fielded; everything else is HQ.
    void HexScenarioDivisionClash(HexState& hex_state) {
        hex_state.hex_map = GenerateTerrainType({ 20, 8 }, 3489);

        // GER (attacker): pushed up against the left edge. Division → 2 Regiments → Bns, with divisional art + armor.
        const Handle<Unit> ger_div = HexStateSpawnUnit(hex_state, UnitFormation { .tag = CountryTag::TAG_GER, .icon = UnitIcon::ICON_HQ, .echelon = Echelon::ECHELON_DIVISION }, { 0, 3 });
        const Handle<Unit> ger_r1 = HexStateSpawnUnit(hex_state, UnitFormation { .parent = ger_div, .tag = CountryTag::TAG_GER, .icon = UnitIcon::ICON_HQ, .echelon = Echelon::ECHELON_REGIMENT }, { 1, 1 });
        const Handle<Unit> ger_r2 = HexStateSpawnUnit(hex_state, UnitFormation { .parent = ger_div, .tag = CountryTag::TAG_GER, .icon = UnitIcon::ICON_HQ, .echelon = Echelon::ECHELON_REGIMENT }, { 1, 6 });
        // 6 inf bns split across the two regiments, staggered
        (void)HexStateSpawnUnit(hex_state, UnitFormation { .parent = ger_r1, .tag = CountryTag::TAG_GER, .icon = UnitIcon::ICON_INF, .echelon = Echelon::ECHELON_BATTALION }, { 3, 0 });
        (void)HexStateSpawnUnit(hex_state, UnitFormation { .parent = ger_r1, .tag = CountryTag::TAG_GER, .icon = UnitIcon::ICON_INF, .echelon = Echelon::ECHELON_BATTALION }, { 2, 2 });
        (void)HexStateSpawnUnit(hex_state, UnitFormation { .parent = ger_r1, .tag = CountryTag::TAG_GER, .icon = UnitIcon::ICON_INF, .echelon = Echelon::ECHELON_BATTALION }, { 4, 3 });
        (void)HexStateSpawnUnit(hex_state, UnitFormation { .parent = ger_r2, .tag = CountryTag::TAG_GER, .icon = UnitIcon::ICON_INF, .echelon = Echelon::ECHELON_BATTALION }, { 4, 4 });
        (void)HexStateSpawnUnit(hex_state, UnitFormation { .parent = ger_r2, .tag = CountryTag::TAG_GER, .icon = UnitIcon::ICON_INF, .echelon = Echelon::ECHELON_BATTALION }, { 2, 5 });
        (void)HexStateSpawnUnit(hex_state, UnitFormation { .parent = ger_r2, .tag = CountryTag::TAG_GER, .icon = UnitIcon::ICON_INF, .echelon = Echelon::ECHELON_BATTALION }, { 3, 7 });
        // divisional support: 1 art bn + 1 armor bn
        (void)HexStateSpawnUnit(hex_state, UnitFormation { .parent = ger_div, .tag = CountryTag::TAG_GER, .icon = UnitIcon::ICON_ART, .echelon = Echelon::ECHELON_BATTALION }, { 1, 3 });
        (void)HexStateSpawnUnit(hex_state, UnitFormation { .parent = ger_div, .tag = CountryTag::TAG_GER, .icon = UnitIcon::ICON_TANK, .echelon = Echelon::ECHELON_BATTALION }, { 2, 4 });
        (void)HexStateSpawnUnit(hex_state, UnitFormation { .parent = ger_div, .tag = CountryTag::TAG_GER, .icon = UnitIcon::ICON_ENGINEER, .echelon = Echelon::ECHELON_BATTALION }, { 3, 4 });
        (void)HexStateSpawnUnit(hex_state, UnitFormation { .parent = ger_div, .tag = CountryTag::TAG_GER, .icon = UnitIcon::ICON_ENGINEER, .echelon = Echelon::ECHELON_BATTALION }, { 3, 6 });

        // SOV (defender): dug in close to the attackers. Division → 2 Rifle Regiments → Bns + Coys, with divisional art.
        const Handle<Unit> sov_div = HexStateSpawnUnit(hex_state, UnitFormation { .tag = CountryTag::TAG_SOV, .icon = UnitIcon::ICON_HQ, .echelon = Echelon::ECHELON_DIVISION }, { 9, 3 });
        const Handle<Unit> sov_r1 = HexStateSpawnUnit(hex_state, UnitFormation { .parent = sov_div, .tag = CountryTag::TAG_SOV, .icon = UnitIcon::ICON_HQ, .echelon = Echelon::ECHELON_REGIMENT }, { 8, 1 });
        const Handle<Unit> sov_r2 = HexStateSpawnUnit(hex_state, UnitFormation { .parent = sov_div, .tag = CountryTag::TAG_SOV, .icon = UnitIcon::ICON_HQ, .echelon = Echelon::ECHELON_REGIMENT }, { 8, 6 });
        // 4 inf bns split across the two regiments, staggered
        (void)HexStateSpawnUnit(hex_state, UnitFormation { .parent = sov_r1, .tag = CountryTag::TAG_SOV, .icon = UnitIcon::ICON_INF, .echelon = Echelon::ECHELON_BATTALION }, { 6, 0 });
        (void)HexStateSpawnUnit(hex_state, UnitFormation { .parent = sov_r1, .tag = CountryTag::TAG_SOV, .icon = UnitIcon::ICON_INF, .echelon = Echelon::ECHELON_BATTALION }, { 7, 2 });
        (void)HexStateSpawnUnit(hex_state, UnitFormation { .parent = sov_r2, .tag = CountryTag::TAG_SOV, .icon = UnitIcon::ICON_INF, .echelon = Echelon::ECHELON_BATTALION }, { 7, 5 });
        (void)HexStateSpawnUnit(hex_state, UnitFormation { .parent = sov_r2, .tag = CountryTag::TAG_SOV, .icon = UnitIcon::ICON_INF, .echelon = Echelon::ECHELON_BATTALION }, { 6, 7 });
        // 2 inf companies, both under the first regiment
        (void)HexStateSpawnUnit(hex_state, UnitFormation { .parent = sov_r1, .tag = CountryTag::TAG_SOV, .icon = UnitIcon::ICON_INF, .echelon = Echelon::ECHELON_COMPANY }, { 5, 1 });
        (void)HexStateSpawnUnit(hex_state, UnitFormation { .parent = sov_r1, .tag = CountryTag::TAG_SOV, .icon = UnitIcon::ICON_INF, .echelon = Echelon::ECHELON_COMPANY }, { 6, 3 });
        // divisional support: 1 art bn
        (void)HexStateSpawnUnit(hex_state, UnitFormation { .parent = sov_div, .tag = CountryTag::TAG_SOV, .icon = UnitIcon::ICON_ART, .echelon = Echelon::ECHELON_BATTALION }, { 9, 5 });
    }
}
