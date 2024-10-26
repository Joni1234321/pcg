#pragma once

#include "__msvc_formatter.hpp"
#include "collections.hpp"
#include "types.hpp"
#include <format>


namespace pcg {
template <typename T>
struct C1 {
	T Value;
	operator T() const { return static_cast<T>(Value); }
	C1(T val) : Value(val) {}
	C1 &operator-=(const C1 &other) { Value -= other.Value;  return *this; }
	C1 &operator+=(const C1 &other) { Value += other.Value;  return *this; }
	C1 &operator*=(const C1 &other) { Value *= other.Value;  return *this; }
	C1 &operator/=(const C1 &other) { Value /= other.Value;  return *this; }
};

enum FarmType { Wine, Wheat, Fish, Cows };
enum ResourceBuildings { Wood, Fe, Ag, Au };

// Structs
struct Money : C1<f32> { using C1::C1; };
struct Production : C1<u32> { using C1::C1; };
struct Good : C1<u32> { using C1::C1; };
struct Market {
	u32 Population;
	u32 Demand;
	u32 Sold = 0;
	i32 PriceFactor = 0;

	f32 GetPrice() { return 100.0f * PriceFactor; }
	void RecalculateMarkets() { PriceFactor += Sold < Demand ? 1 : -1; Sold = 0; }
};

// Archetypes
struct Archetype {
	u32 n = 0;
	virtual bool Add() { n++; return true; }
	virtual bool Remove(const Entity) { if (n == 0) return false; n--; return true; };

	constexpr Entity begin() const noexcept { return Entity(0); }
	constexpr Entity end() const noexcept { return Entity(n); }

};

struct State : Archetype {
	pce::Parent players;
	pce::Component<Market> markets;

	bool Add(const Entity);
	bool Remove(const Entity);
};

struct Sector : Archetype {
	pce::Parent players;
};
struct Mine : public Sector {};
struct Farm : public Sector {
	pce::Component<FarmType> buildings;
	bool Add(const Entity, const FarmType farmType);
	bool Remove(const Entity);
};

struct Wine : Farm {};
struct Wheat : Farm {};


// Declare
inline Farm farmArchetype;
inline State stateArchetype;

struct Player : Archetype {
	pce::Component<Money> moneys;

	bool Add(u32);
	bool Remove(const Entity);
};
inline Player playerArchetype;

const u32 TRAVEL_COST = 10;
};  // namespace pcg

template <>
struct std::formatter<pcg::Money> : std::formatter<f32> {
	auto format(const pcg::Money &data, std::format_context &ctx) const {
		return formatter<f32>::format(data.Value, ctx);
	}
};
template <typename T>
struct std::formatter<pcg::C1<T>> : formatter<T> {
	auto format(const pcg::C1<T> &data, format_context &ctx) const {
		return formatter<T>::format(data.Value, ctx);
	}
};
