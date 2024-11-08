#pragma once
#include <random>

#include "types.hpp"
#include <memory>
#include <stdexcept>
#include <vector>

namespace pce {
template <typename To, typename From> constexpr To& reinterpret(From& from) { return *reinterpret_cast<To*>(&from); } // NOLINT(*-pro-type-reinterpret-cast)
inline u32 Rand() {
    static std::random_device random_device;
    static std::mt19937 gen(random_device());
    static std::uniform_int_distribution distribution(0U, U32_MAX);
    return distribution(gen);
}
inline u32 Rand(u32 max) {
    if (max == 0U) return -1;
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
template <typename T = void> struct Minus {
    T operator()(const T& left, const T& right) const { return left - right; } // NOLINT(*-overloaded-operator)
};
template <typename T = void> struct Plus {
    T operator()(const T& left, const T& right) const { return left + right; } // NOLINT(*-overloaded-operator)
};
template <typename T = void> struct Size {
    u32 operator()(const T& container) const { return container.Size(); } // NOLINT(*-overloaded-operator)
};
template <typename T> T Max(T left, T right) { return left > right ? left : right; }
template <typename T> T Min(T left, T right) { return left < right ? left : right; }
inline u32 SubSafe(const u32 left, const u32 right) { return static_cast<u32>(right < left) * (left - right); }
} // namespace pce::math
