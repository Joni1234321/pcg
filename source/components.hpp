#pragma once

#include "collections.hpp"
#include "ecs.hpp"
#include "types.hpp"

namespace pcg {
enum class FARM_TYPE : u8 { CONSTRUCTION, WINE, WHEAT, FISH, COWS };
enum class RESOURCE_BUILDINGS : u8 { WOOD, FE, AG, AU };
enum class GOOD : u8 { WOOD, IRON, FOOD, STEEL, COAL };

//struct Money : C1<f32, Money> {
//    using C1::C1;
//};
//struct Production : C1<u32, Production> {
//    using C1::C1;
//};
//struct Money {
//    f32 value;
//};
using Tick = NamedType<u32, struct TickTag, Arithmetic>;
using Money = NamedType<f32, struct MoneyTag, Arithmetic>;
using Production = NamedType<u32, struct ProductionTag, Arithmetic>;
// two ways

struct BuildingUnderConstruction {
    FARM_TYPE type;
    u16 progress;
};
struct ConstructionQueue : pce::Queue<BuildingUnderConstruction> {
    u32 construction_capacity { 1U };
};
struct Market {
    u32 population { };
    u32 demand { };
    u32 sold = 0;
    i32 price_factor = 0;
    [[nodiscard]] f32 GetPrice() const { return 100.0F * static_cast<f32>(price_factor); }
    void RecalculateMarkets() {
        price_factor += sold < demand ? 1 : -1;
        sold = 0;
    }
};

struct StateArchetype final : Archetype {
    pce::Parent players;
    pce::Component<Market> markets;
    bool Add(Entity);
    bool Remove(Entity entity) override;
};
struct Sector : Archetype {
    pce::Parent planets;
};
struct FarmSectorArchetype final : Sector {
    pce::Component<FARM_TYPE> type;
    bool Add(Entity player, FARM_TYPE farm_type);
    bool Remove(Entity entity) override;
};

struct PlayerArchetype final : Archetype {
    pce::Component<Money> moneys;
    pce::Component<ConstructionQueue> construction_queue;
    bool Add(Money);
    bool Remove(Entity entity) override;
};

struct PlanetArchetype final : Archetype {
    pce::Parent players;
    pce::Component<Money> moneys;
    pce::Component<ConstructionQueue> construction_queue;
    pce::Component<Tick> ages;

    bool Add(Money, Tick);
    bool Remove(Entity entity) override;
};

inline PlayerArchetype player_archetype;
inline StateArchetype state_archetype;
inline FarmSectorArchetype farm_archetype;
inline PlanetArchetype planet_archetype;
} // namespace pcg


