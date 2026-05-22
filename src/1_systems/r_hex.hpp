#pragma once
#include <SDL3/SDL_render.h>
#include <array>
#include <cmath>
#include <numbers>

#include "0_engine/g_globals.hpp"
#include "0_engine/r_window_state.hpp"
#include "0_engine/u_collections.hpp"
#include "0_engine/u_colors.hpp"
#include "0_engine/u_logger.hpp"
#include "0_engine/u_util.hpp"
#include "SDL3/SDL_pixels.h"
#include "SDL3/SDL_rect.h"

namespace pce {

struct Camera {
    float2 world_position;
    f32 zoom;
    [[nodiscard]] constexpr float2 ScreenToWorld(const float2 screen) const {
        return (float2{static_cast<f32>(screen.x), static_cast<f32>(screen.y)} - world_position) / zoom;
    }
    [[nodiscard]] constexpr float2 WorldToScreen(const float2 world) const {
        return world * zoom + world_position;
    }
};
// https://www.redblobgames.com/grids/hexagons/
constexpr u32 HEX_CORNERS = 6;
constexpr float2 HEX_SPACING { math::Sqrt3, 1.5F };
// NOLINTNEXTLINE(*-throwing-static-initialization)
const Array<float2, HEX_CORNERS> HEX_ANGLE = { { float2 { math::Cos(-30.0F * math::DEG_2_RAD), math::Sin(-30.0F * math::DEG_2_RAD) }, float2 { math::Cos(30.0F * math::DEG_2_RAD), math::Sin(30.0F * math::DEG_2_RAD) },
                                                 float2 { math::Cos(90.0F * math::DEG_2_RAD), math::Sin(90.0F * math::DEG_2_RAD) }, float2 { math::Cos(150.0F * math::DEG_2_RAD), math::Sin(150.0F * math::DEG_2_RAD) },
                                                 float2 { math::Cos(210.0F * math::DEG_2_RAD), math::Sin(210.0F * math::DEG_2_RAD) }, float2 { math::Cos(270.0F * math::DEG_2_RAD), math::Sin(270.0F * math::DEG_2_RAD) } } };

constexpr Array<int3, HEX_CORNERS> HEX_CUBE_NEIGHBOURS {
    int3 { +1, 0, -1 }, int3 { +1, -1, 0 }, int3 { 0, -1, +1 }, int3 { -1, 0, +1 }, int3 { -1, +1, 0 }, int3 { 0, +1, -1 },
};
constexpr Array<int3, HEX_CORNERS> HEX_CUBE_DIAGONALS {
    int3 { +2, -1, -1 }, int3 { +1, -2, +1 }, int3 { -1, -1, +2 }, int3 { -2, +1, +1 }, int3 { -1, +2, -1 }, int3 { +1, +1, -2 },
};
constexpr Array<int2, HEX_CORNERS> HEX_AXIAL_NEIGHBOURS {
    int2 { +1, 0 }, int2 { +1, -1 }, int2 { 0, -1 }, int2 { -1, 0 }, int2 { -1, +1 }, int2 { 0, +1 },
};

constexpr int2 HexCubeToAxial(const int3 cube) { return int2 { cube.x, cube.y }; }
constexpr int3 HexAxialToCube(const int2 axial) { return int3 { axial.x, axial.y, -axial.x - axial.y }; }
constexpr int3 HexCubeRound(const float3 cube_frac) {
    int3 cube { math::Round(cube_frac.x), math::Round(cube_frac.y), math::Round(cube_frac.z) };

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
constexpr int2 HexAxialRound(const float2 axial_frac) {
    const float3 cube_frac = float3{axial_frac.x, axial_frac.y, -axial_frac.x - axial_frac.y};
    return HexCubeToAxial(HexCubeRound(cube_frac));
}
constexpr u32 HexCubeDistance(const int3 a, const int3 b) {
    int3 const diff = a - b;
    return math::Abs(diff.x + diff.y + diff.z) / 2; // or max(diff.x, diff.y, diff.z)
}

// world space: 1 unit = 1 hex size
constexpr float2 HexAxialToWorld(const int2 axial) {
    return HEX_SPACING * float2{axial.x + axial.y / 2.0F, static_cast<f32>(axial.y)};
}
constexpr int2 HexWorldToAxial(const float2 world) {
    float2 const coord = float2{1.0F / 3.0F, 2.0F / 3.0F} * float2{HEX_SPACING.x * world.x - world.y, world.y};
    return HexAxialRound(coord);
}

// pixel to world
constexpr float2 HEX_CENTER { 400.0F, 400.0F };


struct Hex {
    float2 position;
    SDL_Color color;
    Hex(float2 position, SDL_Color color) : position(position), color(color) { }
};

struct HexMap {
    u32 width{};
    u32 height{};
    List<Hex> hexes { };

    [[nodiscard]] constexpr u32 AxialToIndex(const int2 axial) const {
        return axial.x + axial.y / 2 + axial.y * width;
    }
    [[nodiscard]] constexpr int2 IndexToAxial(const u32 index) const {
        const i32 y = index / width;
        return { static_cast<i32>(index % width) - y / 2, y };
    }
    [[nodiscard]] constexpr b8 Contains(const int2 axial) const {
        u32 const y = axial.y;
        u32 const x = axial.x + axial.y / 2;
        return x < width && y < height;
    }
    constexpr Hex& operator[] (int2 axial) { return hexes[AxialToIndex(axial)]; }

    void AddMap(uint2 map_size) {
        width = map_size.x;
        height = map_size.y;
        for (u32 i = 0; i < map_size.x * map_size.y; i++) {
            hexes.push_back(Hex { HexAxialToWorld(IndexToAxial(i)), colors::radiant_orange });
        }
    }
};

inline void HexAppend(List<SDL_Vertex>& vertecies, const f32 hex_size, const float2& position, const SDL_FColor color) {
    Array<SDL_FPoint, HEX_CORNERS> points { };
    for (u32 i = 0; i < HEX_CORNERS; i++) {
        const float2 vertex = position + HEX_ANGLE[i] * hex_size;
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
        const Camera& camera = Singleton::Get<Camera>();

        const InputState& input_state = Singleton::Get<InputState>();
        List<SDL_Vertex> vertecies;

        // const float2 mouse_world = camera.ScreenToWorld(input_state.mouse_position);
        // const int2 axial = HexWorldToAxial(mouse_world);
        // if (hex_map.Contains(axial)) {
        //     float2 pixel = camera.WorldToScreen(HexAxialToWorld(axial));
        //     HexAppend(vertecies, hex_map.hex_size, pixel, colors::ToSDL_FColor(colors::teal));
        //     Logger().Log("[{:3},{:3}] pixel [{:4},{:4}]", axial.x, axial.y, input_state.mouse_position.x, input_state.mouse_position.y);
        // }
        //
        // for (const Hex& hex : hex_map.hexes) {
        //     float2 pixel = WorldToScreen(hex.position, hex_map.hex_size);
        //     HexAppend(vertecies, hex_map.hex_size * 0.90F, pixel, colors::ToSDL_FColor(hex.color));
        // }




        SDL_RenderGeometry(sdl_renderer, nullptr, vertecies.data.data(), static_cast<int>(vertecies.size()), nullptr, 0);
    }
};
}
