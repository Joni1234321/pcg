#include <algorithm>
#include "0_engine/u_util.hpp"
#include "g_arcade.hpp"

#include "0_engine/u_collections.hpp"
#include "0_engine/u_colors.hpp"
#include "0_engine/u_types.hpp"

#include "1_systems/i_input_system.hpp"
#include "1_systems/r_render.hpp"
#include "1_systems/r_ui_node.hpp"
#include "1_systems/t_debug_system.hpp"
#include "1_systems/t_tick_system.hpp"
#include "1_systems/u_animation_system.hpp"

namespace pcg::battlesim {
using namespace pce;
using namespace pce::ui;

// --- Data -------------------------------------------------------------------
struct UnitRow {
    String name;
    u32 before;
    u32 after;
};
struct SideData {
    String division;
    String role;
    u32 cv_before;
    u32 cv_after;
    List<UnitRow> units;
};
struct RoundEntry {
    String phase;
    String attacker_action;
    String defender_action;
    u32 attacker_losses;
    u32 defender_losses;
};
struct BattleInfo {
    SideData attacker;
    SideData defender;
    List<RoundEntry> rounds;
};

struct Division {
    const char* name;
    u32 manpower;
    u32 rifles;
    u32 machine_guns;
    u32 artillery;
    u32 anti_tank_guns;
    u32 tanks;
    u32 trucks;

