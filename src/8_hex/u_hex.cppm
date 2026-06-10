module;
#include <cassert>

export module hex.hex;

import std;

import pce.std;
import pce.math;
import pce.collections;

export namespace hex {
// https://www.redblobgames.com/grids/hexagons/
constexpr u32 HEX_CORNERS = 6;
constexpr float2 HEX_SPACING { math::SQRT_3, 1.5F };
// NOLINTNEXTLINE(*-throwing-static-initialization)
const Array<float2, HEX_CORNERS> HEX_ANGLE = { {
    float2 { math::Cos(30.0F * math::DEG_2_RAD), math::Sin(30.0F * math::DEG_2_RAD) },   // 0 = bottom-right
    float2 { math::Cos(-30.0F * math::DEG_2_RAD), math::Sin(-30.0F * math::DEG_2_RAD) }, // 1 = top-right
    float2 { math::Cos(270.0F * math::DEG_2_RAD), math::Sin(270.0F * math::DEG_2_RAD) }, // 2 = top
    float2 { math::Cos(210.0F * math::DEG_2_RAD), math::Sin(210.0F * math::DEG_2_RAD) }, // 3 = top-left
    float2 { math::Cos(150.0F * math::DEG_2_RAD), math::Sin(150.0F * math::DEG_2_RAD) }, // 4 = bottom-left
    float2 { math::Cos(90.0F * math::DEG_2_RAD), math::Sin(90.0F * math::DEG_2_RAD) },   // 5 = bottom
} };

constexpr Array<int3, HEX_CORNERS> HEX_CUBE_NEIGHBOURS {
    int3 { +1, 0, -1 }, int3 { +1, -1, 0 }, int3 { 0, -1, +1 }, int3 { -1, 0, +1 }, int3 { -1, +1, 0 }, int3 { 0, +1, -1 },
};
constexpr Array<int3, HEX_CORNERS> HEX_CUBE_DIAGONALS {
    int3 { +2, -1, -1 }, int3 { +1, -2, +1 }, int3 { -1, -1, +2 }, int3 { -2, +1, +1 }, int3 { -1, +2, -1 }, int3 { +1, +1, -2 },
};
constexpr Array<int2, HEX_CORNERS> HEX_AXIAL_NEIGHBOURS {
    int2 { +1, 0 }, int2 { +1, -1 }, int2 { 0, -1 }, int2 { -1, 0 }, int2 { -1, +1 }, int2 { 0, +1 },
};

[[nodiscard]] constexpr int2 HexCubeToAxial(const int3 cube) { return int2 { cube.x, cube.y }; }
[[nodiscard]] constexpr int3 HexAxialToCube(const int2 axial) { return int3 { axial.x, axial.y, -axial.x - axial.y }; }
[[nodiscard]] constexpr int2 HexAxialToOffset(const int2 axial) { return int2 { axial.x + (axial.y - (axial.y & 1)) / 2, axial.y }; }
[[nodiscard]] constexpr int2 HexOffsetToAxial(const int2 offset) { return int2 { offset.x - (offset.y - (offset.y & 1)) / 2, offset.y }; }

[[nodiscard]] constexpr int3 HexCubeRound(const float3 cube_frac) {
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
    return (math::Abs(diff.x) + math::Abs(diff.y) + math::Abs(diff.z)) / 2;
}
[[nodiscard]] constexpr u32 HexAxialDistance(const int2 axial_a, const int2 axial_b) {
    const int2 diff = axial_a - axial_b;
    return (math::Abs(diff.x) + math::Abs(diff.y) + math::Abs(diff.x + diff.y)) / 2;
}
[[nodiscard]] constexpr float3 HexCubeLerp(const int3 cube_a, const int3 cube_b, float t) { return math::Lerp(static_cast<float3>(cube_a), static_cast<float3>(cube_b), t); }
[[nodiscard]] constexpr float2 HexAxialToWorld(const int2 axial) { return HEX_SPACING * float2 { axial.x + axial.y * 0.5F, static_cast<f32>(axial.y) }; }
[[nodiscard]] constexpr float2 HexAxialFloatToWorld(const float2 axial_float) { return HEX_SPACING * float2 { axial_float.x + axial_float.y * 0.5F, static_cast<f32>(axial_float.y) }; }
[[nodiscard]] constexpr int2 HexWorldToAxial(const float2 world) {
    const float2 coord = float2 { 1.0F / 3.0F, 2.0F / 3.0F } * float2 { HEX_SPACING.x * world.x - world.y, world.y };
    return HexAxialRound(coord);
}
[[nodiscard]] constexpr u8 HexAxialEdgeNeighbor(const int2 axial_a, const int2 axial_neighbour) {
    for (u8 edge = 0U; edge < HEX_CORNERS; edge++) {
        if (axial_a + HEX_AXIAL_NEIGHBOURS[edge] == axial_neighbour) { return edge; }
    }
    std::unreachable();
}
[[nodiscard]] constexpr List<int2> HexAxialLine(const int2 axial_a, const int2 axial_b) {
    const int3 cube_a = HexAxialToCube(axial_a);
    const int3 cube_b = HexAxialToCube(axial_b);
    const u32 distance = HexCubeDistance(cube_a, cube_b);
    const f32 distance_reciprocal = 1.0F / distance;
    List<int2> axial_path;
    for (u32 i = 0; i <= distance; i++) {
        const f32 t = static_cast<f32>(i) * distance_reciprocal;
        const int2 axial = HexCubeToAxial(HexCubeRound(HexCubeLerp(cube_a, cube_b, t)));
        axial_path.EmplaceBack(axial);
    }
    return axial_path;
}

// hex collections
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


struct HexBitset {
    u8 value;
    [[nodiscard]] constexpr b8 None() const { return !value; }
    [[nodiscard]] constexpr b8 Any() const { return value; }
    [[nodiscard]] constexpr b8 Test(const u8 pos) const {
        assert(pos < HEX_CORNERS);
        return value & 0x1 << pos;
    }
    constexpr void Clear() { value = 0U; }
    constexpr void Clear(const u8 pos) {
        assert(pos < HEX_CORNERS);
        value &= ~(0x1 << pos);
    }
    constexpr void Set() { value = 0x3F; }
    constexpr void Set(const u8 pos) {
        assert(pos < HEX_CORNERS);
        value |= 0x1 << pos;
    }
};
struct HexBitset2 {
    static constexpr u8 BITS_PER_HEX = 2;
    static constexpr u16 MASK = 0b11;
    u16 value;
    [[nodiscard]] constexpr b8 None() const { return !value; }
    [[nodiscard]] constexpr b8 Any() const { return value; }
    [[nodiscard]] constexpr u8 Test(const u8 pos) const {
        assert(pos < HEX_CORNERS);
        const u16 shift = pos * BITS_PER_HEX;
        return value >> shift & MASK;
    }
    constexpr void Clear() { value = 0U; }
    constexpr void Clear(const u8 pos) {
        assert(pos < HEX_CORNERS);
        const u16 shift = pos * BITS_PER_HEX;
        value &= ~(MASK << shift);
    }
    constexpr void Set(const u8 pos, const u8 val) {
        assert(pos < HEX_CORNERS);
        assert(val <= MASK);
        const u16 shift = pos * BITS_PER_HEX;
        value &= ~(MASK << shift);
        value |= val << shift;
    }
};

struct HexBitset5 {
    static constexpr u8 BITS_PER_HEX = 5;
    static constexpr u32 MASK = 0b11111;
    u32 value;
    [[nodiscard]] constexpr b8 None() const { return !value; }
    [[nodiscard]] constexpr b8 Any() const { return value; }
    [[nodiscard]] constexpr u8 Test(const u8 pos) const {
        assert(pos < HEX_CORNERS);
        const u32 shift = pos * BITS_PER_HEX;
        return value >> shift & MASK;
    }
    constexpr void Clear() { value = 0U; }
    constexpr void Clear(const u8 pos) {
        assert(pos < HEX_CORNERS);
        const u32 shift = pos * BITS_PER_HEX;
        value &= ~(MASK << shift);
    }
    constexpr void Set(const u8 pos, const u8 val) {
        assert(pos < HEX_CORNERS);
        assert(val <= MASK);
        const u32 shift = pos * BITS_PER_HEX;
        value &= ~(MASK << shift);
        value |= val << shift;
    }
};
} // namespace pce