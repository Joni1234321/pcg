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

struct Counter {
    int2 axial;
    SDL_Color color;
    String text_bottom;
    String text_top;
    Label label_top;
    Label label_bottom;
};

struct CounterState {
    List<Counter> counters;
};

struct RenderCounterSystem {
    static constexpr f32 TEXT_MIN_SCALE = 18.0F;

    void operator()() {
        const CameraState& camera = Singleton::Get<CameraState>();
        const WindowState& window_state = Singleton::Get<WindowState>();
        const CounterState& counter_state = Singleton::Get<CounterState>();
        const ui::FontCollection& font_collection = Singleton::Get<ui::FontCollection>();

        constexpr f32 COUNTER_SIZE = 0.8F;
        const f32 width = camera.scale * COUNTER_SIZE;
        const f32 height = camera.scale * COUNTER_SIZE * 0.8F;
        const int pt = math::Max(6, static_cast<int>(width * 0.17F));
        const ui::Font& font = font_collection.GetFont(static_cast<ui::FontSizes>(pt));

        for (const Counter& counter : counter_state.counters) {
            const float2 world = HexAxialToWorld(counter.axial);
            const int2 screen = camera.WorldToScreen(world);

            const SDL_FRect counter_area {
                .x = static_cast<f32>(screen.x) - width * 0.5F,
                .y = static_cast<f32>(screen.y) - height * 0.5F,
                .w = width,
                .h = height,
            };

            // shadow
            constexpr f32 SHADOW_OFFSET = 4.0F;
            const SDL_FRect shadow_area {
                .x = counter_area.x + SHADOW_OFFSET,
                .y = counter_area.y + SHADOW_OFFSET,
                .w = counter_area.w,
                .h = counter_area.h,
            };
            constexpr SDL_Color SHADOW_COLOR = colors::ColorWithAlpha(colors::black, 128);
            (void)SDL_SetRenderDrawColor(window_state.renderer, SHADOW_COLOR.r, SHADOW_COLOR.g, SHADOW_COLOR.b, SHADOW_COLOR.a);
            (void)SDL_RenderFillRect(window_state.renderer, &shadow_area);

            // background
            (void)SDL_SetRenderDrawColor(window_state.renderer, counter.color.r, counter.color.g, counter.color.b, counter.color.a);
            (void)SDL_RenderFillRect(window_state.renderer, &counter_area);

            // border
            constexpr SDL_Color BORDER_COLOR { colors::white_smoke };
            (void)SDL_SetRenderDrawColor(window_state.renderer, BORDER_COLOR.r, BORDER_COLOR.g, BORDER_COLOR.b, BORDER_COLOR.a);
            (void)SDL_RenderRect(window_state.renderer, &counter_area);

            // area
            const SDL_FRect icon_area {
                .x = counter_area.x + counter_area.w * 0.20F,
                .y = counter_area.y + counter_area.h * 0.25F,
                .w = counter_area.w * 0.60F,
                .h = counter_area.h * 0.40F,
            };
            constexpr SDL_Color AREA_COLOR { colors::gray_tint };
            (void)SDL_SetRenderDrawColor(window_state.renderer, AREA_COLOR.r, AREA_COLOR.g, AREA_COLOR.b, AREA_COLOR.a);
            (void)SDL_RenderFillRect(window_state.renderer, &icon_area);

            // labels
            if (camera.scale >= TEXT_MIN_SCALE) {
                if (!counter.text_bottom.empty()) {
                    (void)TTF_SetTextFont(counter.label_bottom, font.ToSDL());
                    (void)TTF_SetTextString(counter.label_bottom, counter.text_bottom.c_str(), counter.text_bottom.size());
                    i32 text_width = 0;
                    i32 text_height = 0;
                    (void)TTF_GetTextSize(counter.label_bottom, &text_width, &text_height);
                    const f32 text_x = math::Round(counter_area.x + (counter_area.w - static_cast<f32>(text_width)) * 0.5F);
                    const f32 text_y = math::Round(counter_area.y + counter_area.h - static_cast<f32>(text_height) - 2.0F);
                    (void)TTF_DrawRendererText(counter.label_bottom, text_x, text_y);
                }
                if (!counter.text_top.empty()) {
                    (void)TTF_SetTextFont(counter.label_top, font.ToSDL());
                    (void)TTF_SetTextString(counter.label_top, counter.text_top.c_str(), counter.text_top.size());
                    i32 text_width = 0;
                    i32 text_height = 0;
                    (void)TTF_GetTextSize(counter.label_top, &text_width, &text_height);
                    const f32 text_x = math::Round(counter_area.x + (counter_area.w - static_cast<f32>(text_width)) * 0.5F);
                    const f32 text_y = math::Round(counter_area.y - counter_area.h * 0.02F);
                    (void)TTF_DrawRendererText(counter.label_top, text_x, text_y);
                }
            }
        }
    }
};

} // namespace pce