    [[nodiscard]] constexpr u8 Recon() const { return Squads() / 10U; }
    [[nodiscard]] constexpr u8 Squads() const { return rifles / 10U; }
    [[nodiscard]] constexpr u8 Batteries() const { return artillery / 6U; }
};

constexpr Division soviet_rifle_division_1944 {
    .name = "Soviet Rifle Division",
    .manpower = 9500,
    .rifles = 7000,
    .machine_guns = 180,
    .artillery = 48,
    .anti_tank_guns = 18,
    .tanks = 0,
    .trucks = 120,
};

constexpr Division german_infantry_division_1944 {
    .name = "German Infantry Division",
    .manpower = 12500,
    .rifles = 9000,
    .machine_guns = 250,
    .artillery = 72,
    .anti_tank_guns = 24,
    .tanks = 0,
    .trucks = 900,
};

void SimulateBattle() {
    List defenders { german_infantry_division_1944, german_infantry_division_1944 };
    List attackers { soviet_rifle_division_1944, soviet_rifle_division_1944, soviet_rifle_division_1944 };

    Division attacker = german_infantry_division_1944;
    Division defender = soviet_rifle_division_1944;

    // RANGE 10K main force
    // recon roll for targets.
    u8 attacker_targets = 0;
    const u8 recon_squads = attacker.Recon() / 4U; // commits a quarter of squads
    for (u8 i = 0; i < recon_squads; ++i) {
        // mechanic replace with cards. same colour to spot. Closer to number the better
        u8 recon_dice = Rand(6U);
        if (recon_dice == 0) {
            // trap
            attacker.rifles -= Rand(4, 8); // killed soldiers
        } else if (recon_dice == 1) {
            // contact
            attacker.rifles -= Rand(3, 6); // wounded 3-6 soldiers
            defender.rifles -= Rand(1, 3);
        } else if (recon_dice == 2) {
            // nothing
        } else {
            attacker_targets += recon_dice - 2;
        }
    }

    u8 squads_suppressed = 0;
    // fire missions
    // 1 battery per target
    const u8 fire_mission = std::min(attacker.Batteries(), attacker_targets);
    for (u8 i = 0; i < fire_mission; i++) {
        u8 hit = Rand(6u);
        // roll for hit
        defender.rifles -= hit;
        // roll for wound/kill

        // suppressed squads
        squads_suppressed += hit * 3;
    }

    u8 squads_operating_defender = defender.Squads() - squads_suppressed;
    // then for range 1K. long range fights open
    for (u8 i = 0; i < squads_operating_defender; i++) {
        // mg firing
        u8 hit = Rand(6U);
        attacker.rifles -= hit;
    }
}

// struct MilUnit {
//     u32 inf, art, arm;
// };
//
// constexpr MilUnit InfantryDivision {.inf = 100, .art = 10, .arm = 1};
// constexpr MilUnit InfantryDivision2 {.inf = 80, .art = 20, .arm = 10};
// constexpr MilUnit ArmorDivision {.inf = 30, .art = 20, .arm = 30};

inline BattleInfo MakeBattle() {
    BattleInfo b { };
    b.attacker.division = "11th Panzer Division";
    b.attacker.role = "Attacker";
    b.attacker.cv_before = 42U;
    b.attacker.cv_after = 31U;
    b.attacker.units.EmplaceBack(UnitRow { "Manpower", 14200U, 11850U });
    b.attacker.units.EmplaceBack(UnitRow { "Tanks", 180U, 142U });
    b.attacker.units.EmplaceBack(UnitRow { "Armored Cars", 36U, 30U });
    b.attacker.units.EmplaceBack(UnitRow { "Artillery", 64U, 58U });
    b.attacker.units.EmplaceBack(UnitRow { "Anti-Tank Guns", 48U, 39U });
    b.attacker.units.EmplaceBack(UnitRow { "Anti-Air Guns", 22U, 20U });
    b.attacker.units.EmplaceBack(UnitRow { "Trucks", 1240U, 1108U });
    b.attacker.units.EmplaceBack(UnitRow { "Horses", 380U, 360U });

    b.defender.division = "295th Rifle Division";
    b.defender.role = "Defender";
    b.defender.cv_before = 28U;
    b.defender.cv_after = 12U;
    b.defender.units.EmplaceBack(UnitRow { "Manpower", 9800U, 5230U });
    b.defender.units.EmplaceBack(UnitRow { "Tanks", 12U, 3U });
    b.defender.units.EmplaceBack(UnitRow { "Armored Cars", 8U, 2U });
    b.defender.units.EmplaceBack(UnitRow { "Artillery", 52U, 21U });
    b.defender.units.EmplaceBack(UnitRow { "Anti-Tank Guns", 30U, 11U });
    b.defender.units.EmplaceBack(UnitRow { "Anti-Air Guns", 16U, 7U });
    b.defender.units.EmplaceBack(UnitRow { "Trucks", 240U, 90U });
    b.defender.units.EmplaceBack(UnitRow { "Horses", 1600U, 1100U });

    b.rounds.EmplaceBack(RoundEntry { "Recon", "Forward elements probe the line", "Outposts spot enemy advance", 40U, 60U });
    b.rounds.EmplaceBack(RoundEntry { "Assault", "Panzers engage the main line", "Defenders hold prepared positions", 850U, 1900U });
    b.rounds.EmplaceBack(RoundEntry { "Assault", "Mechanized infantry break through", "Reserves committed", 720U, 1450U });
    b.rounds.EmplaceBack(RoundEntry { "Exploitation", "Armor exploits the gap", "Defenders begin to fall back", 410U, 820U });
    b.rounds.EmplaceBack(RoundEntry { "Pursuit", "Recon battalions pursue", "Rearguard delaying actions", 180U, 340U });
    return b;
}

static constexpr u32 CARD_WIDTH = 1600U;
static constexpr u32 COL_NUM = 110U;
static constexpr u32 COL_LOST = 110U;

// --- Banner frame (always visible) -----------------------------------------
struct BannerFrame : Frame {
    BattleInfo& info;
    explicit BannerFrame(BattleInfo& i) : info(i) { }

    // Full-window centering wrapper
    Handle<Node> outer { B(frame).Node(fill, hug).Center().Fill(colors::CLEAR).Build() };
    Handle<Node> card { B(outer).Node(CARD_WIDTH, hug).Direction(vertical).Gap(0U).Build() };

    // Colorful header strip
    Handle<Node> header { BuildHeader() };

