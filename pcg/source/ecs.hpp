#pragma once

#include "types.hpp"
namespace pcg {
	template <typename T, typename Derived> struct C1 {
		T Value;
		operator T() const { return static_cast<T>(Value); }
		C1() {}
		C1(T val) : Value(val) {}

		Derived operator+(const Derived& other) const { return Derived(this->Value + other.Value); }

		Derived& operator-=(const Derived& other) { Value -= other.Value;  return static_cast<Derived&>(*this); }
		Derived& operator+=(const Derived& other) { Value += other.Value;  return static_cast<Derived&>(*this); }
		Derived& operator*=(const Derived& other) { Value *= other.Value;  return static_cast<Derived&>(*this); }
		Derived& operator/=(const Derived& other) { Value /= other.Value;  return static_cast<Derived&>(*this); }
	};
	struct Archetype {
		u32 n = 0;
		virtual bool Add() { n++; return true; }
		virtual bool Remove(Entity) { if (n == 0) return false; n--; return true; };

		constexpr Entity begin() const noexcept { return Entity(0); }
		constexpr Entity end() const noexcept { return Entity(n); }
	};
}