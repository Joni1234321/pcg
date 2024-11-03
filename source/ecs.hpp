#pragma once

#include "types.hpp"

namespace pcg {
template <typename T, typename Derived> struct C1 {
    T Value;
    operator T() const { return static_cast<T>(Value); }
    C1() = default;
    C1(T val) : Value(val) { }

    Derived operator+(const Derived& other) const { return Derived(this->Value + other.Value); }

    Derived& operator-=(const Derived& other) {
        Value -= other.Value;
        return static_cast<Derived&>(*this);
    }
    Derived& operator+=(const Derived& other) {
        Value += other.Value;
        return static_cast<Derived&>(*this);
    }
    Derived& operator*=(const Derived& other) {
        Value *= other.Value;
        return static_cast<Derived&>(*this);
    }
    Derived& operator/=(const Derived& other) {
        Value /= other.Value;
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
    virtual bool Remove(Entity) {
        if (Count == 0) return false;
        Count--;
        return true;
    };

    [[nodiscard]] static constexpr Entity begin() noexcept { return 0; }
    [[nodiscard]] constexpr Entity end() const noexcept { return Count; }
};
}