    // 3px border around the whole banner
    Handle<Node> banner_border { B(card).Node(fill, hug).Fill(colors::DARK_SLATE).Padding(3U).Build() };
    Handle<Node> banner { B(banner_border).Node(fill, hug).Fill(colors::DARK_GRAY).Build() };
    Handle<Node> banner_left { BuildSide(banner, info.attacker, colors::WARM_GRAY) };
    Handle<Node> banner_mid { BuildOutcome(banner) };
    Handle<Node> banner_right { BuildSide(banner, info.defender, colors::MAROON) };

    // Details toggle button strip
    Handle<Node> btn_row { B(card).Node(fill, hug).Fill(colors::DARK_DARK_BROWN).Padding2({ 10U, 5U }).Build() };
    Handle<Node> btn_label { B(btn_row).Node(hug, hug).Text("▼  Details", FontSizes::small, colors::SILVER).Build() };

private:
    Handle<Node> BuildHeader() {
        const Handle<Node> bar = B(card).Node(fill, hug).Fill(colors::RUBY_RED).Padding2({ 14U, 10U }).Gap(12U).Build();
        (void)B(bar).Node(hug, hug).Text("⚔", FontSizes::h3, colors::WHITE).Build();
        (void)B(bar).Node(fill, hug).Text("COMBAT REPORT", FontSizes::h2, colors::WHITE).Build();
        (void)B(bar).Node(hug, hug).Text("⚔", FontSizes::h3, colors::WHITE).Right().Build();
        return bar;
    }
    Handle<Node> BuildSide(const Handle<Node> parent, const SideData& side, const Color accent) {
        // 2px border via accent-color padding wrapper
        const Handle<Node> border = B(parent).Node(fill, hug).Fill(accent).Padding(2U).Direction(vertical).Gap(2U).Build();
        const Handle<Node> panel = B(border).Node(fill, hug).Direction(vertical).Gap(0U).Fill(colors::DARK_GRAY).Build();

        // CV at top
        const Color cv_color = side.cv_after < side.cv_before ? colors::SALMON : colors::LIME;
        const Handle<Node> cv = B(panel).Node(fill, hug).Fill(colors::DARK_SLATE).Padding2({ 8U, 6U }).Direction(vertical).Gap(0U).Build();
        (void)B(cv).Node(fill, hug).Text("CV", FontSizes::tiny, colors::SILVER).Build();
        (void)B(cv).Node(fill, hug).Text(std::format("{}  →  {}", side.cv_before, side.cv_after), FontSizes::h2, cv_color).Build();

        // Division name + role
        const Handle<Node> hdr = B(panel).Node(fill, hug).Fill(accent).Padding2({ 8U, 6U }).Direction(vertical).Gap(1U).Build();
        (void)B(hdr).Node(fill, hug).Text(side.division, FontSizes::h4, colors::WHITE).Build();
        (void)B(hdr).Node(fill, hug).Text(side.role, FontSizes::tiny, colors::WHITE_SMOKE).Build();
        return border;
    }
    Handle<Node> BuildOutcome(const Handle<Node> parent) {
        const bool attacker_wins = info.attacker.cv_after > info.defender.cv_after;
        const String& winner = attacker_wins ? info.attacker.division : info.defender.division;

        const Handle<Node> box = B(parent).Node(340U, hug).Direction(vertical).Gap(2U).Fill(colors::DARK_DARK_BROWN).Padding2({ 10U, 8U }).Build();
        (void)B(box).Node(fill, hug).Text(attacker_wins ? "AXIS VICTORY" : "SOVIET VICTORY", FontSizes::tiny, colors::SILVER).Center().Build();
        (void)B(box).Node(fill, hug).Text(winner, FontSizes::h4, colors::DEEP_GOLD).Center().Build();
        (void)B(box).Node(fill, 1U).Fill(colors::DARK_SLATE).Build();
        (void)B(box).Node(fill, hug).Text(std::format("{}:{}", info.attacker.cv_after, info.defender.cv_after), FontSizes::title, colors::WHITE).Center().Build();
        return box;
    }
};

// --- Details frame (toggle) -------------------------------------------------
struct DetailsFrame : Frame {
    BattleInfo& info;
    explicit DetailsFrame(BattleInfo& i) : info(i) { }

