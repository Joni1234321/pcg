#pragma once
#include <SDL3/SDL_render.h>
#include <SDL3_ttf/SDL_ttf.h>

#include "0_engine/g_globals.hpp"
#include "0_engine/r_window_state.hpp"
#include "0_engine/u_collections.hpp"
#include "0_engine/u_colors.hpp"
#include "0_engine/u_fonts.hpp"
#include "0_engine/u_texture.hpp"
#include "r_camera_system.hpp"
#include "r_hex_system.hpp"

namespace pce {

enum class Echelon : u8 { ECHELON_SQUAD, ECHELON_PLATOON, ECHELON_COMPANY, ECHELON_BATTALION, ECHELON_REGIMENT, ECHELON_BRIGADE, ECHELON_DIVISION, ECHELON_CORPS, ECHELON_ARMY };

[[nodiscard]] constexpr String EchelonToString(const Echelon echelon) {
    switch (echelon) {
        case Echelon::ECHELON_SQUAD: return "..";
        case Echelon::ECHELON_PLATOON: return "...";
        case Echelon::ECHELON_COMPANY: return "I";
        case Echelon::ECHELON_BATTALION: return "II";
        case Echelon::ECHELON_REGIMENT: return "III";
        case Echelon::ECHELON_BRIGADE: return "x";
        case Echelon::ECHELON_DIVISION: return "xx";
        case Echelon::ECHELON_CORPS: return "xxx";
        case Echelon::ECHELON_ARMY: return "xxxx";
    }
    __builtin_unreachable();
}

enum class UnitIcon : u8 { ICON_INF, ICON_ART, ICON_HQ, ICON_TANK };

[[nodiscard]] constexpr String UnitIconToString(const UnitIcon icon) {
    switch (icon) {
        case UnitIcon::ICON_INF:  return "inf";
        case UnitIcon::ICON_ART:  return "art";
        case UnitIcon::ICON_HQ:   return "hq";
        case UnitIcon::ICON_TANK: return "tnk";
    }
    __builtin_unreachable();
}
struct CounterStack {
    Color color_background;
    Color color_icon;
    Color color_border;
};
struct Counter {
    int2 axial { };
    Array<CounterStack, 12> stack { };
    Label label_top;
    Label label_center;
    Label label_bottom;
};

inline void RenderCounters(const Pool<Counter>& counters) {
    static constexpr f32 TEXT_MIN_SCALE = 18.0F;
    const CameraState& camera = Singleton::Get<CameraState>();
    const WindowState& window_state = Singleton::Get<WindowState>();
    const ui::FontCollection& font_collection = Singleton::Get<ui::FontCollection>();

    constexpr f32 COUNTER_SIZE = 1.1F;
    const float2 counter_size = float2 { camera.scale * COUNTER_SIZE } * float2 { 1.0F, 0.8F };
    const i32 pt = static_cast<i32>(counter_size.y * 0.25F);
    const ui::Font& font = font_collection.GetFontBold(static_cast<ui::FontSizes>(pt));
    TTF_SetFontWrapAlignment(font, TTF_HORIZONTAL_ALIGN_CENTER);

    for (const Counter& counter : counters) {
        const float2 world = HexAxialToWorld(counter.axial);
        const int2 screen = camera.WorldToScreen(world);
        const float2 counter_center = static_cast<float2>(screen);
        const float2 counter_point = counter_center - counter_size * float2 { 0.5F };

        // draw static counters
        for (i32 i = static_cast<i32>(counter.stack.size()) - 1; i >= 0; i--) {
            const CounterStack& counter_stack = counter.stack[static_cast<u32>(i)];
            if (counter_stack.color_icon.a == 0) { continue; }

            constexpr f32 OFFSET_STACK = 1.0F / 16.0F;
            constexpr f32 OFFSET_SHADOW = OFFSET_STACK * 0.15F;
            constexpr f32 BORDER_THICKNESS = OFFSET_STACK * 0.30F;

            const AABBF area_counter = AABBF::FromCenter(counter_center + float2 { OFFSET_STACK * counter_size.x * static_cast<f32>(i) }, counter_size);

            // shadow
            const AABBF area_shadow = area_counter.WithOffset(float2 { OFFSET_SHADOW * counter_size.x });
            constexpr Color COLOR_SHADOW = colors::ColorWithAlpha(colors::BLACK, 0.5F);
            (void)SDL_SetRenderDrawColor(window_state.renderer, COLOR_SHADOW.r, COLOR_SHADOW.g, COLOR_SHADOW.b, COLOR_SHADOW.a);
            (void)SDL_RenderFillRect(window_state.renderer, area_shadow);

            // border
            (void)SDL_SetRenderDrawColor(window_state.renderer, counter_stack.color_border.r, counter_stack.color_border.g, counter_stack.color_border.b, counter_stack.color_border.a);
            (void)SDL_RenderFillRect(window_state.renderer, area_counter);

            // background
            const AABBF area_background = area_counter.WithPadding(float2 { BORDER_THICKNESS * counter_size.x });;
            (void)SDL_SetRenderDrawColor(window_state.renderer, counter_stack.color_background.r, counter_stack.color_background.g, counter_stack.color_background.b, counter_stack.color_background.a);
            (void)SDL_RenderFillRect(window_state.renderer, area_background);
        }

        // area
        const AABBF area_icon_border = AABBF::FromPoint(counter_point + counter_size * float2 { 0.20F, 0.25F }, counter_size * float2 { 0.6F, 0.4F });
        constexpr Color COLOR_ICON_BORDER { colors::BLACK };
        (void)SDL_SetRenderDrawColor(window_state.renderer, COLOR_ICON_BORDER.r, COLOR_ICON_BORDER.g, COLOR_ICON_BORDER.b, COLOR_ICON_BORDER.a);
        (void)SDL_RenderFillRect(window_state.renderer, area_icon_border);

        const AABBF area_icon = area_icon_border.WithPadding(float2 { camera.scale / 40.0F });
        const Color color_icon = counter.stack[0].color_icon;
        (void)SDL_SetRenderDrawColor(window_state.renderer, color_icon.r, color_icon.g, color_icon.b, color_icon.a);
        (void)SDL_RenderFillRect(window_state.renderer, area_icon);

        // labels
        if (camera.scale >= TEXT_MIN_SCALE) {
            (void)TTF_SetTextFont(counter.label_bottom, font);
            // (void)TTF_SetTextColorFloat(counter.label_bottom, 0.0F, 0.0F, 0.0F, 1.0F);
            (void)TTF_SetTextWrapWidth(counter.label_bottom, static_cast<i32>(counter_size.x));
            (void)TTF_DrawRendererText(counter.label_bottom, counter_point.x, counter_point.y + counter_size.y - static_cast<f32>(pt));

            (void)TTF_SetTextFont(counter.label_center, font);
            (void)TTF_SetTextColorFloat(counter.label_center, 0.0F, 0.0F, 0.0F, 1.0F);
            (void)TTF_SetTextWrapWidth(counter.label_center, static_cast<i32>(counter_size.x));
            // (void)TTF_DrawRendererText(counter.label_center, counter_point.x, counter_point.y + counter_size.y * 0.4F - static_cast<f32>(pt) * 0.3F);

            (void)TTF_SetTextFont(counter.label_top, font);
            // (void)TTF_SetTextColorFloat(counter.label_top, 0.0F, 0.0F, 0.0F, 1.0F);
            (void)TTF_SetTextWrapWidth(counter.label_top, static_cast<i32>(counter_size.x));
            (void)TTF_DrawRendererText(counter.label_top, counter_point.x, counter_point.y);
        }
    }
}

} // namespace pce
