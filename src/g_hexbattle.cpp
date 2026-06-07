// =================================================================
//  HEX BATTLE -- C++ port of tools/battlesim/tilesim.ts
//  Regiment combat on a 4x8 hex grid (rendered as a counter grid).
//  Replays a deterministic simulation step-by-step.
// =================================================================

#include <algorithm>
#include <array>
#include <ranges>

#include "g_arcade.hpp"

#include "0_engine/u_collections.hpp"
#include "0_engine/u_logger.hpp"
#include "0_engine/u_util.hpp"

#include "1_systems/i_input_system.hpp"
#include "1_systems/r_render.hpp"
#include "1_systems/r_ui_node.hpp"
#include "1_systems/t_debug_system.hpp"
#include "1_systems/t_tick_system.hpp"

import pce.engine.colors;
import pce.engine.types;

namespace pcg::hexbattle {
using namespace pce;
using namespace pce::ui;

// -- Grid constants ------------------------------------------------
constexpr u32 COLS = 4U;
constexpr u32 ROWS = 8U;
constexpr u32 N_TILES = COLS * ROWS;

static const Array<const char*, ROWS> ROW_NAMES { "A", "B", "C", "D", "E", "F", "G", "H" };

[[nodiscard]] static u32 TileIdx(const u32 c, const u32 r) { return r * COLS + c; }
[[nodiscard]] static u32 ColOf(const u32 i) { return i % COLS; }
[[nodiscard]] static u32 RowOf(const u32 i) { return i / COLS; }

[[nodiscard]] static String Label(const u32 i) { return String { "{}{}", ROW_NAMES[RowOf(i)], ColOf(i) + 1U }; }

// Flat-top odd-q offset hex neighbours
[[nodiscard]] static List<u32> ComputeAdj(const u32 i) {
    List<u32> result { };
    const i32 c = static_cast<i32>(ColOf(i));
    const i32 r = static_cast<i32>(RowOf(i));
    const b8 odd = (c % 2) == 1;
    struct CR {
        i32 c, r;
    };
    const Array<CR, 6> odd_nbrs { { { c, r - 1 }, { c, r + 1 }, { c - 1, r }, { c - 1, r + 1 }, { c + 1, r }, { c + 1, r + 1 } } };
    const Array<CR, 6> even_nbrs { { { c, r - 1 }, { c, r + 1 }, { c - 1, r - 1 }, { c - 1, r }, { c + 1, r - 1 }, { c + 1, r } } };
    const auto& nbrs = odd ? odd_nbrs : even_nbrs;
    for (const auto& [nc, nr] : nbrs) {
        if (nc >= 0 && nc < static_cast<i32>(COLS) && nr >= 0 && nr < static_cast<i32>(ROWS)) { result.push_back(TileIdx(static_cast<u32>(nc), static_cast<u32>(nr))); }
    }
    return result;
}

// -- Types ---------------------------------------------------------
enum class Side : u8 { atk, def, neutral };
enum class UnitType : u8 { inf, recon, support };

struct Tile {
    u32 index;
    String terrain;
    f32 mul;
    Side owner;
};

struct Unit {
    String id;
    String name;
    String rgt;
    Side side;
    UnitType type;
    i32 men;
    i32 rifles;
    i32 mg;
    i32 mortar;
    i32 at;
    i32 morale;
    i32 suppression;
    u32 tile;
    b8 revealed;
    b8 routed;
    b8 entrenched;
    String parent;
    i32 objective; // -1 = none
    b8 objective_complete;
    String stance; // "hold" | "delay" | ""
    i32 delay_turns;
    String activity;
    i32 reorg_turns;
};

enum class LogType : u8 { hdr, info, recon, arty, combat, move, result };
struct LogEntry {
    String text;
    LogType type;
};

struct Frame_ {
    u32 turn;
    List<Unit> units;
    List<Side> owners;
    List<u8> known_atk;
    List<u8> known_def;
    u32 order_set;
    u32 def_order_set;
    i32 atk_guns;
    i32 def_guns;
    u32 log_end;
    b8 over;
};

// -- Helpers -------------------------------------------------------
[[nodiscard]] static i32 D6() { return static_cast<i32>(Rand(1U, 7U)); }
[[nodiscard]] static i32 RandRange(const i32 a, const i32 b) { return a + static_cast<i32>(Rand(static_cast<u32>(b - a + 1))); }
[[nodiscard]] static i32 Clamp(const i32 v, const i32 lo, const i32 hi) { return std::max(lo, std::min(hi, v)); }
[[nodiscard]] static f32 Frand() { return static_cast<f32>(Rand(10000U)) / 10000.0F; }

[[nodiscard]] static String TStr(const u32 t) {
    const u32 total_min = 6U * 60U + (t - 1U) * 30U;
    const u32 hh = total_min / 60U;
    const u32 mm = total_min % 60U;
    return String { "{:02}{:02}", hh, mm };
}

[[nodiscard]] static i32 CV(const Unit& u) { return static_cast<i32>(std::round(u.rifles / 10.0F + static_cast<f32>(u.mg) + static_cast<f32>(u.mortar) * 0.67F + static_cast<f32>(u.at) * 1.5F)); }

static void ApplyCas(Unit& u, const i32 n) {
    const i32 actual = std::min(n, std::max(0, u.men));
    u.men -= actual;
    u.rifles = std::max(0, u.rifles - static_cast<i32>(std::floor(actual * 0.85F)));
    if (actual >= 25) { u.mg = std::max(0, u.mg - actual / 70); }
    if (actual >= 50 && u.mortar > 0 && D6() <= 2) { u.mortar--; }
    if (actual >= 40 && u.at > 0 && D6() <= 2) { u.at--; }
}

// -- Sim ------------------------------------------------------------
struct Sim {
    List<Tile> tiles { };
    List<List<u32>> adj { };
    List<Unit> units { };
    List<LogEntry> full_log { };
    u32 turn { 0U };
    b8 over { false };
    i32 atk_guns { 12 };
    i32 def_guns { 8 };
    List<Frame_> steps { };
    List<u8> known_atk { };
    List<u8> known_def { };
    b8 reserve_committed { false };
    u32 order_set { 1U };
    u32 order_hold_turns { 0U };
    u32 stall_turns { 0U };
    u32 def_order_set { 1U };
    u32 def_order_hold_turns { 0U };

    void AddLog(const String& s, const LogType t) { full_log.push_back(LogEntry { s, t }); }

    [[nodiscard]] u32 CapacityOf(const String& terrain) const {
        const std::string_view tv { terrain.c_str() };
        return (tv == "forest" || tv == "ridge") ? 2U : 1U;
    }

    [[nodiscard]] static f32 TerrainMul(const std::string_view name) {
        if (name == "field") { return 1.0F; }
        if (name == "hill") { return 1.5F; }
        if (name == "village") { return 1.8F; }
        if (name == "forest") { return 2.0F; }
        if (name == "ridge") { return 1.7F; }
        return 1.0F;
    }

    Frame_ Snap() {
        Frame_ f { };
        f.turn = turn;
        f.units = units;
        f.owners.reserve(N_TILES);
        for (const Tile& t : tiles) { f.owners.push_back(t.owner); }
        f.known_atk = known_atk;
        f.known_def = known_def;
        f.order_set = order_set;
        f.def_order_set = def_order_set;
        f.atk_guns = atk_guns;
        f.def_guns = def_guns;
        f.log_end = full_log.size();
        f.over = over;
        return f;
    }

    Unit MkCo(const String& bn, const u32 num, const String& rgt, const Side side, const i32 men, const i32 rifles, const i32 mg, const i32 mortar, const i32 at, const i32 morale, const u32 tile) const {
        Unit u { };
        u.name = String { "{}/{}", num, bn };
        u.rgt = rgt;
        const String rgt_short = rgt; // e.g. "1028th"
        u.id = String { "{}/{}", u.name, rgt_short };
        const b8 reserve = num == 3U;
        u.side = side;
        u.type = UnitType::inf;
        u.men = men;
        u.rifles = rifles;
        u.mg = mg;
        u.mortar = mortar;
        u.at = at;
        u.morale = morale;
        u.suppression = 0;
        u.tile = tile;
        u.revealed = side == Side::atk;
        u.routed = false;
        u.entrenched = side == Side::def;
        u.parent = reserve ? String { "RSV/{}/{}", bn, rgt_short } : String { "{}/{}", bn, rgt_short };
        u.objective = -1;
        u.objective_complete = false;
        u.delay_turns = 0;
        u.activity = reserve ? "Reserve" : (side == Side::def ? "Holding" : "Waiting");
        u.reorg_turns = 0;
        return u;
    }