    Handle<Node> outer { B(frame).Node(fill, hug).Center().Fill(colors::CLEAR).Build() };
    Handle<Node> card { B(outer).Node(CARD_WIDTH, hug).Direction(vertical).Gap(0U).Fill(colors::DARK_GRAY).Build() };
    Handle<Node> body { B(card).Node(fill, hug).Gap(2U).Fill(colors::DARK_SLATE).Padding(2U).Build() };
    Handle<Node> body_left { BuildUnitTable(body, info.attacker, colors::WARM_GRAY) };
    List<Handle<Node>> round_rows { }; // filled by BuildRoundsPanel — must be declared before body_mid
    Handle<Node> body_mid { BuildRoundsPanel(body) };
    Handle<Node> body_right { BuildUnitTable(body, info.defender, colors::MAROON) };

private:
    Handle<Node> BuildUnitTable(const Handle<Node> parent, const SideData& side, const Color accent) {
        // 2px accent border
        const Handle<Node> border = B(parent).Node(fill, hug).Fill(accent).Padding(2U).Direction(vertical).Gap(0U).Build();
        const Handle<Node> panel = B(border).Node(fill, hug).Direction(vertical).Gap(0U).Fill(colors::WHITE_SMOKE).Build();

        const Handle<Node> label = B(panel).Node(fill, hug).Fill(accent).Padding2({ 6U, 3U }).Build();
        (void)B(label).Node(fill, hug).Text("Unit Strengths", FontSizes::tiny, colors::WHITE).Build();

        const Handle<Node> grid = B(panel).Node(fill, hug).Direction(vertical).Fill(colors::MID_GRAY).Gap(1U).Build();

        const Handle<Node> header = B(grid).Node(fill, hug).Fill(colors::DARK_SLATE).Padding2({ 6U, 3U }).Build();
        (void)B(header).Node(fill, hug).Text("Type", FontSizes::tiny, colors::SILVER).Build();
        (void)B(header).Node(COL_NUM, hug).Text("Before", FontSizes::tiny, colors::SILVER).Right().Build();
        (void)B(header).Node(COL_NUM, hug).Text("After", FontSizes::tiny, colors::SILVER).Right().Build();
        (void)B(header).Node(COL_LOST, hug).Text("Lost", FontSizes::tiny, colors::SALMON).Right().Build();

        for (u32 i = 0U; i < side.units.size(); ++i) {
            const UnitRow& unit = side.units[i];
            const Color row_bg = (i % 2U == 0U) ? colors::WHITE : colors::WHITE_SMOKE;
            const Handle<Node> row = B(grid).Node(fill, hug).Fill(row_bg).Padding2({ 6U, 3U }).Build();
            (void)B(row).Node(fill, hug).Text(unit.name, FontSizes::body, colors::BLACK).Build();
            (void)B(row).Node(COL_NUM, hug).Text(std::format("{}", unit.before), FontSizes::body, colors::DARK_GRAY).Right().Build();
            (void)B(row).Node(COL_NUM, hug).Text(std::format("{}", unit.after), FontSizes::body, colors::DARK_GRAY).Right().Build();
            const u32 lost = unit.before > unit.after ? unit.before - unit.after : 0U;
            (void)B(row).Node(COL_LOST, hug).Text(std::format("-{}", lost), FontSizes::body, colors::RUBY_RED).Right().Build();
        }
        return border;
    }
    Handle<Node> BuildRoundsPanel(const Handle<Node> parent) {
        // 2px dark_slate border
        const Handle<Node> border = B(parent).Node(fill, hug).Fill(colors::DARK_SLATE).Padding(2U).Direction(vertical).Gap(0U).Build();
        const Handle<Node> panel = B(border).Node(fill, hug).Direction(vertical).Gap(0U).Fill(colors::WHITE_SMOKE).Build();

        const Handle<Node> label = B(panel).Node(fill, hug).Fill(colors::DARK_SLATE).Padding2({ 6U, 3U }).Build();
        (void)B(label).Node(fill, hug).Text("Combat Rounds", FontSizes::tiny, colors::SILVER).Build();

        const Handle<Node> grid = B(panel).Node(fill, hug).Direction(vertical).Fill(colors::MID_GRAY).Gap(1U).Build();

        const Handle<Node> rh = B(grid).Node(fill, hug).Fill(colors::DARK_SLATE).Padding2({ 6U, 3U }).Gap(6U).Build();
        (void)B(rh).Node(90U, hug).Text("Phase", FontSizes::tiny, colors::SILVER).Build();
        (void)B(rh).Node(fill, hug).Text("Attacker", FontSizes::tiny, colors::LIGHT_SKY_BLUE).Build();
        (void)B(rh).Node(fill, hug).Text("Defender", FontSizes::tiny, colors::SALMON).Build();
        (void)B(rh).Node(100U, hug).Text("Atk -", FontSizes::tiny, colors::SILVER).Right().Build();
        (void)B(rh).Node(100U, hug).Text("Def -", FontSizes::tiny, colors::SILVER).Right().Build();

        for (u32 i = 0U; i < info.rounds.size(); ++i) {
            const RoundEntry& r = info.rounds[i];
            const Color row_bg = (i % 2U == 0U) ? colors::IVORY : colors::WHITE;
            const Handle<Node> row = B(grid).Node(fill, hug).Fill(row_bg).Padding2({ 6U, 3U }).Gap(6U).Build();
            round_rows.push_back(row);
            (void)B(row).Node(90U, hug).Text(std::format("#{} {}", i + 1U, r.phase), FontSizes::small, colors::DEEP_PURPLE).Build();
            (void)B(row).Node(fill, hug).Text(r.attacker_action, FontSizes::small, colors::NAVY).Build();
            (void)B(row).Node(fill, hug).Text(r.defender_action, FontSizes::small, colors::MAROON).Build();
            (void)B(row).Node(100U, hug).Text(std::format("{}", r.attacker_losses), FontSizes::small, colors::RUBY_RED).Right().Build();
            (void)B(row).Node(100U, hug).Text(std::format("{}", r.defender_losses), FontSizes::small, colors::RUBY_RED).Right().Build();
        }
        return border;
    }
};

// --- Round popup frame (shown on top when a round is clicked) ---------------
struct RoundPopupFrame : Frame {
    BattleInfo& info;
    explicit RoundPopupFrame(BattleInfo& i) : info(i) { }

