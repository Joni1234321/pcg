#pragma once

#include <SDL3/SDL.h>
#include <imgui.h>
#include <backends/imgui_impl_sdl3.h>
#include <backends/imgui_impl_sdlrenderer3.h>

#include "0_engine/g_globals.hpp"
#include "0_engine/r_window_state.hpp"
#include "0_engine/u_colors.hpp"
#include "0_engine/u_texture.hpp"
#include "0_engine/u_types.hpp"
#include "g_hex.hpp"

namespace pcg {
using namespace pce;

// All runtime-tunable hex visuals live here. Initial values mirror the original
// constexpr defaults so visual output is unchanged when the window isn't touched.
struct HexTuning {
    TerrainScheme  terrain_scheme  { TerrainScheme::PASTEL_HIGHLAND };
    HexStyle       hex_style       { HexStyle::SPACED };
    TerritoryStyle territory_style { TerritoryStyle::OFF };
    f32 hex_inner        { HEX_INNER_SPACED };
    f32 hex_outer_double { HEX_OUTER_DOUBLE };
    f32 territory_alpha  { TERRITORY_ALPHA };
    f32 double_ring_dark { 0.55F };
    b8 show_decorations  { true };
    f32 decoration_alpha { 0.85F };
    // Map features (cities / villages / roads / rivers) - deterministic noise
    // hashed from axial coordinates so the map reads as something inhabited.
    b8 show_cities    { true };
    b8 show_villages  { true };
    b8 show_roads     { true };
    b8 show_rivers    { true };
    // BORDER mode (Civ6-style). The strip sits on the hex perimeter; the
    // outer edge floats `border_outset` beyond the edge (so two neighbours
    // meet in the middle), the inner edge sits `border_depth` inside the hex,
    // and the inner vertex alpha is scaled by `border_fade` so the colour
    // bleeds into the territory like Civ.
    f32 border_outset     { 0.04F }; // fraction of hex radius beyond the edge
    f32 border_depth      { 0.22F }; // fraction of hex radius inside the hex
    f32 border_fade       { 0.0F  }; // 0 = fully transparent at inner edge, 1 = solid
    f32 border_alpha      { 1.0F  };
    f32 border_fade_curve { 1.0F  }; // gamma applied to the inner-fade alpha
    // Shrink the coloured hex fill below the lattice radius so the territory
    // border (which always references the full lattice edge) can occupy the
    // gap between tiles. 1.0 = touching neighbours, 0.9 = small gap, etc.
    f32 hex_fill_scale    { 1.0F  };

    // Territory OVERLAY: alpha at the hex centre. The current OVERLAY pass
    // fades from `territory_alpha` at the edge to `territory_alpha_center`
    // at the centre so the country colour is most visible near the border.
    f32 territory_alpha_center { 0.0F };

    // Fog of war: every non-player tile farther than `fog_radius` hexes from
    // the closest player-owned tile gets a dark overlay of `fog_alpha`. The
    // ring exactly at `fog_radius` is half-strength to soften the boundary.
    i32 fog_radius { 2 };
    f32 fog_alpha  { 0.55F };
    // When zoomed out the thin sides vanish, so optionally cross-fade into a
    // full hex overlay between these two camera.scale values.
    b8  border_fade_overlay { true };
    f32 fade_scale_max      { 40.0F }; // at/above this scale -> only borders
    f32 fade_scale_min      { 15.0F }; // at/below this scale -> full overlay

    // HSL is the source of truth for the four editable colors below; the
    // matching Color cache is recomputed whenever one of the HSL sliders moves.
    // Storing HSL avoids the u8->f32->u8 round-trip drift that pulled hue/sat
    // toward 0 when a slider was held.
    f32 background_h { 180.0F }, background_s { 0.50F }, background_l { 0.20F };
    f32 ger_h        { 90.0F  }, ger_s        { 0.20F }, ger_l        { 0.30F };
    f32 sov_h        { 2.0F   }, sov_s        { 0.60F }, sov_l        { 0.30F };
    f32 usa_h        { 218.0F }, usa_s        { 0.60F }, usa_l        { 0.30F };