    [[nodiscard]] b8 ParentStartsWithRSV(const Unit& u) const { return u.parent.size() >= 4U && std::string_view { u.parent.c_str() }.starts_with("RSV/"); }

    void Init() {
        // Terrain
        const Array<const char*, COLS> TERR_DEF { "forest", "ridge", "village", "hill" };
        const Array<const char*, COLS> TERR_MID { "field", "hill", "forest", "field" };
        const Array<const char*, COLS> TERR_ATK { "field", "field", "hill", "field" };

        tiles.clear();
        for (u32 r = 0U; r < ROWS; ++r) {
            Array<const char*, COLS> tab { };
            if (r <= 2U) {
                tab = TERR_DEF;
            } else if (r >= 5U) {
                tab = TERR_ATK;
            } else {
                tab = TERR_MID;
            }
            // shuffle
            Array<const char*, COLS> shuf = tab;
            for (i32 i = static_cast<i32>(COLS) - 1; i > 0; --i) {
                const u32 j = Rand(static_cast<u32>(i + 1));
                std::swap(shuf[i], shuf[j]);
            }
            for (u32 c = 0U; c < COLS; ++c) {
                const u32 idx = TileIdx(c, r);
                Tile t { };
                t.index = idx;
                t.terrain = String { shuf[c] };
                t.mul = TerrainMul(shuf[c]);
                t.owner = r >= 5U ? Side::atk : r <= 3U ? Side::def : Side::neutral;
                tiles.push_back(t);
            }
        }

        // Adjacency
        adj.clear();
        for (u32 i = 0U; i < N_TILES; ++i) { adj.push_back(ComputeAdj(i)); }

        // Units
        units.clear();

        // Defenders
        List<u32> row_c, row_d, row_b;
        for (u32 i = 0U; i < N_TILES; ++i) {
            if (RowOf(i) == 2U) {
                row_c.push_back(i);
            } else if (RowOf(i) == 3U) {
                row_d.push_back(i);
            } else if (RowOf(i) == 1U) {
                row_b.push_back(i);
            }
        }
        // sort row_d by terrain.mul desc
        std::ranges::sort(row_d.data, [&](const u32 a, const u32 b) { return tiles[a].mul > tiles[b].mul; });

        List<u32> def_front;
        for (const u32 t : row_c) { def_front.push_back(t); }
        if (!row_d.empty()) { def_front.push_back(row_d[0]); }
        if (row_d.size() > 1U) {
            def_front.push_back(row_d[1]);
        } else if (!row_d.empty()) {
            def_front.push_back(row_d[0]);
        }

        List<u32> def_rear;
        for (u32 i = 0U; i < 3U && i < row_b.size(); ++i) { def_rear.push_back(row_b[i]); }
        while (def_rear.size() < 3U && !row_b.empty()) { def_rear.push_back(row_b[0]); }

        const Array<const char*, 3> def_bn_names { "I", "II", "III" };
        u32 di = 0U;
        for (u32 bi = 0U; bi < def_bn_names.size(); ++bi) {
            const String bn { def_bn_names[bi] };
            for (u32 n = 1U; n <= 3U; ++n) {
                const u32 tile = n <= 2U ? def_front[di++] : def_rear[bi];
                Unit co = MkCo(bn, n, "1028th", Side::def, 220, 180, 3, 2, 1, 75, tile);
                if (n <= 2U) {
                    co.stance = RowOf(tile) == 3U ? "delay" : "hold";
                    co.objective = static_cast<i32>(tile);
                    co.objective_complete = false;
                    co.delay_turns = 0;
                }
                units.push_back(co);
            }
        }

        // Attackers
        List<u32> atk_assault_cands;
        for (u32 i = 0U; i < N_TILES; ++i) {
            if (RowOf(i) == 4U || RowOf(i) == 5U) { atk_assault_cands.push_back(i); }
        }
        std::ranges::sort(atk_assault_cands.data, [&](const u32 a, const u32 b) { return tiles[a].mul > tiles[b].mul; });
        List<u32> atk_assault;
        for (const u32 t : atk_assault_cands) {
            if (atk_assault.size() >= 6U) { break; }
            const u32 cap = CapacityOf(tiles[t].terrain);
            for (u32 k = 0U; k < cap && atk_assault.size() < 6U; ++k) { atk_assault.push_back(t); }
        }
        while (atk_assault.size() < 6U && !atk_assault_cands.empty()) { atk_assault.push_back(atk_assault_cands[0]); }

        List<u32> atk_rear_cands;
        for (u32 i = 0U; i < N_TILES; ++i) {
            if (RowOf(i) == 6U) { atk_rear_cands.push_back(i); }
        }
        std::ranges::sort(atk_rear_cands.data, [&](const u32 a, const u32 b) { return tiles[a].mul < tiles[b].mul; });
        List<u32> atk_rear;
        for (const u32 t : atk_rear_cands) {
            if (atk_rear.size() >= 3U) { break; }
            atk_rear.push_back(t);
        }
        while (atk_rear.size() < 3U && !atk_rear_cands.empty()) { atk_rear.push_back(atk_rear_cands[0]); }

        const Array<const char*, 3> atk_bn_names { "I", "II", "III" };
        u32 ai = 0U;
        for (u32 bi = 0U; bi < atk_bn_names.size(); ++bi) {
            const String bn { atk_bn_names[bi] };
            for (u32 n = 1U; n <= 3U; ++n) {
                const u32 tile = n <= 2U ? atk_assault[ai++] : atk_rear[bi];
                units.push_back(MkCo(bn, n, "394th", Side::atk, 220, 180, 3, 2, 0, 80, tile));
            }
        }

        // Support
        const u32 lead_tile = atk_assault[0];
        Unit spt { };
        spt.id = "SPT/394";
        spt.name = "SPT";
        spt.rgt = "394th";
        spt.side = Side::atk;
        spt.type = UnitType::support;
        spt.men = 60;
        spt.rifles = 30;
        spt.mg = 1;
        spt.mortar = 4;
        spt.at = 2;
        spt.morale = 85;
        spt.suppression = 0;
        spt.tile = lead_tile;
        spt.revealed = true;
        spt.routed = false;
        spt.entrenched = false;
        spt.parent = "I/394";
        spt.objective = -1;
        spt.objective_complete = false;
        spt.delay_turns = 0;
        spt.activity = "Supporting";
        spt.reorg_turns = 0;
        units.push_back(spt);

        // Issue initial orders
        order_set = 1U;
        order_hold_turns = 0U;
        IssueOrders();

        def_order_set = 1U;
        def_order_hold_turns = 0U;
        IssueDefOrders();

        reserve_committed = false;
        known_atk.clear();
        known_def.clear();
        for (u32 i = 0U; i < N_TILES; ++i) {
            known_atk.push_back(false);
            known_def.push_back(RowOf(i) <= 4U);
        }
        UpdateOwnership();
        UpdateFog();

        atk_guns = 12;
        def_guns = 8;
        turn = 1U;
        over = false;
        full_log.clear();

        SimulateAll();
    }

    void IssueOrders() {
        const u32 target_row = order_set == 1U ? 3U : order_set == 2U ? 2U : 1U;
        List<u32> obj_tiles;
        for (u32 i = 0U; i < N_TILES; ++i) {
            if (RowOf(i) == target_row) { obj_tiles.push_back(i); }
        }
        AddLog(String { "ATK BN CMD: seize row {}", ROW_NAMES[target_row] }, LogType::hdr);

        List<u8> assigned;
        assigned.resize(N_TILES);
        for (u32 i = 0U; i < N_TILES; ++i) { assigned[i] = false; }

        for (Unit& co : units) {
            if (co.side != Side::atk || co.type != UnitType::inf || co.routed || ParentStartsWithRSV(co)) { continue; }
            const i32 col = static_cast<i32>(ColOf(co.tile));
            i32 best = -1;
            i32 best_dist = 999;
            for (const u32 t : obj_tiles) {
                if (assigned[t]) { continue; }
                const i32 dist = std::abs(static_cast<i32>(ColOf(t)) - col) + std::abs(static_cast<i32>(RowOf(t)) - static_cast<i32>(RowOf(co.tile)));
                if (dist < best_dist) {
                    best_dist = dist;
                    best = static_cast<i32>(t);
                }
            }
            if (best == -1) {
                for (const u32 t : obj_tiles) {
                    const i32 dist = std::abs(static_cast<i32>(ColOf(t)) - col) + std::abs(static_cast<i32>(RowOf(t)) - static_cast<i32>(RowOf(co.tile)));
                    if (dist < best_dist) {
                        best_dist = dist;
                        best = static_cast<i32>(t);
                    }
                }
            }
            co.objective = best;
            co.objective_complete = false;
            if (best >= 0) { assigned[static_cast<u32>(best)] = true; }
            AddLog(String { "{} -> OBJ: {}", co.id, Label(static_cast<u32>(best)) }, LogType::info);
        }

        // Reserves: no obj yet
        for (Unit& co : units) {
            if (co.side == Side::atk && ParentStartsWithRSV(co)) {
                co.objective = -1;
                co.objective_complete = false;
            }
        }
    }