    // fill×fill overlay so the card can be centered in the window
    Handle<Node> outer { B(frame).Node(fill, fill).Center().Fill(colors::CLEAR).Build() };
    Handle<Node> card { B(outer).Node(760U, hug).Direction(vertical).Gap(0U).Build() };

    Handle<Node> hdr { B(card).Node(fill, hug).Fill(colors::DARK_SLATE).Padding2({ 14U, 10U }).Gap(10U).Build() };
    Handle<Node> phase_lbl { B(hdr).Node(fill, hug).Text("Round", FontSizes::h3, colors::SILVER).Build() };

    Handle<Node> body { B(card).Node(fill, hug).Direction(vertical).Gap(0U).Fill(colors::DARK_GRAY).Padding(14U).Build() };

    Handle<Node> atk_hdr { B(body).Node(fill, hug).Text("Attacker Action", FontSizes::tiny, colors::WARM_GRAY).Build() };
    Handle<Node> atk_txt { B(body).Node(fill, hug).Text("", FontSizes::body, colors::WHITE_SMOKE).Padding2({ 0U, 5U }).Build() };

    Handle<Node> div { B(body).Node(fill, 1U).Fill(colors::DARK_SLATE).Padding2({ 0U, 4U }).Build() };

    Handle<Node> def_hdr { B(body).Node(fill, hug).Text("Defender Action", FontSizes::tiny, colors::SALMON).Build() };
    Handle<Node> def_txt { B(body).Node(fill, hug).Text("", FontSizes::body, colors::WHITE_SMOKE).Padding2({ 0U, 5U }).Build() };

