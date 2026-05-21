#pragma once
#include <SDL3/SDL_render.h>
#include <cmath>

#include "0_engine/g_globals.hpp"
#include "0_engine/r_window_state.hpp"
#include "0_engine/u_collections.hpp"
#include "0_engine/u_colors.hpp"
#include "0_engine/u_util.hpp"
#include "SDL3/SDL_pixels.h"

namespace pce {

struct Hex { };

inline std::array<float2, 6> make_hex(float cx, float cy, float radius) {
    std::array<float2, 6> pts { };

    for (int i = 0; i < pts.size(); ++i) {
        const f32 angle = (60.0F * i - 30.0F) * math::DEG_2_RAD;

        pts[i].x = cx + radius * std::cos(angle);
        pts[i].y = cy + radius * std::sin(angle);
    }

    return pts;
}

struct HexRenderSystem {
    void operator()() const;
    // ~HexRenderSystem() { globalData.Get<HexMap>().clear(); }
};
inline void HexRenderSystem::operator()() const {
    SDL_Renderer* sdl_renderer = Singleton::Get<WindowState>().renderer;

    List<SDL_Vertex> verts;
    constexpr u32 N = 100;
    for (u32 hex = 0; hex < N; hex++) {
        constexpr u32 HEX_VERTECIES = 6;
        constexpr SDL_FColor color = colors::ToSDL_FColor(colors::red);
        std::array<SDL_FPoint, HEX_VERTECIES> v { };
        for (u32 i = 0; i < HEX_VERTECIES; i++) {
            const f32 a = (60.0f * i - 30.0f) * math::DEG_2_RAD;
            v[i] = { 600.0F + 80.0F * cos(a), 300.0F + 80.0F * sin(a) };
        }
        for (u32 i = 0; i < HEX_VERTECIES; i++) {
            verts.push_back(SDL_Vertex { .position = { .x=600, .y=300 }, .color = color, .tex_coord = { } });
            verts.push_back(SDL_Vertex { .position = v[i], .color = color, .tex_coord = { } });
            verts.push_back(SDL_Vertex { .position = v[(i + 1) % 6], .color = color, .tex_coord = { } });
        }
    }
    // create hexes
    SDL_RenderGeometry(sdl_renderer, nullptr, verts.data.data(), verts.size(), nullptr, 0);
}

}
