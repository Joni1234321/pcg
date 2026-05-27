#pragma once
#include <SDL3/SDL_render.h>
#include <array>
#include <optional>

#include "0_engine/u_collections.hpp"
#include "0_engine/u_colors.hpp"
#include "0_engine/u_texture.hpp"
#include "0_engine/u_util.hpp"
#include "SDL3/SDL_rect.h"

namespace pce {
// https://www.redblobgames.com/grids/hexagons/
constexpr u32 HEX_CORNERS = 6;
constexpr float2 HEX_SPACING { math::SQRT_3, 1.5F };
// NOLINTNEXTLINE(*-throwing-static-initialization)
const Array<float2, HEX_CORNERS> HEX_ANGLE = { { float2 { math::Cos(-30.0F * math::DEG_2_RAD), math::Sin(-30.0F * math::DEG_2_RAD) }, float2 { math::Cos(30.0F * math::DEG_2_RAD), math::Sin(30.0F * math::DEG_2_RAD) }, float2 { math::Cos(90.0F * math::DEG_2_RAD), math::Sin(90.0F * math::DEG_2_RAD) },
                                                 float2 { math::Cos(150.0F * math::DEG_2_RAD), math::Sin(150.0F * math::DEG_2_RAD) }, float2 { math::Cos(210.0F * math::DEG_2_RAD), math::Sin(210.0F * math::DEG_2_RAD) }, float2 { math::Cos(270.0F * math::DEG_2_RAD), math::Sin(270.0F * math::DEG_2_RAD) } } };

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
[[nodiscard]] constexpr int2 HexAxialRound(const float2 axial_frac) {
    const float3 cube_frac = float3 { axial_frac.x, axial_frac.y, -axial_frac.x - axial_frac.y };
    return HexCubeToAxial(HexCubeRound(cube_frac));
}
[[nodiscard]] constexpr u32 HexCubeDistance(const int3 cube_a, const int3 cube_b) {
    const int3 diff = cube_a - cube_b;
    return (math::Abs(diff.x) + math::Abs(diff.y) + math::Abs(diff.z)) / 2; // or max(diff.x, diff.y, diff.z)
}
[[nodiscard]] constexpr u32 HexAxialDistance(const int2 axial_a, const int2 axial_b) {
    const int2 diff = axial_a - axial_b;
    return (math::Abs(diff.x) + math::Abs(diff.y) + math::Abs(diff.x + diff.y)) / 2; // derived from cube distance
}
[[nodiscard]] constexpr float3 HexCubeLerp(const int3 cube_a, const int3 cube_b, float t) { return math::Lerp(static_cast<float3>(cube_a), static_cast<float3>(cube_b), t); }
[[nodiscard]] constexpr float2 HexAxialToWorld(const int2 axial) { return HEX_SPACING * float2 { axial.x + axial.y * 0.5F, static_cast<f32>(axial.y) }; }
[[nodiscard]] constexpr int2 HexWorldToAxial(const float2 world) {
    const float2 coord = float2 { 1.0F / 3.0F, 2.0F / 3.0F } * float2 { HEX_SPACING.x * world.x - world.y, world.y };
    return HexAxialRound(coord);
}

// indexable with axial coordiantes
template <class T> struct HexList {
    uint2 map_size { 0, 0 };
    List<T> data { 0 };

    [[nodiscard]] constexpr u32 AxialToIndex(const int2 axial) const { return axial.x + axial.y / 2 + axial.y * map_size.x; }
    [[nodiscard]] constexpr int2 IndexToAxial(const u32 index) const {
        const i32 y = index / map_size.x;
        return { static_cast<i32>(index % map_size.x) - y / 2, y };
    }
    [[nodiscard]] constexpr b8 Contains(const int2 axial) const {
        const u32 y = axial.y;
        const u32 x = axial.x + axial.y / 2;
        return x < map_size.x && y < map_size.y;
    }
    [[nodiscard]] constexpr T& operator[](int2 axial) { return data[AxialToIndex(axial)]; }
    [[nodiscard]] constexpr const T& operator[](int2 axial) const { return data[AxialToIndex(axial)]; }
    [[nodiscard]] constexpr u32 Size() const { return map_size.x * map_size.y; }

    void Resize(const uint2 new_size) {
        data.clear();
        map_size = new_size;
        data.resize(Size());
    }
    constexpr List<T>::iterator begin() { return data.begin(); }
    constexpr List<T>::iterator end() { return data.end(); }
    constexpr List<T>::const_iterator begin() const { return data.begin(); }
    constexpr List<T>::const_iterator end() const { return data.end(); }
    constexpr List<T>::const_iterator cbegin() const { return data.cbegin(); }
    constexpr List<T>::const_iterator cend() const { return data.cend(); }
};

inline void HexAppend(List<Vertex>& vertecies, const f32 hex_size, const int2 hex_screen, const ColorF hex_color, const Optional<ColorF> hex_color_inner = std::nullopt) {
    float2 center { hex_screen };
    Array<float2, HEX_CORNERS> points { };
    for (u32 i = 0; i < HEX_CORNERS; i++) { points[i] = center + HEX_ANGLE[i] * float2 { hex_size }; }
    for (u32 i = 0; i < HEX_CORNERS; i++) {
        vertecies.EmplaceBack(center, hex_color_inner.value_or(colors::ColorMul(hex_color, 1.2F)));
        vertecies.EmplaceBack(points[i], hex_color);
        vertecies.EmplaceBack(points[(i + 1) % HEX_CORNERS], hex_color);
    }
}
} // namespace pce
