#pragma once

#include <algorithm>
#include <cmath>
#include <concepts>
#include <numbers>
#include <random>
#include <stdexcept>
#include <utility>

#include "0_engine/u_types.hpp"

namespace pce {
template <typename T, template <typename> class Skill> concept HasASkill = std::derived_from<T, Skill<T>>;
template <typename To, typename From> constexpr To& Reinterpret(From& from) { return *reinterpret_cast<To*>(&from); } // NOLINT(*-pro-type-reinterpret-cast)
[[nodiscard]] inline u32 Rand() {
    static std::random_device random_device;
    static std::mt19937 gen(random_device());
    static std::uniform_int_distribution distribution(0U, U32_MAX);
    return distribution(gen);
}
[[nodiscard]] inline u32 Rand(const u32 max) {
    STL_ASSERT(max > 0, "max must be greater than 0");
    return Rand() % max;
}
[[nodiscard]] inline u32 Rand(const u32 min, const u32 max) { return min + Rand(max - min); }

template <typename Collection> Collection::key_type RandomKey(const Collection& collection) {
    if (std::empty(collection)) { throw std::runtime_error("Collection is empty!"); }
    auto iterator = std::next(std::begin(collection), Rand() % std::size(collection));
    return iterator->first;
}
template <typename Collection> const Collection::value_type& RandomValue(const Collection& collection) {
    if (std::empty(collection)) { throw std::runtime_error("Collection is empty!"); }
    auto iterator = std::next(std::begin(collection), Rand() % std::size(collection));
    return iterator->second;
}
} // namespace pce

namespace pce::noise {
[[nodiscard]] inline u32 Hash(i32 x, i32 y) {
    u32 h = static_cast<u32>(x * 374761393 + y * 668265263);
    h ^= h >> 13;
    h *= 1274126177U;
    return h ^ (h >> 16);
}
[[nodiscard]] inline f32 Grad(i32 ix, i32 iy, f32 fx, f32 fy) {
    const u32 h = Hash(ix, iy) & 3U;
    return ((h & 1U) != 0U ? fx : -fx) + ((h & 2U) != 0U ? fy : -fy);
}
[[nodiscard]] inline f32 Fade(f32 t) { return t * t * t * (t * (t * 6.0F - 15.0F) + 10.0F); }
[[nodiscard]] inline f32 Perlin(f32 x, f32 y) {
    const i32 ix = static_cast<i32>(std::floor(x));
    const i32 iy = static_cast<i32>(std::floor(y));
    const f32 fx = x - static_cast<f32>(ix);
    const f32 fy = y - static_cast<f32>(iy);
    const f32 u = Fade(fx);
    const f32 v = Fade(fy);
    // clang-format off
    const f32 n00 = Grad(ix,     iy,     fx,        fy       );
    const f32 n10 = Grad(ix + 1, iy,     fx - 1.0F, fy       );
    const f32 n01 = Grad(ix,     iy + 1, fx,        fy - 1.0F);
    const f32 n11 = Grad(ix + 1, iy + 1, fx - 1.0F, fy - 1.0F);
    // clang-format on
    return std::lerp(std::lerp(n00, n10, u), std::lerp(n01, n11, u), v);
}
[[nodiscard]] inline f32 Fbm(f32 x, f32 y, i32 octaves = 6) {
    f32 value = 0.0F, amplitude = 0.5F, frequency = 1.0F;
    for (i32 i = 0; i < octaves; ++i) {
        value += amplitude * Perlin(x * frequency, y * frequency);
        amplitude *= 0.5F;
        frequency *= 2.0F;
    }
    return value;
}
} // namespace pce::noise

namespace pce::math {
static constexpr f32 PI = std::numbers::pi_v<f32>;
static constexpr f32 SQRT_3 = std::numbers::sqrt3_v<float>;
static constexpr f32 DEG_2_RAD = PI / 180.0F;
[[nodiscard]] inline f32 Sin(const f32 t) { return std::sinf(t); }
[[nodiscard]] inline f32 Cos(const f32 t) { return std::cosf(t); }
constexpr std::pair<u32, u32> Div(const u32 value, const u32 divisor) { return { value / divisor, value % divisor }; }
template <typename T = void> T Sub(const T& left, const T& right) { return left - right; }
template <typename T = void> struct Minus {
    T operator()(const T& left, const T& right) const { return left - right; } // NOLINT(*-overloaded-operator)
};
template <typename T = void> struct Plus {
    T operator()(const T& left, const T& right) const { return left + right; } // NOLINT(*-overloaded-operator)
};
template <typename T = void> struct Size {
    u32 operator()(const T& container) const { return container.size(); } // NOLINT(*-overloaded-operator)
};
template <typename T> [[nodiscard]] constexpr T Max(T left, T right) { return left > right ? left : right; }
template <typename T> [[nodiscard]] constexpr T Min(T left, T right) { return left < right ? left : right; }
template <typename T> [[nodiscard]] constexpr u32 FloorToU32(const T value) { return static_cast<u32>(value); }
[[nodiscard]] inline f32 Ceil(const f32 value) { return std::ceilf(value); }
[[nodiscard]] inline f32 Floor(const f32 value) { return std::floorf(value); }
[[nodiscard]] constexpr f32 Clamp(const f32 value, const f32 min, const f32 max) { return std::clamp(value, min, max); }
[[nodiscard]] constexpr i32 Round(f32 a) { return static_cast<i32>(std::roundf(a)); }
template <typename T> [[nodiscard]] constexpr T Abs(const T value) { return value < 0 ? -value : value; }
template <typename T> [[nodiscard]] constexpr T Lerp(const T a, const T b, const f32 t) { return t * (b - a) + a; }

template <class _Ty> [[nodiscard]] constexpr const _Ty& max2(const _Ty& _Left, const _Ty& _Right) noexcept(noexcept(_Left < _Right)) { return _Left < _Right ? _Right : _Left; }
template <class T> struct max {
    [[nodiscard]] constexpr const T& operator()(const T& left, const T& right) const noexcept(noexcept(left < right)) { return left < right ? right : left; }
};
template <typename T> [[nodiscard]] constexpr Vec2<T> Abs(Vec2<T> v) { return { Abs(v.x), Abs(v.y) }; }
template <typename T> [[nodiscard]] constexpr Vec3<T> Abs(Vec3<T> v) { return { Abs(v.x), Abs(v.y), Abs(v.z) }; }
template <typename T> [[nodiscard]] constexpr Vec4<T> Abs(Vec4<T> v) { return { Abs(v.x), Abs(v.y), Abs(v.z), Abs(v.w) }; }
template <typename T> [[nodiscard]] constexpr Vec2<T> Lerp(Vec2<T> a, Vec2<T> b, f32 t) { return { Lerp(a.x, b.x, t), Lerp(a.y, b.y, t) }; }
template <typename T> [[nodiscard]] constexpr Vec3<T> Lerp(Vec3<T> a, Vec3<T> b, f32 t) { return { Lerp(a.x, b.x, t), Lerp(a.y, b.y, t), Lerp(a.z, b.z, t) }; }
template <typename T> [[nodiscard]] constexpr Vec4<T> Lerp(Vec4<T> a, Vec4<T> b, f32 t) { return { Lerp(a.x, b.x, t), Lerp(a.y, b.y, t), Lerp(a.z, b.z, t), Lerp(a.w, b.w, t) }; }
} // namespace pce::math