    void IssueDefOrders() {
        if (def_order_set == 1U) {
            AddLog("DEF BN CMD: Hold main line (C), delay forward (D)", LogType::hdr);
            for (Unit& co : units) {
                if (co.side != Side::def || co.type != UnitType::inf || co.routed || ParentStartsWithRSV(co)) { continue; }
                co.stance = RowOf(co.tile) >= 3U ? "delay" : "hold";
                co.objective = static_cast<i32>(co.tile);
                co.objective_complete = false;
                co.delay_turns = 0;
                AddLog(String { "{} -> {} at {}", co.id, co.stance, Label(co.tile) }, LogType::info);
            }
            return;
        }

        const u32 target_row = def_order_set == 2U ? 2U : 1U;
        AddLog(String { "DEF BN CMD: Fall back to row {}", ROW_NAMES[target_row] }, LogType::hdr);
        List<u32> row_tiles;
        for (u32 i = 0U; i < N_TILES; ++i) {
            if (RowOf(i) == target_row) { row_tiles.push_back(i); }
        }
        List<u8> assigned;
        assigned.resize(N_TILES);
        for (u32 i = 0U; i < N_TILES; ++i) { assigned[i] = false; }
        for (Unit& co : units) {
            if (co.side != Side::def || co.type != UnitType::inf || co.routed || ParentStartsWithRSV(co)) { continue; }
            const i32 col = static_cast<i32>(ColOf(co.tile));
            i32 best = -1;
            i32 best_dist = 999;
            for (const u32 t : row_tiles) {
                if (assigned[t]) { continue; }
                const i32 dist = std::abs(static_cast<i32>(ColOf(t)) - col);
                if (dist < best_dist) {
                    best_dist = dist;
                    best = static_cast<i32>(t);
                }
            }
            if (best == -1 && !row_tiles.empty()) { best = static_cast<i32>(row_tiles[0]); }
            co.stance = "hold";
            co.objective = best;
            co.objective_complete = false;
            co.delay_turns = 0;
            if (best >= 0) { assigned[static_cast<u32>(best)] = true; }
            AddLog(String { "{} -> HOLD at {}", co.id, Label(static_cast<u32>(best)) }, LogType::info);
        }
    }

    void CheckDefenderOrders() {
        if (def_order_hold_turns > 0U) {
            def_order_hold_turns--;
            if (def_order_hold_turns == 0U) {
                IssueDefOrders();
            } else {
                AddLog(String { "DEF: new orders in {}", def_order_hold_turns }, LogType::info);
            }
            return;
        }
        List<Unit*> def_cos;
        for (Unit& u : units) {
            if (u.side == Side::def && u.type == UnitType::inf && !u.routed && !ParentStartsWithRSV(u)) { def_cos.push_back(&u); }
        }
        if (def_cos.empty()) { return; }

        if (def_order_set == 1U) {
            b8 fwd_lost = true;
            b8 any_fwd = false;
            for (const Unit* u : def_cos) {
                if (u->stance == String { "delay" }) {
                    any_fwd = true;
                    if (RowOf(u->tile) >= 3U) { fwd_lost = false; }
                }
            }
            if (!any_fwd) { fwd_lost = true; }
            if (fwd_lost) {
                def_order_set = 2U;
                def_order_hold_turns = 2U;
                AddLog("DEF: forward screen lost — reorganizing", LogType::hdr);
            }
        } else if (def_order_set == 2U) {
            u32 main_line = 0U, weak = 0U;
            for (const Unit* u : def_cos) {
                if (RowOf(u->tile) == 2U) {
                    main_line++;
                    if (u->morale < 40 || u->men < 120) { weak++; }
                }
            }
            b8 atk_on_c = false;
            for (const Unit& u : units) {
                if (u.side == Side::atk && !u.routed && RowOf(u.tile) <= 2U) {
                    atk_on_c = true;
                    break;
                }
            }
            const b8 main_lost = main_line == 0U || weak >= (main_line + 1U) / 2U;
            if (main_lost || atk_on_c) {
                def_order_set = 3U;
                def_order_hold_turns = 2U;
                AddLog("DEF: main line breaking — falling back to row B", LogType::hdr);
            }
        }
    }

    void CheckOrderCompletion() {
        if (order_hold_turns > 0U) {
            order_hold_turns--;
            if (order_hold_turns == 0U) {
                IssueOrders();
            } else {
                AddLog(String { "ATK: new orders in {}", order_hold_turns }, LogType::info);
            }
            return;
        }
        List<Unit*> atk_cos;
        for (Unit& u : units) {
            if (u.side == Side::atk && u.type == UnitType::inf && !u.routed && u.objective >= 0 && !ParentStartsWithRSV(u)) { atk_cos.push_back(&u); }
        }
        if (atk_cos.empty()) { return; }

        for (Unit* co : atk_cos) {
            if (co->objective < 0 || co->objective_complete) { continue; }
            const u32 obj = static_cast<u32>(co->objective);
            const b8 on_obj = co->tile == obj;
            const b8 past_obj = RowOf(co->tile) < RowOf(obj);
            b8 obj_clear = true;
            for (const Unit& u : units) {
                if (u.side == Side::def && u.tile == obj && !u.routed) {
                    obj_clear = false;
                    break;
                }
            }
            if ((on_obj || past_obj) && obj_clear) {
                co->objective_complete = true;
                AddLog(String { "{} consolidates on {}", co->id, Label(co->tile) }, LogType::result);
            }
        }

        b8 all_done = true;
        for (const Unit* co : atk_cos) {
            if (!co->objective_complete) {
                all_done = false;
                break;
            }
        }
        if (!all_done) { return; }
        if (order_set >= 3U) { return; }
        order_set++;
        AddLog("** NEW ORDERS — advancing **", LogType::hdr);
        IssueOrders();
    }

