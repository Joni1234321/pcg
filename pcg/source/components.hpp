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
struct Money : C1<f32> { using C1::C1; };
struct Good : C1<u32> { using C1::C1; };
struct Transporter : C1<u32> { using C1::C1; };
namespace markets {
struct Market {
	u32 Population;
	u32 Demand;
	u32 Sold = 0;
	i32 PriceFactor = 0;
};
pce::Component<Market> markets;
}
struct Archetype {
	u32 n = 0;
	virtual bool Remove(const Entity) = 0;
};
struct Market : Archetype {
	pce::Parent players;

	bool Add(const Entity);
};
struct Sector : Archetype {
	pce::Parent markets;

	pce::Component<Money> moneys;
};
struct Factory : public Sector {
	pce::Component<Good> goods;
	pce::Component<pce::List<Transporter>> transporters;

	bool Add(const Entity, const u32, const u32);
	bool Remove(const Entity);
};
struct Mine : public Sector {};
struct Farm : public Sector {};

Factory factoryArchetype;

//namespace mine {
//u32 n;
//pce::Parent markets;
//
//pce::Component<Money> moneys;
//pce::Component<u32> goods;
//pce::Component<pce::List<u32>> transporters;
//};
//namespace farm {
//u32 n;
//pce::Parent markets;
//
//pce::Component<Money> moneys;
//pce::Component<u32> goods;
//pce::Component<pce::List<u32>> transporters;
//};
//
//namespace energy {
//u32 n;
//pce::Parent owners;
//pce::Parent markets;
//
//pce::Component<Money> moneys;
//pce::Component<u32> goods;
//pce::Component<pce::List<u32>> transporters;
//};
namespace player {
u32 n;
pce::Component<Money> moneys;
};
const u32 TRAVEL_COST = 10;
};

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
