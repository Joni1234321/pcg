#pragma once
#include <SDL3/SDL_render.h>
#include <string>

#include "0_engine/g_globals.hpp"
#include "0_engine/r_window_state.hpp"
#include "0_engine/u_collections.hpp"
#include "0_engine/u_colors.hpp"
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
};

struct CounterState {
    List<Counter> counters;
};

struct RenderCounterSystem {
    static constexpr f32 TEXT_MIN_SCALE = 18.0F; // hide label text when zoomed out

    void operator()() {
        const CameraState& camera       = Singleton::Get<CameraState>();
        const WindowState& window_state = Singleton::Get<WindowState>();
        CounterState&      counter_state = Singleton::Get<CounterState>();
        for (Counter& counter : counter_state.counters) {
            const float2 world = HexAxialToWorld(counter.axial);
            const int2 screen = camera.WorldToScreen(world);

            constexpr f32 COUNTER_SIZE = 0.8F;
            const f32 full = camera.scale * COUNTER_SIZE;
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
            constexpr SDL_Color BORDER_COLOR {colors::white_smoke};
            (void)SDL_SetRenderDrawColor(window_state.renderer, BORDER_COLOR.r, BORDER_COLOR.g, BORDER_COLOR.b, BORDER_COLOR.a);
            (void)SDL_RenderRect(window_state.renderer, &square);

            // area
            const SDL_FRect icon_area {
                .x = square.x + square.w * 0.20F,
                .y = square.y + square.h * 0.15F,
                .w = square.w * 0.60F,
                .h = square.h * 0.40F,
            };
            constexpr SDL_Color AREA_COLOR {colors::gray_tint};
            (void)SDL_SetRenderDrawColor(window_state.renderer, AREA_COLOR.r, AREA_COLOR.g, AREA_COLOR.b, AREA_COLOR.a);
            (void)SDL_RenderFillRect(window_state.renderer, &icon_area);

            // icon
            // DrawCounterIcon(window_state.renderer, counter.unit_type, icon_area, counter.text_color);

            // label
            if (camera.scale >= TEXT_MIN_SCALE && !counter.text.empty()) {
                constexpr f32 CHAR_W = SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE;
                constexpr f32 CHAR_H = SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE;
                const f32 tw = static_cast<f32>(counter.text.size()) * CHAR_W;
                const f32 tx = square.x + (square.w - tw) * 0.5F;
                const f32 ty = square.y + square.h - CHAR_H - 2.0F;
                (void)SDL_SetRenderDrawColor(window_state.renderer, 255, 255, 255, 255);
                (void)SDL_RenderDebugText(window_state.renderer, tx, ty, counter.text.c_str());
            }
        }
    }
};

} // namespace pce