    void CheckReserveCommit() {
        if (!reserve_committed) {
            b8 has_reserve = false;
            u32 routed_count = 0U;
            b8 all_engaged = true;
            b8 any_fwd = false;
            for (Unit& u : units) {
                if (u.side == Side::atk && ParentStartsWithRSV(u) && !u.routed) { has_reserve = true; }
                if (u.side == Side::atk && !ParentStartsWithRSV(u) && u.type == UnitType::inf) {
                    any_fwd = true;
                    if (u.routed) {
                        routed_count++;
                    } else {
                        b8 engaged = false;
                        for (const Unit& d : units) {
                            if (d.side == Side::def && !d.routed) {
                                if (d.tile == u.tile) {
                                    engaged = true;
                                    break;
                                }
                                if (adj[u.tile].Contains(d.tile)) {
                                    engaged = true;
                                    break;
                                }
                            }
                        }
                        if (!engaged) { all_engaged = false; }
                    }
                }
            }
            if (!has_reserve) {
                reserve_committed = true;
            } else {
                const b8 late_game = turn >= 6U;
                if (routed_count >= 2U || (any_fwd && all_engaged && turn >= 3U) || late_game) {
                    reserve_committed = true;
                    for (Unit& r : units) {
                        if (r.side != Side::atk || !ParentStartsWithRSV(r) || r.routed) { continue; }
                        // strip RSV/
                        std::string p { r.parent.c_str() };
                        if (p.starts_with("RSV/")) { p = p.substr(4); }
                        r.parent = String { std::move(p) };
                        const u32 target_row = order_set == 1U ? 3U : order_set == 2U ? 2U : 1U;
                        i32 best = -1;
                        i32 best_dist = 999;
                        const i32 col = static_cast<i32>(ColOf(r.tile));
                        for (u32 i = 0U; i < N_TILES; ++i) {
                            if (RowOf(i) != target_row) { continue; }
                            const i32 dist = std::abs(static_cast<i32>(ColOf(i)) - col);
                            if (dist < best_dist) {
                                best_dist = dist;
                                best = static_cast<i32>(i);
                            }
                        }
                        r.objective = best;
                        r.objective_complete = false;
                        AddLog(String { "{} (reserve) -> OBJ: {}", r.id, Label(static_cast<u32>(best)) }, LogType::info);
                    }
                    AddLog("Reserves committed!", LogType::info);
                }
            }
        }

        // Defender reserves
        for (Unit& dr : units) {
            if (dr.side != Side::def || !ParentStartsWithRSV(dr) || dr.routed) { continue; }
            std::string p { dr.parent.c_str() };
            const std::string bn_full = p.starts_with("RSV/") ? p.substr(4) : p;
            b8 under_pressure = false;
            for (const Unit& u : units) {
                if (u.side != Side::def || ParentStartsWithRSV(u) || u.routed) { continue; }
                if (std::string_view { u.parent.c_str() } != bn_full) { continue; }
                for (const Unit& a : units) {
                    if (a.side == Side::atk && !a.routed) {
                        if (a.tile == u.tile || adj[u.tile].Contains(a.tile)) {
                            under_pressure = true;
                            break;
                        }
                    }
                }
                if (under_pressure) { break; }
            }
            if (under_pressure) {
                dr.parent = String { bn_full.c_str() };
                AddLog(String { "{} (def reserve) committed", dr.id }, LogType::info);
            }
        }
    }

    void UpdateOwnership() {
        for (u32 i = 0U; i < N_TILES; ++i) {
            b8 atk_here = false;
            b8 def_here = false;
            for (const Unit& u : units) {
                if (u.tile != i || u.routed) { continue; }
                if (u.side == Side::atk) {
                    atk_here = true;
                } else if (u.side == Side::def) {
                    def_here = true;
                }
            }
            if (atk_here && !def_here) {
                tiles[i].owner = Side::atk;
            } else if (def_here && !atk_here) {
                tiles[i].owner = Side::def;
            }
        }
    }

    void UpdateFog() {
        for (u32 i = 0U; i < N_TILES; ++i) {
            if (tiles[i].owner == Side::atk) {
                known_atk[i] = true;
                continue;
            }
            for (const Unit& u : units) {
                if (u.side == Side::atk && !u.routed && (u.tile == i || adj[u.tile].Contains(i))) {
                    known_atk[i] = true;
                    break;
                }
            }
        }
        for (u32 i = 0U; i < N_TILES; ++i) {
            if (tiles[i].owner == Side::def) {
                known_def[i] = true;
                continue;
            }
            for (const Unit& u : units) {
                if (u.side == Side::def && !u.routed && (u.tile == i || adj[u.tile].Contains(i))) {
                    known_def[i] = true;
                    break;
                }
            }
        }
        for (Unit& d : units) {
            if (d.side != Side::def || d.routed || d.revealed) { continue; }
            if (known_atk[d.tile]) {
                d.revealed = true;
                AddLog(String { "Recon: {} spotted at {}", d.id, Label(d.tile) }, LogType::recon);
            }
        }
    }

    [[nodiscard]] b8 InEnemyZoc(const u32 tile, const Side side) const {
        for (const u32 t : adj[tile]) {
            for (const Unit& u : units) {
                if (u.side == side || u.routed || u.tile != t) { continue; }
                if (side == Side::def || u.revealed) { return true; }
            }
        }
        return false;
    }

    [[nodiscard]] u32 MoveCost(const u32 tile, const Side side) const {
        const b8 known = side == Side::atk ? known_atk[tile] : known_def[tile];
        const b8 friendly = tiles[tile].owner == side;
        return (friendly && known) ? 1U : 2U;
    }

    void DoMovement() {
        // Defender movement: fallback to objective
        for (Unit& d : units) {
            if (d.side != Side::def || d.routed || d.type != UnitType::inf || ParentStartsWithRSV(d)) { continue; }
            if (d.objective < 0 || d.tile == static_cast<u32>(d.objective) || d.objective_complete) { continue; }
            const u32 target_row = RowOf(static_cast<u32>(d.objective));
            if (RowOf(d.tile) <= target_row) { continue; }
            List<u32> rear;
            for (const u32 t : adj[d.tile]) {
                if (RowOf(t) >= RowOf(d.tile)) { continue; }
                b8 blocked = false;
                for (const Unit& u : units) {
                    if (u.side == Side::atk && u.tile == t && !u.routed) {
                        blocked = true;
                        break;
                    }
                }
                if (!blocked) { rear.push_back(t); }
            }
            if (rear.empty()) { continue; }
            const i32 obj_col = static_cast<i32>(ColOf(static_cast<u32>(d.objective)));
            std::ranges::sort(rear.data, [&](const u32 a, const u32 b) { return std::abs(static_cast<i32>(ColOf(a)) - obj_col) < std::abs(static_cast<i32>(ColOf(b)) - obj_col); });
            const u32 from = d.tile;
            d.tile = rear[0];
            d.entrenched = false;
            d.activity = "Falling Back";
            AddLog(String { "{} falls back {} -> {}", d.id, Label(from), Label(d.tile) }, LogType::move);
            if (d.tile == static_cast<u32>(d.objective)) {
                d.entrenched = true;
                d.objective_complete = true;
                d.activity = "Digging In";
            }
        }

        // Attacker movement
        for (u32 ui = 0U; ui < units.size(); ++ui) {
            Unit& a = units[ui];
            if (a.side != Side::atk || a.routed) { continue; }
            if (a.type == UnitType::support) {
                Unit* lead = nullptr;
                for (Unit& u : units) {
                    if (u.side != Side::atk || u.parent != a.parent || u.type != UnitType::inf || u.routed) { continue; }
                    if (!lead || RowOf(u.tile) < RowOf(lead->tile)) { lead = &u; }
                }
                if (lead && a.tile != lead->tile) {
                    a.tile = lead->tile;
                    a.activity = "Supporting";
                }
                continue;
            }
            if (ParentStartsWithRSV(a) && !reserve_committed) {
                a.activity = "Reserve";
                continue;
            }
            if (a.reorg_turns > 0) {
                a.reorg_turns--;
                a.activity = "Reorganizing";
                continue;
            }
            if (a.objective_complete && a.objective >= 0) {
                a.activity = "Holding OBJ";
                continue;
            }

            i32 mp = 2;
            a.activity = "Advancing";
            while (mp > 0) {
                auto friendly_count = [&](const u32 t) {
                    u32 c = 0U;
                    for (const Unit& u : units) {
                        if (u.side == a.side && u.tile == t && !u.routed && u.id != a.id) { c++; }
                    }
                    return c;
                };
                auto enemy_on = [&](const u32 t) {
                    for (const Unit& u : units) {
                        if (u.side != a.side && u.tile == t && !u.routed) { return true; }
                    }
                    return false;
                };
                auto passable = [&](const u32 t) { return !enemy_on(t) && friendly_count(t) < CapacityOf(tiles[t].terrain); };

                List<u32> opts;
                for (const u32 t : adj[a.tile]) {
                    if (RowOf(t) < RowOf(a.tile) && passable(t)) { opts.push_back(t); }
                }
                if (opts.empty()) {
                    for (const u32 t : adj[a.tile]) {
                        if (RowOf(t) == RowOf(a.tile) && passable(t)) { opts.push_back(t); }
                    }
                }
                if (opts.empty()) { break; }
                std::ranges::sort(opts.data, [&](const u32 x, const u32 y) {
                    const u32 fx = friendly_count(x), fy = friendly_count(y);
                    if (fx != fy) { return fx < fy; }
                    return tiles[y].mul < tiles[x].mul;
                });
                const u32 dest = opts[0];
                const u32 cost = MoveCost(dest, a.side);
                const b8 entering_zoc = InEnemyZoc(dest, a.side);
                const u32 actual_cost = entering_zoc ? 2U : cost;
                if (static_cast<i32>(actual_cost) > mp) { break; }
                mp -= static_cast<i32>(actual_cost);
                const u32 from = a.tile;
                a.tile = dest;
                AddLog(String { "{} marches {} -> {} (-{}MP)", a.id, Label(from), Label(dest), actual_cost }, LogType::move);
                AutoSpot(units[ui]);
                if (entering_zoc) {
                    a.activity = "Deploying";
                    break;
                }
            }
            if (InEnemyZoc(a.tile, a.side) && a.activity != String { "Deploying" }) { a.activity = "In Contact"; }
        }
    }