    Handle<Node> loss_row { B(body).Node(fill, hug).Gap(8U).Padding2({ 0U, 10U }).Build() };
    Handle<Node> atk_loss { B(loss_row).Node(fill, hug).Text("", FontSizes::h2, colors::WARM_GRAY).Build() };
    Handle<Node> def_loss { B(loss_row).Node(fill, hug).Text("", FontSizes::h2, colors::RUBY_RED).Right().Build() };

    Handle<Node> hint { B(card).Node(fill, hug).Fill(colors::DARK_DARK_BROWN).Padding2({ 8U, 5U }).Text("click anywhere to dismiss", FontSizes::small, colors::SILVER).Center().Build() };

    void Update(const u32 idx) {
        const RoundEntry& r = info.rounds[idx];
        globalData[tree].node_properties[phase_lbl].text = std::format("Round #{} — {}", idx + 1U, r.phase);
        globalData[tree].node_properties[atk_txt].text = r.attacker_action;
        globalData[tree].node_properties[def_txt].text = r.defender_action;
        globalData[tree].node_properties[atk_loss].text = std::format("Atk  -{}", r.attacker_losses);
        globalData[tree].node_properties[def_loss].text = std::format("Def  -{}", r.defender_losses);
        globalData[tree].MarkDirty();
    }
};

// --- Status bar frame (bottom right, simulation state) ----------------------
struct StatusEntry {
    u32 bar_fill { 0U };
    u32 pct_txt { 0U };
    u32 msg_txt { 0U };
};
struct StatusBarFrame : Frame {
    static constexpr u32 NUM_ENTRIES = 5U;
    static constexpr u32 BAR_WIDTH = 80U;
    static constexpr u32 BAR_HEIGHT = 10U;
    static constexpr u32 PCT_COL = 42U;
    static constexpr u32 PANEL_WIDTH = 440U;

    Handle<Node> outer { B(frame).Node(fill, fill).Alignment(bottom_right).Build() };
    Handle<Node> panel { B(outer).Node(PANEL_WIDTH, hug).Direction(vertical).Gap(1U).Fill(colors::DARK_SLATE).Padding(2U).Build() };

    Array<StatusEntry, NUM_ENTRIES> entries = BuildEntries();

    void Set(const u32 idx, const f32 pct, const String& msg) {
        if (idx >= NUM_ENTRIES) { return; }
        const u32 fill_px = static_cast<u32>(std::clamp(pct, 0.0F, 1.0F) * static_cast<f32>(BAR_WIDTH));
        globalData[tree].styles[Handle<Node> { entries[idx].bar_fill }].width = LayoutLength { fill_px, LayoutLength::fixed };
        globalData[tree].node_properties[Handle<Node> { entries[idx].pct_txt }].text = std::format("{}%", static_cast<u32>(std::clamp(pct, 0.0F, 1.0F) * 100.0F));
        globalData[tree].node_properties[Handle<Node> { entries[idx].msg_txt }].text = msg;
        globalData[tree].MarkDirty();
    }

private:
    Array<StatusEntry, NUM_ENTRIES> BuildEntries() {
        Array<StatusEntry, NUM_ENTRIES> result { };
        for (u32 i = 0U; i < NUM_ENTRIES; ++i) {
            const Color row_bg = (i % 2U == 0U) ? colors::DARK_DARK_BROWN : colors::DARK_GRAY;
            const Handle<Node> row = B(panel).Node(fill, hug).Direction(horizontal).Gap(6U).Fill(row_bg).Padding2({ 6U, 4U }).Build();
            const Handle<Node> bar_bg = B(row).Node(BAR_WIDTH, BAR_HEIGHT).Fill(colors::DARK_SLATE).Build();
            const Handle<Node> bar_fill = B(bar_bg).Node(0U, BAR_HEIGHT).Fill(colors::FOREST_GREEN).Build();
            const Handle<Node> pct = B(row).Node(PCT_COL, hug).Text("0%", FontSizes::tiny, colors::SILVER).Right().Build();
            const Handle<Node> msg = B(row).Node(fill, hug).Text("Idle", FontSizes::tiny, colors::SILVER).Build();
            result[i] = StatusEntry { bar_fill.id, pct.id, msg.id };
        }
        return result;
    }
};

// --- Sim class (like ClickCore) ---------------------------------------------
class BattleSim {
    BattleInfo battle_info { MakeBattle() };
    // popup_frame created first → its NodeTree is tree[0] → rendered last = on top of everything
    RoundPopupFrame popup_frame { battle_info };
    StatusBarFrame status_bar { };
    BannerFrame banner_frame { battle_info };
    DetailsFrame details_frame { battle_info };

