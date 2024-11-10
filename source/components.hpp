#pragma once

#include <unordered_map>

#include "collections.hpp"
#include "ecs.hpp"
#include "types.hpp"

namespace pcg {
using Percentage = NamedType<f32, struct PercentageTag, Arithmetic>;

using Tick = NamedType<u32, struct TickTag, Arithmetic>;
using Money = NamedType<f32, struct MoneyTag, Arithmetic>;
using Population = NamedType<f32, struct PopulationTag, Arithmetic>;
using Production = NamedType<u32, struct ProductionTag, Arithmetic>;

// theory aktiver og passiver. aktiver is the initial cost of investments
// https://ordbog.ku.dk/pdf/financial_terminology.pdf
// https://uwaterloo.ca/economics/sites/default/files/uploads/documents/econ-101-syllabus.pdf
// https://en.wikipedia.org/wiki/Supply_and_demand
struct alignas(16U) FarmStats {
    u32 assets;       // aktiver
    u32 production;   // produktion
    u32 depreciation; // afskrivning

    [[nodiscard]] Money FinancialResult(const u32 cost_of_good) const { return Money { static_cast<f32>(Income(cost_of_good)) - static_cast<f32>(Expenses()) }; }     // resultat
    [[nodiscard]] Percentage ReturnOnAssets(const u32 cost_of_good) const { return Percentage { FinancialResult(cost_of_good).Value() / static_cast<f32>(assets) }; } // afkastningsgrad
private:
    [[nodiscard]] u32 Expenses() const { return depreciation; }
    [[nodiscard]] u32 Income(const u32 cost_of_good) const { return production * cost_of_good; } //
};
enum class FARM_TYPE : u8 { CONSTRUCTION, WINE, WHEAT, FISH, COWS };
enum class RESOURCE_BUILDINGS : u8 { WOOD, FE, AG, AU };
enum class GOOD : u8 { WOOD, IRON, FOOD, STEEL, COAL };

struct Data {
    std::unordered_map<FARM_TYPE, FarmStats> farm_types;
};

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
    Entity Add(Entity);
    b8 Remove(Entity entity) override;
};
struct Sector : Archetype {
    pce::Parent planets;
};
struct FarmSectorArchetype final : Sector {
    pce::Component<FARM_TYPE> type;
    Entity Add(Entity player, FARM_TYPE farm_type);
    b8 Remove(Entity entity) override;
};

struct PlayerArchetype final : Archetype {
    pce::Component<Money> moneys;
    pce::Component<ConstructionQueue> construction_queue;
    Entity Add(Money);
    b8 Remove(Entity entity) override;
};

struct PlanetArchetype final : Archetype {
    pce::Parent players;
    pce::Component<Money> moneys;
    pce::Component<ConstructionQueue> construction_queue;
    pce::Component<Tick> ages;

    Entity Add(Tick, Money);
    b8 Remove(Entity entity) override;
};

inline PlayerArchetype player_archetype;
inline StateArchetype state_archetype;
inline FarmSectorArchetype farm_archetype;
inline PlanetArchetype planet_archetype;
} // namespace pcg
