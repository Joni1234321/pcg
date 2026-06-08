module;

#include <cassert>
#include <cmath>
export module pce.util;

import std;
import pce.std;

export namespace pce {
template <typename T, template <typename> class Skill> concept HasASkill = std::derived_from<T, Skill<T>>;
template <typename To, typename From> constexpr To& Reinterpret(From& from) { return *reinterpret_cast<To*>(&from); } // NOLINT(*-pro-type-reinterpret-cast)
[[nodiscard]] inline u32 Rand() {
    static std::random_device random_device;
    static std::mt19937 gen(random_device());
    static std::uniform_int_distribution distribution(0U, U32_MAX);
    return distribution(gen);
}
[[nodiscard]] inline u32 Rand(const u32 max) {
    assert(max > 0);
    return Rand() % max;
}
[[nodiscard]] inline u32 Rand(const u32 min, const u32 max) { return min + Rand(max - min); }
[[nodiscard]] inline f32 RandF() { return static_cast<f32>(Rand()) / static_cast<f32>(U32_MAX); }
[[nodiscard]] inline f32 RandF(const f32 min, const f32 max) { return min + RandF() * (max - min); }

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

export namespace pce::noise {
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