    DebugSystem debug_system { };
    TickSystem tick_system { };
    InputSystem input_system { };
    InputNodeSystem node_input_system { };
    AnimationSystem animation_system { };
    RenderNodeSystem node_render_system { };
    ParticleSystem particle_system { };
    RenderWindowSystem window_render_system { };

    b8 details_visible { false };
    b8 banner_measured { false };
    u32 banner_bottom { 0U };

public:
    b8 running { true };

    BattleSim() {
        Singleton::Get<WindowState>().clear_color = colors::DARK_DARK_BROWN;
        globalData[details_frame.tree].SetDisplay(false);
        globalData[popup_frame.tree].SetDisplay(false);
        status_bar.Set(0, 1.00F, "Terrain & weather loaded");
        status_bar.Set(1, 0.86F, "Supply routes computed");
        status_bar.Set(2, 0.55F, "Combat resolution");
        status_bar.Set(3, 0.30F, "Applying losses");
        status_bar.Set(4, 0.08F, "Updating unit states");
    }

    void Tick() {
        tick_system();
        input_system();
        debug_system();
        node_input_system();

        if (Singleton::Get<InputState>().keys_down[SDLK_ESCAPE]) { running = false; }

        // Details button toggle
        const InputState& input = Singleton::Get<InputState>();
        if (input.left_mouse_down) {
            if (globalData[banner_frame.tree].styles[banner_frame.btn_row].IsInside(input.mouse_position)) {
                details_visible = !details_visible;
                globalData[details_frame.tree].SetDisplay(details_visible);
                globalData[popup_frame.tree].SetDisplay(false);
                globalData[banner_frame.tree].node_properties[banner_frame.btn_label].text = details_visible ? "▲  Details" : "▼  Details";
                globalData[banner_frame.tree].MarkDirty();
            }
        }

        // Round row clicks and popup dismiss
        if (input.left_mouse_down) {
            if (globalData[popup_frame.tree].display) {
                globalData[popup_frame.tree].SetDisplay(false);
            } else if (details_visible) {
                for (u32 i = 0U; i < details_frame.round_rows.size(); ++i) {
                    if (globalData[details_frame.tree].styles[details_frame.round_rows[i]].IsInside(input.mouse_position)) {
                        popup_frame.Update(i);
                        globalData[popup_frame.tree].SetDisplay(true);
                        break;
                    }
                }
            }
        }

        animation_system();
        node_render_system();

        // After first banner layout: pin details frame below banner and set clip rect.
        // Use btn_row bounding box bottom edge — frame.bounding_box.h is the full window height.
        if (!banner_measured) {
            const SDL_FRect& bb = globalData[banner_frame.tree].styles[banner_frame.btn_row].bounding_box;
            if (bb.h > 0.0F) {
                banner_bottom = static_cast<u32>(bb.y + bb.h);
                globalData[details_frame.tree].styles[details_frame.frame].padding = uint4 { 0U, banner_bottom, 0U, 0U };
                globalData[details_frame.tree].MarkDirty();
                banner_measured = true;
            }
        }

        window_render_system();
    }
};
} // namespace pcg::battlesim

void pcg::arcade::RunBattleSim() {
    battlesim::BattleSim sim { };
    while (sim.running) { sim.Tick(); }
}
