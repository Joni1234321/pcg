#pragma once

namespace pce {
template <std::integral T> constexpr T saturating_sub(T a, T b) noexcept {
    T result;
    if (!__builtin_sub_overflow(a, b, &result)) { return result; }
    if constexpr (std::is_unsigned_v<T>) { return std::numeric_limits<T>::min(); }
    return a < 0 ? std::numeric_limits<T>::min() : std::numeric_limits<T>::max();
}
} // namespace pce