    void AutoSpot(Unit& u) {
        List<u32> spot_tiles { u.tile };
        for (const u32 t : adj[u.tile]) { spot_tiles.push_back(t); }
        for (const u32 ti : spot_tiles) {
            if (u.side == Side::atk) {
                known_atk[ti] = true;
            } else {
                known_def[ti] = true;
            }
            for (Unit& e : units) {
                if (e.side == u.side || e.tile != ti || e.routed || e.revealed) { continue; }
                const std::string_view terrain { tiles[ti].terrain.c_str() };
                const i32 pen = terrain == "forest" ? 2 : terrain == "village" ? 1 : 0;
                const i32 roll = D6() + (ti == u.tile ? 3 : 0);
                if (roll - pen >= 3) {
                    e.revealed = true;
                    AddLog(String { "{} spots {} at {}", u.id, e.id, Label(ti) }, LogType::recon);
                }
            }
        }
    }

    void FireAtkArt() {
        if (atk_guns <= 0) { return; }
        List<u32> targets;
        for (const Unit& u : units) {
            if (u.side != Side::def || !u.revealed || u.routed) { continue; }
            if (!targets.Contains(u.tile)) { targets.push_back(u.tile); }
        }
        if (targets.empty()) { return; }
        const i32 per_tgt = static_cast<i32>(std::ceil(static_cast<f32>(atk_guns) / targets.size()));
        i32 used = 0;
        for (const u32 ti : targets) {
            const i32 guns = std::min(per_tgt, atk_guns - used);
            used += guns;
            for (Unit& d : units) {
                if (d.side != Side::def || d.tile != ti || d.routed) { continue; }
                i32 cas = 0;
                for (i32 g = 0; g < guns; ++g) { cas += static_cast<i32>(std::floor(RandRange(1, 4) / tiles[ti].mul)); }
                ApplyCas(d, cas);
                const i32 sl = guns >= 7 ? 3 : guns >= 4 ? 2 : guns >= 2 ? 1 : 0;
                d.suppression = std::max(d.suppression, sl);
                d.morale = Clamp(d.morale - RandRange(3, 7), 0, 100);
                AddLog(String { "Atk Art ({}x 105) -> {}: {} -{} men", guns, Label(ti), d.id, cas }, LogType::arty);
            }
        }
    }

    void FireDefArt() {
        if (def_guns <= 0) { return; }
        List<u32> atk_tiles;
        for (const Unit& u : units) {
            if (u.side != Side::atk || u.type != UnitType::inf || u.routed) { continue; }
            if (!atk_tiles.Contains(u.tile)) { atk_tiles.push_back(u.tile); }
        }
        if (atk_tiles.empty()) { return; }
        const u32 ti = atk_tiles[Rand(static_cast<u32>(atk_tiles.size()))];
        List<Unit*> tgts;
        for (Unit& u : units) {
            if (u.side == Side::atk && u.tile == ti && !u.routed) { tgts.push_back(&u); }
        }
        for (Unit* a : tgts) {
            const i32 cas = std::max(1, static_cast<i32>(std::floor(RandRange(1, 3) * def_guns / std::max<i32>(1, static_cast<i32>(tgts.size())) / tiles[ti].mul)));
            ApplyCas(*a, cas);
            AddLog(String { "Def Art ({}x 76) -> {}: {} -{} men", def_guns, Label(ti), a->id, cas }, LogType::arty);
        }
    }

    void CounterBattery() {
        if (atk_guns > 0 && def_guns > 0 && D6() >= 5) {
            def_guns--;
            AddLog(String { "Counter-battery: def loses 1 gun ({} left)", def_guns }, LogType::arty);
        }
        if (def_guns > 0 && atk_guns > 0 && D6() >= 5) {
            atk_guns--;
            AddLog(String { "Counter-battery: atk loses 1 gun ({} left)", atk_guns }, LogType::arty);
        }
    }

    void ResolveCombat(List<Unit*>& atks, List<Unit*>& defs, const u32 ti) {
        const Tile& tile = tiles[ti];
        i32 atk_cv = 0, def_cv = 0;
        for (const Unit* a : atks) { atk_cv += CV(*a); }
        for (const Unit* d : defs) { def_cv += CV(*d); }
        if (atk_cv <= 0 || def_cv <= 0) { return; }
        if (std::string_view { tile.terrain.c_str() } == "forest") { atk_cv = atk_cv / 2; }

        const f32 ent_mul = std::ranges::any_of(defs.data, [](const Unit* d) { return d->entrenched; }) ? 1.3F : 1.0F;
        i32 t_a_men = 0;
        for (const Unit* a : atks) { t_a_men += a->men; }

        const f32 raw_ratio = static_cast<f32>(atk_cv) / static_cast<f32>(def_cv);
        f32 avg_s = 0.0F;
        for (const Unit* d : defs) { avg_s += static_cast<f32>(d->suppression); }
        avg_s /= static_cast<f32>(defs.size());
        const f32 eff = raw_ratio / (tile.mul * ent_mul) + avg_s * 0.15F;

        f32 ar, dr;
        constexpr f32 base_ar = 0.03F, base_dr = 0.012F;
        if (eff >= 1.0F) {
            ar = std::max(0.005F, base_ar / eff);
            dr = std::min(0.06F, base_dr * eff);
        } else {
            ar = std::min(0.06F, base_ar / eff);
            dr = std::max(0.004F, base_dr * eff);
        }

        const b8 is_delay = std::ranges::any_of(defs.data, [](const Unit* d) { return d->stance == String { "delay" }; });
        if (is_delay) {
            ar *= 0.5F;
            dr *= 0.4F;
            for (Unit* d : defs) {
                if (d->stance == String { "delay" }) { d->delay_turns++; }
            }
        }
        const f32 cas_mod = std::string_view { tile.terrain.c_str() } == "forest" ? 0.6F : 1.0F;

        const i32 a_loss = std::max(1, static_cast<i32>(std::floor(t_a_men * ar * (0.7F + Frand() * 0.6F) * cas_mod)));
        for (Unit* a : atks) {
            const i32 l = std::max(1, static_cast<i32>(std::round(a_loss * static_cast<f32>(a->men) / static_cast<f32>(std::max(1, t_a_men)))));
            ApplyCas(*a, l);
            a->morale = Clamp(a->morale - static_cast<i32>(std::ceil(l / 10.0F)), 0, 100);
            if (a->morale <= 15 || a->men < 80) {
                a->routed = true;
                AddLog(String { "X {} ROUTS", a->id }, LogType::result);
            }
        }

        i32 d_loss = 0;
        for (Unit* d : defs) {
            const i32 l = std::max(1, static_cast<i32>(std::floor(d->men * dr * (0.7F + Frand() * 0.6F) * cas_mod)));
            ApplyCas(*d, l);
            d->morale = Clamp(d->morale - static_cast<i32>(std::ceil(l / 8.0F)), 0, 100);
            d_loss += l;

            if (d->stance == String { "delay" } && d->delay_turns >= 2) {
                List<u32> rear;
                for (const u32 t : adj[d->tile]) {
                    if (RowOf(t) >= RowOf(d->tile)) { continue; }
                    b8 blocked = false;
                    for (const Unit& u2 : units) {
                        if (u2.side == Side::atk && u2.tile == t && !u2.routed) {
                            blocked = true;
                            break;
                        }
                    }
                    if (!blocked) { rear.push_back(t); }
                }
                if (!rear.empty()) {
                    const u32 from = d->tile;
                    d->tile = rear[Rand(static_cast<u32>(rear.size()))];
                    d->entrenched = false;
                    d->delay_turns = 0;
                    AddLog(String { "{} withdraws {} -> {}", d->id, Label(from), Label(d->tile) }, LogType::move);
                    continue;
                }
            }

            if (d->morale <= 25 || d->men < 100) {
                List<u32> rear;
                for (const u32 t : adj[d->tile]) {
                    if (RowOf(t) >= RowOf(d->tile)) { continue; }
                    b8 blocked = false;
                    for (const Unit& u2 : units) {
                        if (u2.side == Side::atk && u2.tile == t && !u2.routed) {
                            blocked = true;
                            break;
                        }
                    }
                    if (!blocked) { rear.push_back(t); }
                }
                if (!rear.empty() && d->morale > 10 && d->men >= 40) {
                    const u32 from = d->tile;
                    d->tile = rear[Rand(static_cast<u32>(rear.size()))];
                    d->entrenched = false;
                    AddLog(String { "{} retreats {} -> {}", d->id, Label(from), Label(d->tile) }, LogType::move);
                } else {
                    d->routed = true;
                    AddLog(String { "X {} ROUTS", d->id }, LogType::result);
                }
            }
        }

        b8 d_holds = false;
        for (const Unit* d : defs) {
            if (!d->routed && d->tile == ti) {
                d_holds = true;
                break;
            }
        }
        b8 a_alive = false;
        for (const Unit* a : atks) {
            if (!a->routed) {
                a_alive = true;
                break;
            }
        }
        String result;
        if (!d_holds && a_alive) {
            for (Unit* a : atks) {
                if (a->routed) { continue; }
                a->tile = ti;
                a->reorg_turns = 1;
                a->activity = "Reorganizing";
            }
            result = String { "TAKES {}", Label(ti) };
        } else if (a_alive) {
            for (Unit* a : atks) {
                if (!a->routed) { a->activity = "Engaging"; }
            }
            for (Unit* d : defs) {
                if (!d->routed && d->tile == ti) { d->activity = d->stance == String { "delay" } ? "Delaying" : "Defending"; }
            }
            result = eff >= 2.0F ? String { "hammers {}", Label(ti) } : eff >= 1.4F ? String { "presses {}", Label(ti) } : String { "repelled from {}", Label(ti) };
        } else {
            result = String { "repelled from {}", Label(ti) };
        }

        String atks_str;
        for (u32 i = 0U; i < atks.size(); ++i) {
            if (i > 0U) { atks_str += "+"; }
            atks_str += atks[i]->id;
        }
        AddLog(String { "{} -> {}: {}  A-{} D-{}", atks_str, Label(ti), result, a_loss, d_loss }, LogType::combat);
    }

