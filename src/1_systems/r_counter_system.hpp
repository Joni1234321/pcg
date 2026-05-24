#pragma once
#include <SDL3/SDL_render.h>
#include <SDL3_ttf/SDL_ttf.h>

#include "0_engine/g_globals.hpp"
#include "0_engine/r_window_state.hpp"
#include "0_engine/u_collections.hpp"
#include "0_engine/u_colors.hpp"
#include "0_engine/u_fonts.hpp"
#include "0_engine/u_texture.hpp"
#include "0_engine/u_util.hpp"
#include "SDL3/SDL_pixels.h"
#include "SDL3/SDL_rect.h"
#include "r_camera_system.hpp"
#include "r_hex_system.hpp"

namespace pce {

enum class Echelon : u8 {
    ECHELON_SQUAD,
    ECHELON_PLATOON,
    ECHELON_COMPANY,
    ECHELON_BATTALION,
    ECHELON_REGIMENT,
    ECHELON_BRIGADE,
    ECHELON_DIVISION,
    ECHELON_CORPS,
    ECHELON_ARMY,
    ECHELON_HQ,
};

[[nodiscard]] constexpr const char* EchelonToString(const Echelon echelon) {
    switch (echelon) {
        case Echelon::ECHELON_SQUAD:      return "..";
        case Echelon::ECHELON_PLATOON:    return "...";
        case Echelon::ECHELON_COMPANY:    return "I";
        case Echelon::ECHELON_BATTALION:  return "II";
        case Echelon::ECHELON_REGIMENT:   return "III";
        case Echelon::ECHELON_BRIGADE:    return "x";
        case Echelon::ECHELON_DIVISION:   return "xx";
        case Echelon::ECHELON_CORPS:      return "xxx";
        case Echelon::ECHELON_ARMY:       return "xxxx";
        case Echelon::ECHELON_HQ:         return "HQ";
    }
    __builtin_unreachable();
}

struct Counter {
    int2 axial { };
    Array<SDL_Color, 3> colors { };
    String text_bottom;
    String text_top;
    Label label_top;
    Label label_bottom;
    b8 selected { false };
};

inline void RenderCounters(const Span<Counter>& counters) {
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
        for (i32 i = static_cast<i32>(counter.colors.size()) - 1; i >= 0; i--) {
            SDL_Color color_background = counter.colors[static_cast<u32>(i)];

            if (color_background.a == 0) { continue; }

            constexpr f32 STACK_OFFSET = 1.0F / 10.0F;
            constexpr f32 SHADOW_OFFSET = STACK_OFFSET * 0.25F;
            constexpr f32 BORDER_THICKNESS = STACK_OFFSET * 0.20F;

            // shadow
            const AABBF area_shadow = AABBF::FromCenter(counter_center + float2 { SHADOW_OFFSET * counter_size.x }, counter_size);
            const SDL_Color SHADOW_COLOR = counter.selected ? colors::ColorWithAlpha(colors::HEX_SELECT, 1.0F) : colors::ColorWithAlpha(colors::BLACK, 0.5F);
            (void)SDL_SetRenderDrawColor(window_state.renderer, SHADOW_COLOR.r, SHADOW_COLOR.g, SHADOW_COLOR.b, SHADOW_COLOR.a);
            (void)SDL_RenderFillRect(window_state.renderer, area_shadow);

            // border
            AABBF area_border = AABBF::FromCenter(counter_center + float2 { STACK_OFFSET * counter_size.x * static_cast<f32>(i) }, counter_size);
            const SDL_Color border_color = counter.selected ? colors::HEX_SELECT : colors::BLACK;
            (void)SDL_SetRenderDrawColor(window_state.renderer, border_color.r, border_color.g, border_color.b, border_color.a);
            (void)SDL_RenderFillRect(window_state.renderer, area_border);

            // background
            AABBF area_background = AABBF::FromCenter(counter_center + float2 { STACK_OFFSET * counter_size.x * static_cast<f32>(i) }, counter_size - float2 { BORDER_THICKNESS * counter_size.x });
            (void)SDL_SetRenderDrawColor(window_state.renderer, color_background.r, color_background.g, color_background.b, color_background.a);
            (void)SDL_RenderFillRect(window_state.renderer, area_background);
        }

        // area
        const AABBF area_icon = AABBF::FromPoint(counter_point + counter_size * float2 { 0.20F, 0.25F }, counter_size * float2 { 0.6F, 0.4F });
        constexpr Color COLOR_ICON { colors::GRAY_TINT };
        (void)SDL_SetRenderDrawColor(window_state.renderer, COLOR_ICON.r, COLOR_ICON.g, COLOR_ICON.b, COLOR_ICON.a);
        (void)SDL_RenderFillRect(window_state.renderer, area_icon);

        // labels
        if (camera.scale >= TEXT_MIN_SCALE) {
            if (!counter.text_bottom.empty()) {
                (void)TTF_SetTextFont(counter.label_bottom, font);
                (void)TTF_SetTextString(counter.label_bottom, counter.text_bottom.c_str(), counter.text_bottom.size());
                (void)TTF_SetTextWrapWidth(counter.label_bottom, static_cast<i32>(counter_size.x));
                (void)TTF_DrawRendererText(counter.label_bottom, counter_point.x, counter_point.y + counter_size.y - static_cast<f32>(pt));
            }
            if (!counter.text_top.empty()) {
                (void)TTF_SetTextFont(counter.label_top, font);
                (void)TTF_SetTextString(counter.label_top, counter.text_top.c_str(), counter.text_top.size());
                (void)TTF_SetTextWrapWidth(counter.label_top, static_cast<i32>(counter_size.x));
                (void)TTF_DrawRendererText(counter.label_top, counter_point.x, counter_point.y);
            }
        }
    }
}

} // namespace pce
