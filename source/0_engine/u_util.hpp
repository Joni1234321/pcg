#pragma once

#include <cassert>
#include <numbers>
#include <random>
#include <stdexcept>

#include "0_engine/u_types.hpp"

namespace pce {
template <typename T, template<typename> class Skill>concept HasASkill = std::derived_from<T, Skill<T>>;
template <typename To, typename From> constexpr To& Reinterpret(From& from) { return *reinterpret_cast<To*>(&from); } // NOLINT(*-pro-type-reinterpret-cast)
inline u32 Rand() {
    static std::random_device random_device;
    static std::mt19937 gen(random_device());
    static std::uniform_int_distribution distribution(0U, U32_MAX);
    return distribution(gen);
}
inline u32 Rand(const u32 max) {
    assert(max > 0);
    return Rand() % max;
}
template <typename Collection> typename Collection::key_type RandomKey(const Collection& collection) {
    if (std::empty(collection)) { throw std::runtime_error("Collection is empty!"); }
    auto iterator = std::next(std::begin(collection), Rand() % std::size(collection));
    return iterator->first;
}
template <typename Collection> const typename Collection::value_type& RandomValue(const Collection& collection) {
    if (std::empty(collection)) { throw std::runtime_error("Collection is empty!"); }
    auto iterator = std::next(std::begin(collection), Rand() % std::size(collection));
    return iterator->second;
}
} // namespace pce

namespace pce::math {
static constexpr f32 PI = std::numbers::pi_v<f32>;
inline f32 Sin(const f32 t) { return std::sinf(t); }
inline f32 Cos(const f32 t) { return std::cosf(t); }
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
template <typename T> constexpr T Max(T left, T right) { return left > right ? left : right; }
template <typename T> constexpr T Min(T left, T right) { return left < right ? left : right; }
template <typename T> constexpr u32 FloorToU32(const T value) { return static_cast<u32>(value); }
inline f32 Ceil(const f32 value) { return std::ceil(value); }
template <typename T> constexpr T Abs(const T value) { return value < 0 ? -value : value; }
template <typename T> constexpr T Lerp(const T min, const T max, const f32 value) { return value * (max - min) + min; }

template <class _Ty> [[nodiscard]] constexpr const _Ty& max2(const _Ty& _Left, const _Ty& _Right) noexcept(noexcept(_Left < _Right)) { return _Left < _Right ? _Right : _Left; }
template <class T> struct max {
    [[nodiscard]] constexpr const T& operator()(const T& left, const T& right) const noexcept(noexcept(left < right)) { return left < right ? right : left; }
};
} // namespace pce::math