    void DoAssaults() {
        // Build attacker list
        List<u32> attacker_indices;
        for (u32 i = 0U; i < units.size(); ++i) {
            Unit& u = units[i];
            if (u.side != Side::atk || u.routed || u.type != UnitType::inf) { continue; }
            if (ParentStartsWithRSV(u) && !reserve_committed) { continue; }
            attacker_indices.push_back(i);
        }
        List<u32> defender_tiles;
        for (const Unit& d : units) {
            if (d.side == Side::def && !d.routed && d.revealed) {
                if (!defender_tiles.Contains(d.tile)) { defender_tiles.push_back(d.tile); }
            }
        }

        // Phase 1: pin
        List<List<u32>> plan_atks;
        List<u32> plan_tiles;
        List<u8> assigned_atk;
        assigned_atk.resize(attacker_indices.size());
        for (u32 i = 0U; i < attacker_indices.size(); ++i) { assigned_atk[i] = false; }

        for (const u32 ti : defender_tiles) {
            List<u32> cands;
            for (u32 ai = 0U; ai < attacker_indices.size(); ++ai) {
                if (assigned_atk[ai]) { continue; }
                if (adj[units[attacker_indices[ai]].tile].Contains(ti)) { cands.push_back(ai); }
            }
            if (cands.empty()) { continue; }
            // pick first (good enough)
            const u32 pick = cands[0];
            assigned_atk[pick] = true;
            List<u32> atks_for_tile { attacker_indices[pick] };
            plan_atks.push_back(std::move(atks_for_tile));
            plan_tiles.push_back(ti);
        }

        // Phase 2: pile on weakest
        if (!plan_tiles.empty()) {
            u32 weakest_idx = 0U;
            i32 weakest_cv = INT32_MAX;
            for (u32 pi = 0U; pi < plan_tiles.size(); ++pi) {
                i32 s = 0;
                for (const Unit& u : units) {
                    if (u.side == Side::def && u.tile == plan_tiles[pi] && !u.routed) { s += CV(u); }
                }
                if (s < weakest_cv) {
                    weakest_cv = s;
                    weakest_idx = pi;
                }
            }
            const u32 weak_tile = plan_tiles[weakest_idx];
            for (u32 ai = 0U; ai < attacker_indices.size(); ++ai) {
                if (assigned_atk[ai]) { continue; }
                if (!adj[units[attacker_indices[ai]].tile].Contains(weak_tile)) { continue; }
                plan_atks[weakest_idx].push_back(attacker_indices[ai]);
                assigned_atk[ai] = true;
            }
        }

        // Resolve combats
        for (u32 pi = 0U; pi < plan_tiles.size(); ++pi) {
            const u32 ti = plan_tiles[pi];
            List<Unit*> atks_p, defs_p;
            for (const u32 ai : plan_atks[pi]) { atks_p.push_back(&units[ai]); }
            for (Unit& d : units) {
                if (d.side == Side::def && d.tile == ti && !d.routed) { defs_p.push_back(&d); }
            }
            if (!defs_p.empty()) { ResolveCombat(atks_p, defs_p, ti); }
        }

        // Low-morale retreat for defenders not in contact
        for (Unit& d : units) {
            if (d.side != Side::def || d.routed || d.morale >= 35) { continue; }
            b8 in_contact = false;
            for (const Unit& u : units) {
                if (u.side == Side::atk && !u.routed && adj[d.tile].Contains(u.tile)) {
                    in_contact = true;
                    break;
                }
            }
            if (in_contact) { continue; }
            List<u32> bk;
            for (const u32 t : adj[d.tile]) {
                if (RowOf(t) < RowOf(d.tile)) { bk.push_back(t); }
            }
            if (bk.empty()) { continue; }
            const u32 from = d.tile;
            d.tile = bk[Rand(static_cast<u32>(bk.size()))];
            d.entrenched = false;
            AddLog(String { "{} retreats {} -> {} (low morale)", d.id, Label(from), Label(d.tile) }, LogType::move);
        }
    }

    void CheckEnd() {
        b8 all_orders_done = false;
        if (order_set >= 3U) {
            all_orders_done = true;
            for (const Unit& u : units) {
                if (u.side == Side::atk && u.type == UnitType::inf && !u.routed && u.objective >= 0 && !u.objective_complete) {
                    all_orders_done = false;
                    break;
                }
            }
        }
        b8 def_alive = false;
        b8 atk_alive = false;
        for (const Unit& u : units) {
            if (u.routed) { continue; }
            if (u.side == Side::def) {
                def_alive = true;
            } else if (u.side == Side::atk && u.type == UnitType::inf) {
                atk_alive = true;
            }
        }
        if (all_orders_done) {
            over = true;
            AddLog("* ATTACKER VICTORY — orders complete", LogType::result);
        } else if (!def_alive) {
            over = true;
            AddLog("* ATTACKER VICTORY — defenders routed", LogType::result);
        } else if (!atk_alive) {
            over = true;
            AddLog("* DEFENDER VICTORY — attackers routed", LogType::result);
        } else if (turn >= 25U) {
            over = true;
            AddLog(String { "* STALEMATE — nightfall ({})", TStr(turn) }, LogType::result);
        }
    }