    Color background     { Color::FromHsl(180.0F, 0.50F, 0.20F) };
    Color country_ger    { Color::FromHsl(90.0F,  0.20F, 0.30F) };
    Color country_sov    { Color::FromHsl(2.0F,   0.60F, 0.30F) };
    Color country_usa    { Color::FromHsl(218.0F, 0.60F, 0.30F) };

    // marker so g_hex.cpp can rebuild HexDrawInfo.color when terrain_scheme changes
    b8 scheme_dirty { false };
};

[[nodiscard]] inline f32 HexInnerScaleRuntime(const HexTuning& t) {
    return t.hex_inner;
}

[[nodiscard]] inline Color CountryColorRuntime(const HexTuning& t, const CountryTag tag) {
    switch (tag) {
        case CountryTag::TAG_NONE: return colors::BLACK;
        case CountryTag::TAG_GER:  return t.country_ger;
        case CountryTag::TAG_SOV:  return t.country_sov;
        case CountryTag::TAG_USA:  return t.country_usa;
    }
    return colors::BLACK;
}

struct ImGuiInit {
    explicit ImGuiInit() {
        const WindowState& window_state = Singleton::Get<WindowState>();
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.IniFilename = nullptr; // don't litter cwd with imgui.ini
        ImGui::StyleColorsDark();
        (void)ImGui_ImplSDL3_InitForSDLRenderer(window_state.window, window_state.renderer);
        (void)ImGui_ImplSDLRenderer3_Init(window_state.renderer);
    }
    ~ImGuiInit() {
        ImGui_ImplSDLRenderer3_Shutdown();
        ImGui_ImplSDL3_Shutdown();
        ImGui::DestroyContext();
    }
    ImGuiInit(const ImGuiInit&) = delete;
    ImGuiInit& operator=(const ImGuiInit&) = delete;
    ImGuiInit(ImGuiInit&&) = delete;
    ImGuiInit& operator=(ImGuiInit&&) = delete;
};

// Begin frame. Must run after input polling (events forwarded in InputSystem)
// and before any code wants to draw imgui widgets.
struct ImGuiBeginFrameSystem {
    void operator()() const {
        ImGui_ImplSDLRenderer3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();
    }
};

inline void DrawHexTuningPanel(HexTuning& t) {
    constexpr const char* TERRAIN_NAMES[] = {
        "PASTEL_HIGHLAND", "PARCHMENT_MAP", "ARCTIC_PAPER", "MUTED_TOPO",
        "HOI4_PAPER",      "CIV_VIBRANT",   "BLUEPRINT",    "SEPIA_RECON",
        "SLATE_TABLE",
    };
    constexpr const char* HEX_STYLE_NAMES[]  = { "SOLID", "TIGHT", "SPACED", "AIRY", "DOUBLE_RING" };
    constexpr const char* TERR_STYLE_NAMES[] = { "OFF", "OVERLAY", "BORDER" };

    ImGui::SetNextWindowSize(ImVec2(460.0F, 720.0F), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowPos (ImVec2(20.0F, 20.0F),  ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("Hex tuning")) { ImGui::End(); return; }

    int scheme_idx = static_cast<int>(t.terrain_scheme);
    if (ImGui::Combo("terrain scheme", &scheme_idx, TERRAIN_NAMES, IM_ARRAYSIZE(TERRAIN_NAMES))) {
        t.terrain_scheme = static_cast<TerrainScheme>(scheme_idx);
        t.scheme_dirty   = true;
    }
    int hex_idx = static_cast<int>(t.hex_style);
    if (ImGui::Combo("hex style", &hex_idx, HEX_STYLE_NAMES, IM_ARRAYSIZE(HEX_STYLE_NAMES))) {
        t.hex_style = static_cast<HexStyle>(hex_idx);
    }
    int terr_idx = static_cast<int>(t.territory_style);
    if (ImGui::Combo("territory style", &terr_idx, TERR_STYLE_NAMES, IM_ARRAYSIZE(TERR_STYLE_NAMES))) {
        t.territory_style = static_cast<TerritoryStyle>(terr_idx);
    }
    ImGui::Separator();
    ImGui::SliderFloat("hex inner",      &t.hex_inner,        0.3F, 1.1F);
    ImGui::SliderFloat("hex fill scale", &t.hex_fill_scale,   0.5F, 1.1F);
    ImGui::SliderFloat("DOUBLE outer",   &t.hex_outer_double, 0.3F, 1.1F);
    ImGui::SliderFloat("DOUBLE darken",&t.double_ring_dark, 0.0F, 1.0F);
    ImGui::Separator();
    ImGui::SliderFloat("territory alpha",        &t.territory_alpha,        0.0F, 1.0F);
    ImGui::SliderFloat("territory alpha center", &t.territory_alpha_center, 0.0F, 1.0F);
    ImGui::SliderInt  ("fog radius (tiles)",     &t.fog_radius,             0, 12);
    ImGui::SliderFloat("fog alpha",              &t.fog_alpha,              0.0F, 1.0F);
    if (t.territory_style == TerritoryStyle::BORDER) {
        ImGui::SliderFloat("border outset",     &t.border_outset,     0.0F, 0.3F);
        ImGui::SliderFloat("border depth",      &t.border_depth,      0.0F, 0.6F);
        ImGui::SliderFloat("border fade",       &t.border_fade,       0.0F, 1.0F);
        ImGui::SliderFloat("border fade curve", &t.border_fade_curve, 0.25F, 4.0F);
        ImGui::SliderFloat("border alpha",      &t.border_alpha,      0.0F, 1.0F);
        ImGui::Checkbox  ("fade to overlay when zoomed out", &t.border_fade_overlay);
        if (t.border_fade_overlay) {
            ImGui::SliderFloat("fade scale max (border only)", &t.fade_scale_max, 1.0F, 100.0F);
            ImGui::SliderFloat("fade scale min (full overlay)", &t.fade_scale_min, 1.0F, 100.0F);
        }
    }
    ImGui::Separator();
    ImGui::Checkbox("show terrain decorations", &t.show_decorations);
    ImGui::SliderFloat("decoration alpha", &t.decoration_alpha, 0.0F, 1.0F);
    ImGui::Checkbox("show cities",   &t.show_cities);   ImGui::SameLine();
    ImGui::Checkbox("show villages", &t.show_villages);
    ImGui::Checkbox("show roads",    &t.show_roads);    ImGui::SameLine();
    ImGui::Checkbox("show rivers",   &t.show_rivers);
    ImGui::Separator();

    auto edit_color = [](const char* label, f32& h, f32& s, f32& l, Color& c) {
        ImGui::PushID(label);
        ImGui::TextUnformatted(label);
        const ImVec4 swatch { static_cast<f32>(c.r) / 255.0F, static_cast<f32>(c.g) / 255.0F, static_cast<f32>(c.b) / 255.0F, 1.0F };
        ImGui::SameLine();
        ImGui::ColorButton("##swatch", swatch, ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoPicker, ImVec2(28.0F, 18.0F));
        b8 changed = false;
        changed |= ImGui::SliderFloat("H", &h, 0.0F, 360.0F);
        changed |= ImGui::SliderFloat("S", &s, 0.0F, 1.0F);
        changed |= ImGui::SliderFloat("L", &l, 0.0F, 1.0F);
        ImGui::PopID();
        if (changed) { c = Color::FromHsl(h, s, l); }
    };
    edit_color("background", t.background_h, t.background_s, t.background_l, t.background);
    edit_color("GER",        t.ger_h,        t.ger_s,        t.ger_l,        t.country_ger);
    edit_color("SOV",        t.sov_h,        t.sov_s,        t.sov_l,        t.country_sov);
    edit_color("USA",        t.usa_h,        t.usa_s,        t.usa_l,        t.country_usa);

    ImGui::End();
}

// Issues the tuning window every frame. The actual scheme rebuild + clear color
// sync is handled in g_hex.cpp before render uses these values.
struct HexTuningPanelSystem {
    void operator()() const {
        HexTuning& t = Singleton::Get<HexTuning>();
        DrawHexTuningPanel(t);
    }
};

// Renders all queued imgui draw lists into the SDL renderer. Must run AFTER
// game geometry has been drawn and BEFORE SDL_RenderPresent (i.e. before
// RenderWindowSystem in this codebase, since that present-flips at top).
struct ImGuiRenderSystem {
    void operator()() const {
        const WindowState& window_state = Singleton::Get<WindowState>();
        ImGui::Render();
        ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), window_state.renderer);
    }
};
} // namespace pcg
