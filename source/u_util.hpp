#pragma once

#include <cassert>
#include <memory>
#include <random>
#include <stdexcept>

#include "u_types.hpp"

namespace pce {
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
template <typename T = void> T Sub(const T& left, const T& right) { return left - right; }
template <typename T = void> struct Minus {
    T operator()(const T& left, const T& right) const { return left - right; } // NOLINT(*-overloaded-operator)
};
template <typename T = void> struct Plus {
    T operator()(const T& left, const T& right) const { return left + right; } // NOLINT(*-overloaded-operator)
};
template <typename T = void> struct Size {
    u32 operator()(const T& container) const { return container.Size(); } // NOLINT(*-overloaded-operator)
};
template <typename T> constexpr T Max(T left, T right) { return left > right ? left : right; }
template <typename T> constexpr T Min(T left, T right) { return left < right ? left : right; }
template <typename T> constexpr u32 FloorToU32(const T value) { return static_cast<u32>(value); }
inline f32 Ceil(const f32 value) { return std::ceil(value); }
template <typename T> constexpr T Abs(const T value) { return value < 0 ? -value : value; }
template <typename T> constexpr T Lerp(const T min, const T max, const f32 value) { return (value * (max - min)) + min; }
} // namespace pce::math
