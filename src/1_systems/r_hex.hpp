#pragma once
#include <SDL3/SDL_render.h>
#include <array>

#include "0_engine/g_globals.hpp"
#include "0_engine/r_window_state.hpp"
#include "0_engine/u_collections.hpp"
#include "0_engine/u_colors.hpp"
#include "0_engine/u_logger.hpp"
#include "0_engine/u_util.hpp"
#include "1_systems/i_input_system.hpp"
#include "SDL3/SDL_pixels.h"
#include "SDL3/SDL_rect.h"
#include "r_camera_system.hpp"

namespace pce {
// https://www.redblobgames.com/grids/hexagons/
constexpr u32 HEX_CORNERS = 6;
constexpr float2 HEX_SPACING { math::SQRT_3, 1.5F };
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

    const float3 diff = math::Abs(float3 { cube } - cube_frac);

    if (diff.x > diff.y && diff.x > diff.z) {
        cube.x = -cube.y - cube.z;
    } else if (diff.y > diff.z) {
        cube.y = -cube.x - cube.z;
    } else {
        cube.z = -cube.x - cube.y;
    }

    return cube;
}
constexpr int2 HexAxialRound(const float2 axial_frac) {
    const float3 cube_frac = float3 { axial_frac.x, axial_frac.y, -axial_frac.x - axial_frac.y };
    return HexCubeToAxial(HexCubeRound(cube_frac));
}
constexpr u32 HexCubeDistance(const int3 a, const int3 b) {
    const int3 diff = a - b;
    return math::Abs(diff.x + diff.y + diff.z) / 2; // or max(diff.x, diff.y, diff.z)
}

// world space: 1 unit = 1 hex size
constexpr float2 HexAxialToWorld(const int2 axial) { return HEX_SPACING * float2 { axial.x + axial.y / 2.0F, static_cast<f32>(axial.y) }; }
constexpr int2 HexWorldToAxial(const float2 world) {
    const float2 coord = float2 { 1.0F / 3.0F, 2.0F / 3.0F } * float2 { HEX_SPACING.x * world.x - world.y, world.y };
    return HexAxialRound(coord);
}

// pixel to world
constexpr float2 HEX_CENTER { 400.0F, 400.0F };

struct Hex {
    float2 position;
    SDL_Color color;
    Hex(float2 position, SDL_Color color) : position(position), color(color) { }
};

struct HexMapState {
    u32 width { };
    u32 height { };
    List<Hex> hexes { };

    [[nodiscard]] constexpr u32 AxialToIndex(const int2 axial) const { return axial.x + axial.y / 2 + axial.y * width; }
    [[nodiscard]] constexpr int2 IndexToAxial(const u32 index) const {
        const i32 y = index / width;
        return { static_cast<i32>(index % width) - y / 2, y };
    }
    [[nodiscard]] constexpr b8 Contains(const int2 axial) const {
        const u32 y = axial.y;
        const u32 x = axial.x + axial.y / 2;
        return x < width && y < height;
    }
    constexpr Hex& operator[](int2 axial) { return hexes[AxialToIndex(axial)]; }

    void AddMap(uint2 map_size) {
        width = map_size.x;
        height = map_size.y;
        for (u32 i = 0; i < map_size.x * map_size.y; i++) { hexes.push_back(Hex { HexAxialToWorld(IndexToAxial(i)), colors::indigo }); }
    }

    void GenerateTerrain(u32 seed = 0) {
        constexpr f32 SCALE = 0.04F;
        const f32 seed_f = static_cast<f32>(seed);
        for (u32 i = 0; i < hexes.size(); i++) {
            const float2 pos = hexes[i].position;
            const f32 h = (pce::noise::Fbm(pos.x * SCALE + seed_f, pos.y * SCALE + seed_f) + 1.0F) * 0.5F;
            // clang-format off
            SDL_Color color;
            if      (h < 0.25F) color = SDL_Color {  20U,  60U, 120U, 255U }; // deep ocean
            else if (h < 0.38F) color = SDL_Color {  50U, 100U, 180U, 255U }; // ocean
            else if (h < 0.43F) color = colors::khaki;                         // beach
            else if (h < 0.60F) color = SDL_Color { 100U, 190U,  80U, 255U }; // grass
            else if (h < 0.72F) color = colors::forest_green;                  // forest
            else if (h < 0.85F) color = colors::gray;                          // mountain
            else                color = colors::white;                          // snow
            // clang-format on
            hexes[i].color = color;
        }
    }
};

inline void HexAppend(List<SDL_Vertex>& vertecies, const f32 hex_size, const int2 hex_screen, const SDL_FColor hex_color) {
    SDL_FPoint center { .x = static_cast<f32>(hex_screen.x), .y = static_cast<f32>(hex_screen.y) };
    Array<SDL_FPoint, HEX_CORNERS> points { };
    for (u32 i = 0; i < HEX_CORNERS; i++) {
        const float2 vertex = float2 { hex_screen } + HEX_ANGLE[i] * hex_size;
        points[i] = SDL_FPoint { .x = vertex.x, .y = vertex.y };
    }
    for (u32 i = 0; i < HEX_CORNERS; i++) {
        SDL_FColor sdl_f_color = colors::ColorMul(hex_color, 0.8F);
        // SDL_FColor sdl_f_color = colors::ToSDL_FColor(colors::white_smoke);
        vertecies.EmplaceBack(center, sdl_f_color, SDL_FPoint { });
        vertecies.EmplaceBack(points[i], hex_color, SDL_FPoint { });
        vertecies.EmplaceBack(points[(i + 1) % HEX_CORNERS], hex_color, SDL_FPoint { });
    }
}

struct RenderHexSystem {
    List<SDL_Vertex> vertecies;
    void operator()() {
        SDL_Renderer* sdl_renderer = Singleton::Get<WindowState>().renderer;
        const HexMapState& hex_map = Singleton::Get<HexMapState>();
        const CameraState& camera = Singleton::Get<CameraState>();
        const InputState& input_state = Singleton::Get<InputState>();

        vertecies.clear();
        vertecies.reserve(HEX_CORNERS * hex_map.hexes.size());

        const float2 mouse_world = camera.ScreenToWorld(input_state.mouse_position);
        const int2 mouse_axial = HexWorldToAxial(mouse_world);
        if (hex_map.Contains(mouse_axial)) {
            const int2 mouse_hex_center = camera.WorldToScreen(HexAxialToWorld(mouse_axial));
            HexAppend(vertecies, camera.scale, mouse_hex_center, colors::ToSDL_FColor(colors::teal));
        }

        for (const Hex& hex : hex_map.hexes) {
            const int2 pixel = camera.WorldToScreen(hex.position);
            HexAppend(vertecies, camera.scale * 0.90F, pixel, colors::ToSDL_FColor(hex.color));
        }

        SDL_RenderGeometry(sdl_renderer, nullptr, vertecies.data.data(), static_cast<int>(vertecies.size()), nullptr, 0);
    }
};

}
