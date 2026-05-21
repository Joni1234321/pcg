#pragma once
#include <SDL3/SDL_render.h>
#include <array>
#include <cmath>

#include "0_engine/g_globals.hpp"
#include "0_engine/r_window_state.hpp"
#include "0_engine/u_collections.hpp"
#include "0_engine/u_colors.hpp"
#include "0_engine/u_util.hpp"
#include "SDL3/SDL_pixels.h"
#include "SDL3/SDL_rect.h"

namespace pce {

// struct Hex {
//     float2 position;
//     SDL_Color color;
// };
struct HexMap {
    List<float2> positions { };
    f32 size { };
    SDL_Color color { };
};

constexpr u32 HEX_CORNERS = 6;
const std::array<float2, HEX_CORNERS> HEX_ANGLE = { { float2 { std::cos(-30.0F * math::DEG_2_RAD), std::sin(-30.0F * math::DEG_2_RAD) }, float2 { std::cos(30.0F * math::DEG_2_RAD), std::sin(30.0F * math::DEG_2_RAD) },
                                                      float2 { std::cos(90.0F * math::DEG_2_RAD), std::sin(90.0F * math::DEG_2_RAD) }, float2 { std::cos(150.0F * math::DEG_2_RAD), std::sin(150.0F * math::DEG_2_RAD) },
                                                      float2 { std::cos(210.0F * math::DEG_2_RAD), std::sin(210.0F * math::DEG_2_RAD) }, float2 { std::cos(270.0F * math::DEG_2_RAD), std::sin(270.0F * math::DEG_2_RAD) } } };
struct HexRenderSystem {
    void operator()() const;
    // ~HexRenderSystem() { globalData.Get<HexMap>().clear(); }
};
inline void AppendHex(List<SDL_Vertex>& verts, const float2 hex_size, const float2& position, const SDL_FColor color) {
    std::array<SDL_FPoint, HEX_CORNERS> points { };
    for (u32 i = 0; i < HEX_CORNERS; i++) {
        const float2 vertex = position + hex_size * HEX_ANGLE[i];
        points[i] = SDL_FPoint { .x = vertex.x, .y = vertex.y };
    }
    for (u32 i = 0; i < HEX_CORNERS; i++) {
        verts.push_back(SDL_Vertex { .position = { .x = position.x, .y = position.y }, .color = color, .tex_coord = { } });
        verts.push_back(SDL_Vertex { .position = points[i], .color = color, .tex_coord = { } });
        verts.push_back(SDL_Vertex { .position = points[(i + 1) % 6], .color = color, .tex_coord = { } });
    }
}
inline void HexRenderSystem::operator()() const {
    SDL_Renderer* sdl_renderer = Singleton::Get<WindowState>().renderer;
    const HexMap& hex_map = Singleton::Get<HexMap>();
    const float2 hex_size { hex_map.size, hex_map.size };

    List<SDL_Vertex> verts;
    for (const float2& position : hex_map.positions) {
        AppendHex(verts, hex_size, position, colors::ToSDL_FColor(colors::red));
        AppendHex(verts, hex_size * 0.9F, position,  colors::ToSDL_FColor(colors::beige));
    }
    // create hexes
    SDL_RenderGeometry(sdl_renderer, nullptr, verts.data.data(), static_cast<int>(verts.size()), nullptr, 0);
}

}
