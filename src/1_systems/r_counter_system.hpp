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
    Array<SDL_Color, 3> colors;
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

        constexpr f32 COUNTER_SIZE = 1.2F;
        const f32 width = camera.scale * COUNTER_SIZE;
        const f32 height = camera.scale * COUNTER_SIZE * 0.8F;
        const i32 pt = static_cast<i32>(height * 0.3F);
        const ui::Font& font = font_collection.GetFontBold(static_cast<ui::FontSizes>(pt));

        for (const Counter& counter : counter_state.counters) {
            const float2 world = HexAxialToWorld(counter.axial);
            const int2 screen = camera.WorldToScreen(world);

            const SDL_FRect area_counter {
                .x = static_cast<f32>(screen.x) - width * 0.5F,
                .y = static_cast<f32>(screen.y) - height * 0.5F,
                .w = width,
                .h = height,
            };


            for (i32 i = static_cast<i32>(counter.colors.size()) - 1; i >= 0; i--) {
                SDL_Color color_background = counter.colors[static_cast<u32>(i)];
                if (color_background.a == 0) {continue;}

                constexpr f32 STACK_OFFSET = 1.0F / 10.0F;
                SDL_FRect area = area_counter;
                area.x += STACK_OFFSET * area_counter.w * static_cast<f32>(i);
                area.y += STACK_OFFSET * area_counter.w * static_cast<f32>(i);

                // shadow
                constexpr f32 SHADOW_OFFSET = 4.0F;
                const SDL_FRect area_shadow {
                    .x = area.x + SHADOW_OFFSET,
                    .y = area.y + SHADOW_OFFSET,
                    .w = area.w,
                    .h = area.h,
                };
                constexpr SDL_Color SHADOW_COLOR = colors::ColorWithAlpha(colors::black, 0.5F);
                (void)SDL_SetRenderDrawColor(window_state.renderer, SHADOW_COLOR.r, SHADOW_COLOR.g, SHADOW_COLOR.b, SHADOW_COLOR.a);
                (void)SDL_RenderFillRect(window_state.renderer, &area_shadow);


                // border
                constexpr SDL_Color BORDER_COLOR { colors::black };
                (void)SDL_SetRenderDrawColor(window_state.renderer, BORDER_COLOR.r, BORDER_COLOR.g, BORDER_COLOR.b, BORDER_COLOR.a);
                (void)SDL_RenderFillRect(window_state.renderer, &area);

                // background
                constexpr f32 BORDER_WIDTH_PX = 5.0F;
                f32 border_width = 0.04F * area_counter.w;
                SDL_FRect area_background = area;
                area_background.x += border_width;
                area_background.y += border_width;
                area_background.w -= border_width * 2.0F;
                area_background.h -= border_width * 2.0F;
                (void)SDL_SetRenderDrawColor(window_state.renderer, color_background.r, color_background.g, color_background.b, color_background.a);
                (void)SDL_RenderFillRect(window_state.renderer, &area_background);
            }

            // area
            const SDL_FRect area_icon {
                .x = area_counter.x + area_counter.w * 0.20F,
                .y = area_counter.y + area_counter.h * 0.25F,
                .w = area_counter.w * 0.60F,
                .h = area_counter.h * 0.40F,
            };
            constexpr SDL_Color COLOR_ICON { colors::gray_tint };
            (void)SDL_SetRenderDrawColor(window_state.renderer, COLOR_ICON.r, COLOR_ICON.g, COLOR_ICON.b, COLOR_ICON.a);
            (void)SDL_RenderFillRect(window_state.renderer, &area_icon);

            // labels
            if (camera.scale >= TEXT_MIN_SCALE) {
                if (!counter.text_bottom.empty()) {
                    (void)TTF_SetTextFont(counter.label_bottom, font.ToSDL());
                    (void)TTF_SetTextString(counter.label_bottom, counter.text_bottom.c_str(), counter.text_bottom.size());
                    i32 text_width = 0;
                    i32 text_height = 0;
                    (void)TTF_GetTextSize(counter.label_bottom, &text_width, &text_height);
                    const f32 text_x = math::Round(area_counter.x + (area_counter.w - static_cast<f32>(text_width)) * 0.5F);
                    const f32 text_y = math::Round(area_counter.y + area_counter.h - static_cast<f32>(text_height) * 0.8F);
                    (void)TTF_DrawRendererText(counter.label_bottom, text_x, text_y);
                }
                if (!counter.text_top.empty()) {
                    (void)TTF_SetTextFont(counter.label_top, font.ToSDL());
                    (void)TTF_SetTextString(counter.label_top, counter.text_top.c_str(), counter.text_top.size());
                    i32 text_width = 0;
                    i32 text_height = 0;
                    (void)TTF_GetTextSize(counter.label_top, &text_width, &text_height);
                    const f32 text_x = math::Round(area_counter.x + (area_counter.w - static_cast<f32>(text_width)) * 0.5F);
                    const f32 text_y = math::Round(area_counter.y - area_counter.h * 0.12F);
                    (void)TTF_DrawRendererText(counter.label_top, text_x, text_y);
                }
            }
        }
    }
};

} // namespace pce
