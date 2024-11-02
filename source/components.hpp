#pragma once

#include "collections.hpp"
#include "ecs.hpp"
#include "types.hpp"

namespace pcg {
	enum FarmType { Construction, Wine, Wheat, Fish, Cows };
	enum class ResourceBuildings { Wood, Fe, Ag, Au };
	enum class Good { Wood, Iron, Food, Steel, Coal };

	struct Money : C1<f32, Money> { using C1::C1; };
	struct Production : C1<u32, Production> { using C1::C1; };
	struct BuildingUnderConstruction {
		FarmType type; u16 progress; u16 required;
		BuildingUnderConstruction(FarmType type, u16 required) : type(type), required(required), progress(0)  {}
	};
	struct ConstructionQueue : public pce::Queue<BuildingUnderConstruction> {  };
	struct Market {
		u32 Population; u32 Demand; u32 Sold = 0; i32 PriceFactor = 0;
		f32 GetPrice() { return 100.0f * PriceFactor; }
		void RecalculateMarkets() { PriceFactor += Sold < Demand ? 1 : -1; Sold = 0; }
	};

	struct State : Archetype { pce::Parent players; pce::Component<Market> markets; bool Add(Entity); bool Remove(Entity); };
	struct Sector : Archetype { pce::Parent players; };
	struct Farm : public Sector { pce::Component<FarmType> type; bool Add(Entity player, FarmType farmType); bool Remove(Entity entity); };

	struct Player : Archetype {
		pce::Component<Money> moneys;
		pce::Component<ConstructionQueue> construction;

		bool Add(u32);
		bool Remove(Entity);
	};

	inline Player playerArchetype;
	inline State stateArchetype;
	inline Farm farmArchetype;
};



















#include "format"
// Logging
template <>
struct std::formatter<pcg::Money> : std::formatter<f32> {
	auto format(const pcg::Money &data, std::format_context &ctx) const {
		return formatter<f32>::format(data.Value, ctx);
	}
};
template <typename EnumType> requires std::is_enum_v<EnumType>
struct std::formatter<EnumType> : std::formatter<std::underlying_type_t<EnumType>> {
	auto format(const EnumType &enumValue, format_context &ctx) const {
		return std::formatter<std::underlying_type_t<EnumType>>::format(
			static_cast<std::underlying_type_t<EnumType>>(enumValue), ctx);
	}
};