    void SimulateAll() {
        steps.clear();
        i32 atk_cv = 0, def_cv = 0;
        for (const Unit& u : units) {
            if (u.side == Side::atk) {
                atk_cv += CV(u);
            } else if (u.side == Side::def) {
                def_cv += CV(u);
            }
        }
        AddLog("== 394th Inf Rgt vs 1028th Rifle Rgt ==", LogType::hdr);
        AddLog(String { "394th CV: {}   1028th CV: {} (entrenched)", atk_cv, def_cv }, LogType::info);
        steps.push_back(Snap());

        while (!over) {
            turn++;
            for (Unit& u : units) {
                if (!u.routed) { u.suppression = std::max(0, u.suppression - 1); }
            }
            for (Unit& u : units) {
                if (u.routed) {
                    u.activity = "Routed";
                    continue;
                }
                if (u.side == Side::def) {
                    u.activity = u.entrenched ? (u.stance == String { "delay" } ? "Delaying" : "Holding") : "Moving";
                } else {
                    if (ParentStartsWithRSV(u) && !reserve_committed) {
                        u.activity = "Reserve";
                    } else if (u.reorg_turns > 0) {
                        u.activity = "Reorganizing";
                    } else {
                        u.activity = "Waiting";
                    }
                }
            }
            AddLog(String { "==== T{} {} ====", turn, TStr(turn) }, LogType::hdr);

            CheckReserveCommit();
            FireAtkArt();
            CounterBattery();
            DoMovement();
            DoAssaults();
            FireDefArt();
            UpdateOwnership();
            UpdateFog();
            CheckOrderCompletion();
            CheckDefenderOrders();
            CheckEnd();
            steps.push_back(Snap());

            // Safety: cap simulation to a reasonable upper bound to guarantee termination
            if (turn > 40U) {
                over = true;
                break;
            }
        }
    }
};

// =================================================================
//  RENDERING
// =================================================================

constexpr u32 TILE_W = 130U;
constexpr u32 TILE_H = 100U;

[[nodiscard]] static Color TerrainColor(const std::string_view name) {
    if (name == "field") { return Color { 196U, 188U, 130U, 255U }; }
    if (name == "hill") { return Color { 180U, 140U, 90U, 255U }; }
    if (name == "village") { return Color { 175U, 175U, 175U, 255U }; }
    if (name == "forest") { return Color { 60U, 110U, 60U, 255U }; }
    if (name == "ridge") { return Color { 130U, 100U, 70U, 255U }; }
    return colors::COLOR_GRAY;
}

[[nodiscard]] static Color OwnerBorderColor(const Side s) {
    if (s == Side::atk) { return colors::COLOR_ROYAL_BLUE; }
    if (s == Side::def) { return colors::COLOR_RUBY_RED; }
    return colors::COLOR_DARK_SLATE;
}

struct TileNode {
    Handle<Node> border { U32_MAX };
    Handle<Node> inner { U32_MAX };
    Handle<Node> label { U32_MAX };
    Handle<Node> terrain_lbl { U32_MAX };
    Handle<Node> units_box { U32_MAX };
    Handle<Node> units_text { U32_MAX };
};

struct HexBattleFrame : Frame {
    // Top bar
    Handle<Node> root { B(frame).Node(fill).Direction(vertical).Gap(4U).Padding(8U).Build() };
    Handle<Node> top_bar { B(root).Node(fill, hug).Direction(horizontal).Gap(10U).Fill(colors::COLOR_DARK_SLATE).Padding2({ 10U, 6U }).Build() };
    Handle<Node> title_lbl { B(top_bar).Node(fill, hug).Text("HEX BATTLE — 394th vs 1028th", FontSizes::h3, colors::COLOR_WHITE).Build() };
    Handle<Node> phase_lbl { B(top_bar).Node(500U, hug).Text("...", FontSizes::body, colors::COLOR_GOLD).Build() };
    Handle<Node> step_lbl { B(top_bar).Node(120U, hug).Text("0/0", FontSizes::body, colors::COLOR_SILVER).Right().Build() };

    Handle<Node> controls { B(root).Node(fill, hug).Direction(horizontal).Gap(6U).Padding2({ 0U, 2U }).Build() };
    Handle<Node> btn_prev { B(controls).Node(80U, hug).Fill(colors::COLOR_DARK_GRAY).Padding2({ 8U, 4U }).Text("< Prev", FontSizes::body, colors::COLOR_WHITE).Center().Build() };
    Handle<Node> btn_next { B(controls).Node(80U, hug).Fill(colors::COLOR_DARK_GRAY).Padding2({ 8U, 4U }).Text("Next >", FontSizes::body, colors::COLOR_WHITE).Center().Build() };
    Handle<Node> btn_skip { B(controls).Node(80U, hug).Fill(colors::COLOR_DARK_GRAY).Padding2({ 8U, 4U }).Text("Skip >>", FontSizes::body, colors::COLOR_WHITE).Center().Build() };
    Handle<Node> btn_reset { B(controls).Node(80U, hug).Fill(colors::COLOR_BROWN).Padding2({ 8U, 4U }).Text("Reset", FontSizes::body, colors::COLOR_WHITE).Center().Build() };
    Handle<Node> btn_view { B(controls).Node(120U, hug).Fill(colors::COLOR_ROYAL_BLUE).Padding2({ 8U, 4U }).Text("View: ATK", FontSizes::body, colors::COLOR_WHITE).Center().Build() };

    // Body: grid + side panel
    Handle<Node> body { B(root).Node(fill).Direction(horizontal).Gap(8U).Build() };

    Handle<Node> grid_panel { B(body).Node(fill, fill).Direction(vertical).Gap(0U).Fill(colors::COLOR_BLACK).Padding(4U).Build() };
    Handle<Node> grid_rows { B(grid_panel).Node(fill, fill).Direction(vertical).Gap(2U).Build() };

    Array<Array<TileNode, COLS>, ROWS> tile_nodes { };

    Handle<Node> side { B(body).Node(620U, fill).Direction(vertical).Gap(6U).Build() };
    Handle<Node> tile_info { B(side).Node(fill, 220U).Direction(vertical).Fill(colors::COLOR_DARK_GRAY).Padding(8U).Gap(3U).Build() };
    Handle<Node> tile_info_title { B(tile_info).Node(fill, hug).Text("Click a tile", FontSizes::h4, colors::COLOR_GOLD).Build() };
    Handle<Node> tile_info_body { B(tile_info).Node(fill, fill).Text("", FontSizes::small, colors::COLOR_WHITE).Build() };

    Handle<Node> log_panel { B(side).Node(fill, fill).Direction(vertical).Fill(colors::COLOR_BLACK).Padding(6U).Gap(2U).Build() };
    Handle<Node> log_title { B(log_panel).Node(fill, hug).Text("Turn Log", FontSizes::body, colors::COLOR_GOLD).Build() };
    Handle<Node> log_body { B(log_panel).Node(fill, fill).Text("", FontSizes::tiny, colors::COLOR_WHITE).Build() };

    HexBattleFrame() {
        // Build grid rows × cols
        for (u32 r = 0U; r < ROWS; ++r) {
            const Handle<Node> row = B(grid_rows).Node(fill, hug).Direction(horizontal).Gap(2U).Build();
            // odd columns offset down via per-column padding-top
            for (u32 c = 0U; c < COLS; ++c) {
                const u32 offset_top = (c % 2U) * 25U;
                const Handle<Node> col_wrap = B(row).Node(TILE_W, hug).Direction(vertical).Padding4(uint4 { 0U, offset_top, 0U, 0U }).Build();
                TileNode tn { };
                tn.border = B(col_wrap).Node(TILE_W, TILE_H + 4U).Direction(vertical).Fill(colors::COLOR_DARK_SLATE).Padding(2U).Build();
                tn.inner = B(tn.border).Node(fill, fill).Direction(vertical).Fill(colors::COLOR_GRAY).Padding(3U).Gap(1U).Build();
                const Handle<Node> hdr = B(tn.inner).Node(fill, hug).Direction(horizontal).Gap(4U).Build();
                tn.label = B(hdr).Node(40U, hug).Text("--", FontSizes::small, colors::COLOR_WHITE).Build();
                tn.terrain_lbl = B(hdr).Node(fill, hug).Text("", FontSizes::tiny, colors::COLOR_BLACK).Right().Build();
                tn.units_box = B(tn.inner).Node(fill, fill).Direction(vertical).Fill(colors::COLOR_CLEAR).Build();
                tn.units_text = B(tn.units_box).Node(fill, fill).Text("", FontSizes::tiny, colors::COLOR_WHITE).Build();
                tile_nodes[r][c] = tn;
            }
        }
    }
};

class HexBattle {
    Sim sim { };
    HexBattleFrame ui { };

    RenderWindowSystem present_system { };
    TickSystem tick_system { };
    InputSystem input_system { };
    InputNodeSystem node_input_system { };
    RenderNodeSystem node_render_system { };
    DebugSystem debug_system { };

    u32 step_idx { 0U };
    Side view_side { Side::atk };
    i32 selected_tile { -1 };

    b8 prev_left_mouse_down { false };
    UnorderedMap<SDL_Keycode, b8> prev_keys { };

public:
    b8 running { true };

    HexBattle() {
        Singleton::Get<WindowState>().clear_color = colors::COLOR_DARK_DARK_BROWN;
        sim.Init();
        step_idx = 0U;
        Refresh();
    }

