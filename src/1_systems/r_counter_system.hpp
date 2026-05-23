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
    String text;
    UniquePointer<TTF_Text, DestroyText> label { nullptr };
};

struct CounterState {
    List<Counter> counters;
};

struct RenderCounterSystem {
    static constexpr f32 TEXT_MIN_SCALE = 18.0F;

    void operator()() {
        const CameraState& camera = Singleton::Get<CameraState>();
        const WindowState& window_state = Singleton::Get<WindowState>();
        CounterState& counter_state = Singleton::Get<CounterState>();
        ui::FontCollection& font_collection = Singleton::Get<ui::FontCollection>();

        constexpr f32 COUNTER_SIZE = 0.8F;
        const f32 full = camera.scale * COUNTER_SIZE;
        const int pt = math::Max(6, static_cast<int>(full * 0.17F));
        const ui::Font& font = font_collection.GetFont(static_cast<ui::FontSizes>(pt));

        for (Counter& counter : counter_state.counters) {
            const float2 world = HexAxialToWorld(counter.axial);
            const int2 screen = camera.WorldToScreen(world);

            const SDL_FRect square {
                .x = static_cast<f32>(screen.x) - full * 0.5F,
                .y = static_cast<f32>(screen.y) - full * 0.5F,
                .w = full,
                .h = full,
            };

            // background
            (void)SDL_SetRenderDrawColor(window_state.renderer, counter.color.r, counter.color.g, counter.color.b, counter.color.a);
            (void)SDL_RenderFillRect(window_state.renderer, &square);

            // border
            constexpr SDL_Color BORDER_COLOR { colors::white_smoke };
            (void)SDL_SetRenderDrawColor(window_state.renderer, BORDER_COLOR.r, BORDER_COLOR.g, BORDER_COLOR.b, BORDER_COLOR.a);
            (void)SDL_RenderRect(window_state.renderer, &square);

            // area
            const SDL_FRect icon_area {
                .x = square.x + square.w * 0.20F,
                .y = square.y + square.h * 0.15F,
                .w = square.w * 0.60F,
                .h = square.h * 0.40F,
            };
            constexpr SDL_Color AREA_COLOR { colors::gray_tint };
            (void)SDL_SetRenderDrawColor(window_state.renderer, AREA_COLOR.r, AREA_COLOR.g, AREA_COLOR.b, AREA_COLOR.a);
            (void)SDL_RenderFillRect(window_state.renderer, &icon_area);

            if (camera.scale >= TEXT_MIN_SCALE && !counter.text.empty()) {
                if (!counter.label) {
                    counter.label.Reset(TTF_CreateText(window_state.text_engine, font.ToSDL(), counter.text.c_str(), counter.text.size()));
                    (void)TTF_SetTextColor(counter.label.Get(), 255, 255, 255, 255);
                }
                TTF_SetTextFont(counter.label.Get(), font.ToSDL());
                TTF_SetTextString(counter.label.Get(), counter.text.c_str(), counter.text.size());
                i32 tw = 0;
                i32 th = 0;
                (void)TTF_GetTextSize(counter.label.Get(), &tw, &th);
                const f32 tx = math::Round(square.x + (square.w - static_cast<f32>(tw)) * 0.5F);
                const f32 ty = math::Round(square.y + square.h - static_cast<f32>(th) - 2.0F);
                (void)TTF_DrawRendererText(counter.label.Get(), tx, ty);
            }
        }
    }
};

} // namespace pce
