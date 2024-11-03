// ReSharper disable CppNonExplicitConversionOperator
// ReSharper disable CppNonExplicitConversionOperator
// ReSharper disable CppNonExplicitConvertingConstructor
#pragma once

#include "types.hpp"

namespace pcg {
template <typename T, typename Derived> struct C1 {
    T value;
    operator T() const { return static_cast<T>(value); } // NOLINT(*-explicit-constructor, *-explicit-conversions)
     constexpr C1() = default;
     constexpr C1(T val) : value(val) { } // NOLINT(*-explicit-constructor, *-explicit-conversions)

    Derived operator+(const Derived& other) const { return Derived(this->value + other.value); }

    Derived& operator-=(const Derived& other) {
        value -= other.value;
        return static_cast<Derived&>(*this);
    }
    Derived& operator+=(const Derived& other) {
        value += other.value;
        return static_cast<Derived&>(*this);
    }
    Derived& operator*=(const Derived& other) {
        value *= other.value;
        return static_cast<Derived&>(*this);
    }
    Derived& operator/=(const Derived& other) {
        value /= other.value;
        return static_cast<Derived&>(*this);
    }
};
struct Archetype {
    virtual ~Archetype() = default;
    u32 Count = 0;
    virtual bool Add() {
        Count++;
        return true;
    }
    virtual bool Remove(Entity entity) {
        if (Count == 0) return false;
        Count--;
        return true;
    };

    [[nodiscard]] static constexpr Entity begin() noexcept { return 0; }
    [[nodiscard]] constexpr Entity end() const noexcept { return Count; }
};
}