    void Tick() {
        tick_system();
        input_system();
        debug_system();
        node_input_system();

        const InputState& input = Singleton::Get<InputState>();
        if (input.keys_down.contains(SDLK_ESCAPE) && input.keys_down.at(SDLK_ESCAPE)) {
            running = false;
            return;
        }

        HandleInput(input);

        node_render_system();
        present_system();
    }

private:
    [[nodiscard]] static b8 KeyPressed(const UnorderedMap<SDL_Keycode, b8>& keys, const SDL_Keycode k) { return keys.contains(k) && keys.at(k); }

    void HandleInput(const InputState& input) {
        // Buttons
        if (input.left_mouse_down) {
            if (globalData[ui.tree].styles[ui.btn_prev].IsInside(input.mouse_position)) {
                Prev();
                return;
            }
            if (globalData[ui.tree].styles[ui.btn_next].IsInside(input.mouse_position)) {
                Next();
                return;
            }
            if (globalData[ui.tree].styles[ui.btn_skip].IsInside(input.mouse_position)) {
                Skip();
                return;
            }
            if (globalData[ui.tree].styles[ui.btn_reset].IsInside(input.mouse_position)) {
                Reset();
                return;
            }
            if (globalData[ui.tree].styles[ui.btn_view].IsInside(input.mouse_position)) {
                ToggleView();
                return;
            }

            // Tile click
            for (u32 r = 0U; r < ROWS; ++r) {
                for (u32 c = 0U; c < COLS; ++c) {
                    if (globalData[ui.tree].styles[ui.tile_nodes[r][c].border].IsInside(input.mouse_position)) {
                        selected_tile = static_cast<i32>(TileIdx(c, r));
                        UpdateTileInfo();
                        return;
                    }
                }
            }
        }

        // Keys
        const auto& keys = input.keys_down;
        if (KeyPressed(keys, SDLK_RIGHT) || KeyPressed(keys, SDLK_D) || KeyPressed(keys, SDLK_PERIOD)) {
            Next();
        } else if (KeyPressed(keys, SDLK_LEFT) || KeyPressed(keys, SDLK_A) || KeyPressed(keys, SDLK_COMMA)) {
            Prev();
        } else if (KeyPressed(keys, SDLK_END) || KeyPressed(keys, SDLK_S)) {
            Skip();
        } else if (KeyPressed(keys, SDLK_HOME) || KeyPressed(keys, SDLK_W)) {
            step_idx = 0U;
            Refresh();
        } else if (KeyPressed(keys, SDLK_R)) {
            Reset();
        } else if (KeyPressed(keys, SDLK_V)) {
            ToggleView();
        }
    }

    void Prev() {
        if (step_idx > 0U) {
            step_idx--;
            Refresh();
        }
    }
    void Next() {
        if (step_idx + 1U < sim.steps.size()) {
            step_idx++;
            Refresh();
        }
    }
    void Skip() {
        if (!sim.steps.empty()) {
            step_idx = sim.steps.size() - 1U;
            Refresh();
        }
    }
    void Reset() {
        sim = Sim { };
        sim.Init();
        step_idx = 0U;
        selected_tile = -1;
        Refresh();
    }
    void ToggleView() {
        view_side = view_side == Side::atk ? Side::def : Side::atk;
        const String txt = view_side == Side::atk ? String { "View: ATK" } : String { "View: DEF" };
        globalData[ui.tree].node_properties[ui.btn_view].text = txt;
        globalData[ui.tree].styles[ui.btn_view].background_color = view_side == Side::atk ? colors::COLOR_ROYAL_BLUE : colors::COLOR_RUBY_RED;
        Refresh();
    }

    void Refresh() {
        if (sim.steps.empty()) { return; }
        const Frame_& f = sim.steps[step_idx];

        // Phase label
        const char* order_lbl = f.order_set == 1U ? "Seize D" : f.order_set == 2U ? "Seize C" : "Seize B";
        const char* def_lbl = f.def_order_set == 1U ? "Hold C+D" : f.def_order_set == 2U ? "Hold C" : "Hold B";
        const String phase = f.over ? String { "RESULT — turn {}", f.turn } : String { "T{} {}  |  ATK: {}  |  DEF: {}", f.turn, TStr(f.turn), order_lbl, def_lbl };
        globalData[ui.tree].node_properties[ui.phase_lbl].text = phase;
        globalData[ui.tree].node_properties[ui.step_lbl].text = String { "{} / {}", step_idx + 1U, sim.steps.size() };

        // Tiles
        for (u32 i = 0U; i < N_TILES; ++i) {
            const u32 r = RowOf(i);
            const u32 c = ColOf(i);
            const TileNode& tn = ui.tile_nodes[r][c];
            const Tile& t = sim.tiles[i];
            const b8 known = view_side == Side::atk ? f.known_atk[i] : f.known_def[i];

            globalData[ui.tree].styles[tn.border].background_color = OwnerBorderColor(f.owners[i]);
            globalData[ui.tree].styles[tn.inner].background_color = known ? TerrainColor(std::string_view { t.terrain.c_str() }) : colors::COLOR_DARK_GRAY;
            globalData[ui.tree].node_properties[tn.label].text = Label(i);
            globalData[ui.tree].node_properties[tn.terrain_lbl].text = known ? String { "{} x{:.1f}", t.terrain, t.mul } : String { "???" };

            // Units text
            String units_txt;
            for (const Unit& u : f.units) {
                if (u.tile != i || u.routed) { continue; }
                const b8 show = u.side == view_side || known;
                if (!show) { continue; }
                if (u.side != view_side && !u.revealed) { continue; }
                if (!units_txt.empty()) { units_txt += "\n"; }
                const char* tag = u.side == Side::atk ? "[A]" : "[D]";
                const char marker = u.entrenched ? '#' : (u.routed ? 'X' : '*');
                units_txt += String { "{}{} {} m{} cv{}", tag, marker, u.id, u.men, CV(u) };
            }
            globalData[ui.tree].node_properties[tn.units_text].text = units_txt;
        }

        // Log: combat-relevant entries for the current turn
        String log_txt;
        const u32 prev_end = step_idx > 0U ? sim.steps[step_idx - 1U].log_end : 0U;
        u32 lines = 0U;
        for (u32 i = prev_end; i < f.log_end && lines < 28U; ++i) {
            const LogEntry& e = sim.full_log[i];
            if (!log_txt.empty()) { log_txt += "\n"; }
            log_txt += e.text;
            lines++;
        }
        globalData[ui.tree].node_properties[ui.log_body].text = log_txt;

        UpdateTileInfo();
        globalData[ui.tree].MarkDirty();
    }

    void UpdateTileInfo() {
        if (selected_tile < 0 || sim.steps.empty()) {
            globalData[ui.tree].node_properties[ui.tile_info_title].text = "Click a tile";
            globalData[ui.tree].node_properties[ui.tile_info_body].text = "";
            return;
        }
        const u32 ti = static_cast<u32>(selected_tile);
        const Frame_& f = sim.steps[step_idx];
        const Tile& t = sim.tiles[ti];
        const b8 known = view_side == Side::atk ? f.known_atk[ti] : f.known_def[ti];

        globalData[ui.tree].node_properties[ui.tile_info_title].text = String { "Hex {}  ({} x{:.1f})", Label(ti), t.terrain, t.mul };

        String body;
        body += String { "Owner: {}", f.owners[ti] == Side::atk ? "Attacker" : f.owners[ti] == Side::def ? "Defender" : "Neutral" };
        body += "\n";
        body += known ? "Known" : "Unknown";
        body += "\n\n";

        for (const Unit& u : f.units) {
            if (u.tile != ti) { continue; }
            const b8 show = u.side == view_side || known;
            if (!show) { continue; }
            if (u.side != view_side && !u.revealed) {
                body += "??? (hostile unidentified)\n";
                continue;
            }
            const char* side_lbl = u.side == Side::atk ? "ATK" : "DEF";
            body += String { "{} {}  men:{}  cv:{}  morale:{}  {}\n", side_lbl, u.id, u.men, CV(u), u.morale, u.activity };
            if (u.entrenched) { body += "  entrenched\n"; }
            if (u.routed) { body += "  ROUTED\n"; }
        }
    }
};
} // namespace pcg::hexbattle

void pcg::arcade::RunHexBattle() {
    hexbattle::HexBattle game { };
    while (game.running) { game.Tick(); }
}
