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
constexpr float2 HEX_SPACING { math::Sqrt3, 1.5F };
// NOLINTNEXTLINE(*-throwing-static-initialization)
const Array<float2, HEX_CORNERS> HEX_ANGLE = { { float2 { math::Cos(-30.0F * math::DEG_2_RAD), math::Sin(-30.0F * math::DEG_2_RAD) }, float2 { math::Cos(30.0F * math::DEG_2_RAD), math::Sin(30.0F * math::DEG_2_RAD) },
                                                 float2 { math::Cos(90.0F * math::DEG_2_RAD), math::Sin(90.0F * math::DEG_2_RAD) }, float2 { math::Cos(150.0F * math::DEG_2_RAD), math::Sin(150.0F * math::DEG_2_RAD) },
                                                 float2 { math::Cos(210.0F * math::DEG_2_RAD), math::Sin(210.0F * math::DEG_2_RAD) }, float2 { math::Cos(270.0F * math::DEG_2_RAD), math::Sin(270.0F * math::DEG_2_RAD) } } };

constexpr Array<float3, HEX_CORNERS> HEX_CUBE_NEIGHBOURS {
    float3 { +1, 0, -1 }, float3 { +1, -1, 0 }, float3 { 0, -1, +1 }, float3 { -1, 0, +1 }, float3 { -1, +1, 0 }, float3 { 0, +1, -1 },
};
constexpr Array<float3, HEX_CORNERS> HEX_CUBE_DIAGONALS {
    float3 { +2, -1, -1 }, float3 { +1, -2, +1 }, float3 { -1, -1, +2 }, float3 { -2, +1, +1 }, float3 { -1, +2, -1 }, float3 { +1, +1, -2 },
};
constexpr Array<float2, HEX_CORNERS> HEX_AXIAL_NEIGHBOURS {
    float2 { +1, 0 }, float2 { +1, -1 }, float2 { 0, -1 }, float2 { -1, 0 }, float2 { -1, +1 }, float2 { 0, +1 },
};

constexpr uint2 HexCubeToAxial(const uint3 cube) { return uint2 { cube.x, cube.y }; }
constexpr uint3 HexAxialToCube(const uint2 axial) { return uint3 { axial.x, axial.y, -axial.x - axial.y }; }
constexpr uint3 HexCubeRound(const float3 cube_frac) {
    uint3 cube { math::RoundU32(cube_frac.x), math::RoundU32(cube_frac.y), math::RoundU32(cube_frac.z) };

    float3 const diff = float3{static_cast<f32>(cube.x), static_cast<f32>(cube.y), static_cast<f32>(cube.z)} - cube_frac;

    if (diff.x > diff.y and diff.x > diff.z) {
        cube.x = -cube.y-cube.z;
    } else if (diff.y > diff.z) {
        cube.y = -cube.x-cube.z;
    } else {
        cube.z = -cube.x-cube.y;
    }

    return cube;
}
constexpr uint2 HexAxialRound(const float2 axial_frac) {
    const float3 cube_frac = float3{axial_frac.x, axial_frac.y, -axial_frac.x - axial_frac.y};
    return HexCubeToAxial(HexCubeRound(cube_frac));
}
constexpr u32 HexCubeDistance(const uint3 a, const uint3 b) {
    uint3 const diff = a - b;
    return math::Abs(diff.x + diff.y + diff.z) / 2; // or max(diff.x, diff.y, diff.z)
}
constexpr float2 HexAxialToPixel(const uint2 axial, const float2 hex_size) { return HEX_SPACING * float2{axial.x + axial.y / 2.0F, static_cast<f32>(axial.y)} * hex_size; }
constexpr uint2 HexPixelToAxial(const float2 pixel, const float2 hex_size) {
    float2 const coord = float2{1.0F / 3.0F, 2.0F / 3.0F} * float2{HEX_SPACING.x * pixel.x - pixel.y, pixel.y} / hex_size;
    return HexAxialRound(coord);
}

struct Hex {
    float2 position;
    SDL_Color color;
    Hex(float2 position, SDL_Color color) : position(position), color(color) { }
};

struct HexMap {
    List<Hex> hexes { };
    f32 size { };
    void AddMap(float2 start_position, uint2 map_size) {
        for (u32 y = 0; y < map_size.y; y++) {
            for (u32 x = 0; x < map_size.x; x++) {
                const float2 index { static_cast<f32>(x) + (y & 1 ? 0.5F : 0.0F), static_cast<f32>(y) };
                constexpr u32 SPACING = 1;
                hexes.push_back(Hex { start_position + index * (size + SPACING) * HEX_SPACING, colors::radiant_orange });
            }
        }
    }
};

inline void HexAppend(List<SDL_Vertex>& vertecies, const float2 hex_size, const float2& position, const SDL_FColor color) {
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
            // AppendHex(vertecies, hex_size, hex.position, colors::ToSDL_FColor(colors::black));
            HexAppend(vertecies, hex_size * 0.96F, hex.position, colors::ToSDL_FColor(hex.color));
            // AppendHex(vertecies, hex_size * 0.6F, hex.position,  colors::ToSDL_FColor(colors::teal));
        }
        SDL_RenderGeometry(sdl_renderer, nullptr, vertecies.data.data(), static_cast<int>(vertecies.size()), nullptr, 0);
    }
};
}
