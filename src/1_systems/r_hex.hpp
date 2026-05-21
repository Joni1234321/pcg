#pragma once
#include <SDL3/SDL_render.h>
#include <array>
#include <cmath>
#include <numbers>

#include "0_engine/g_globals.hpp"
#include "0_engine/r_window_state.hpp"
#include "0_engine/u_collections.hpp"
#include "0_engine/u_colors.hpp"
#include "0_engine/u_util.hpp"
#include "SDL3/SDL_pixels.h"
#include "SDL3/SDL_rect.h"

namespace pce {

// https://www.redblobgames.com/grids/hexagons/
constexpr u32 HEX_CORNERS = 6;
constexpr float2 HEX_SPACING {math::Sqrt3, 1.5F};
const Array<float2, HEX_CORNERS> HEX_ANGLE = { { float2 { math::Cos(-30.0F * math::DEG_2_RAD), math::Sin(-30.0F * math::DEG_2_RAD) }, float2 { math::Cos(30.0F * math::DEG_2_RAD), math::Sin(30.0F * math::DEG_2_RAD) }, // NOLINT(*-throwing-static-initialization)
                                                      float2 { math::Cos(90.0F * math::DEG_2_RAD), math::Sin(90.0F * math::DEG_2_RAD) }, float2 { math::Cos(150.0F * math::DEG_2_RAD), math::Sin(150.0F * math::DEG_2_RAD) },
                                                      float2 { math::Cos(210.0F * math::DEG_2_RAD), math::Sin(210.0F * math::DEG_2_RAD) }, float2 { math::Cos(270.0F * math::DEG_2_RAD), math::Sin(270.0F * math::DEG_2_RAD) } } };


struct Hex {
    float2 position;
    SDL_Color color;
    Hex(float2 position, SDL_Color color) : position(position), color(color) {}
};
struct HexMap {
    List<Hex> hexes { };
    f32 size { };
    void AddMap(float2 start_position, uint2 map_size) {
        for (u32 y = 0; y < map_size.y; y++) {
            for (u32 x = 0; x < map_size.x; x++) {
                float2 index{
                    static_cast<f32>(x) + (y & 1 ? 0.5F : 0.0F),
                    static_cast<f32>(y)
                };
                hexes.push_back(Hex{ start_position + index * size * HEX_SPACING, colors::radiant_orange});
            }
        }
    }
};

inline void AppendHex(List<SDL_Vertex>& vertecies, const float2 hex_size, const float2& position, const SDL_FColor color) {
    Array<SDL_FPoint, HEX_CORNERS> points { };
    for (u32 i = 0; i < HEX_CORNERS; i++) {
        const float2 vertex = position + hex_size * HEX_ANGLE[i];
        points[i] = SDL_FPoint { .x = vertex.x, .y = vertex.y };
    }
    for (u32 i = 0; i < HEX_CORNERS; i++) {
        vertecies.push_back(SDL_Vertex { .position = { .x = position.x, .y = position.y }, .color = color, .tex_coord = { } });
        vertecies.push_back(SDL_Vertex { .position = points[i], .color = color, .tex_coord = { } });
        vertecies.push_back(SDL_Vertex { .position = points[(i + 1) % 6], .color = color, .tex_coord = { } });
    }
}

struct HexRenderSystem {
    void operator()() const {
        SDL_Renderer* sdl_renderer = Singleton::Get<WindowState>().renderer;
        const HexMap& hex_map = Singleton::Get<HexMap>();
        const float2 hex_size { hex_map.size, hex_map.size };

        List<SDL_Vertex> vertecies;
        for (const Hex& hex : hex_map.hexes) {
            AppendHex(vertecies, hex_size, hex.position, colors::ToSDL_FColor(colors::black));
            AppendHex(vertecies, hex_size * 0.96F, hex.position,  colors::ToSDL_FColor(hex.color));
        }
        SDL_RenderGeometry(sdl_renderer, nullptr, vertecies.data.data(), static_cast<int>(vertecies.size()), nullptr, 0);
    }
};
}
