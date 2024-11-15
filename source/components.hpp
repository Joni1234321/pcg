#pragma once

#include <unordered_map>

#include "collections.hpp"
#include "ecs.hpp"
#include "types.hpp"

namespace pcg {
using Percentage = NamedType<f32, struct PercentageTag, Arithmetic>;

using Tick = NamedType<u32, struct TickTag, Arithmetic>;
using Money = NamedType<f32, struct MoneyTag, Arithmetic, FormatLongNumber>;
using Population = NamedType<f32, struct PopulationTag, Arithmetic, FormatLongNumber>;
using Production = NamedType<u32, struct ProductionTag, Arithmetic>;

using QualityOfLife = NamedType<f32, struct QualityOfLifeTag, Arithmetic>;
// theory aktiver og passiver. aktiver is the initial cost of investments
// https://ordbog.ku.dk/pdf/financial_terminology.pdf
// https://uwaterloo.ca/economics/sites/default/files/uploads/documents/econ-101-syllabus.pdf
// https://en.wikipedia.org/wiki/Supply_and_demand
struct alignas(16U) Stats {
    // financials
    u32 assets;       // aktiver
    u32 production;   // produktion
    u32 depreciation; // afskrivning
};
struct Finance {
    u32 level;
    Money assets;
    Money liabilities;
    Money equity;

    [[nodiscard]] Money FinancialResult(const Stats stats, const u32 cost_of_good) const { return Income(stats, cost_of_good) - Expenses(stats); }                                              // resultat
    [[nodiscard]] Percentage ReturnOnAssets(const Stats stats, const u32 cost_of_good) const { return Percentage { FinancialResult(stats, cost_of_good).Value() / static_cast<f32>(assets) }; } // afkastningsgrad
private:
    [[nodiscard]] Money Expenses(const Stats stats) const { return Money { static_cast<f32>(level * stats.depreciation) }; }
    [[nodiscard]] Money Income(const Stats stats, const u32 cost_of_good) const { return Money { static_cast<f32>(level * stats.production * cost_of_good) }; }
};
enum class FarmType : u8 { Construction, Wine, Wheat, Fish, Cows };
enum class ResourceBuildings : u8 { Wood, Fe, Ag, Au };
enum class Good : u8 { Wood, Iron, Food, Steel, Coal };
enum class QualityOfLifeStage : u8 { Dying, Surviving, Struggling, Secure, Comfortable, Lavish, Dictator, Extravagant, };

struct Data {
    std::unordered_map<FarmType, Stats> farm_types;
};

struct BuildingUnderConstruction {
    FarmType type;
    u16 progress;
};
struct ConstructionQueue : pce::Queue<BuildingUnderConstruction> {
    u32 construction_capacity { 1U };
};
struct PopulationFinance {
    Money money { 0.0F };
};
struct Market {
    u32 population { };
    u32 demand { };
    u32 sold = 0;
    i32 price_factor = 0;
    [[nodiscard]] f32 GetPrice() const { return 100.0F * static_cast<f32>(price_factor); }
    void RecalculateMarkets() {
        price_factor += sold < demand ? 1 : -1;
        sold = 0U;
    }
};
constexpr QualityOfLife QUALITY_OF_LIFE_LEVELS_PER_STAGE = QualityOfLife { 5.0F };
static QualityOfLifeStage GetQualityOfLifeStage(const QualityOfLife quality_of_life) {
    return static_cast<QualityOfLifeStage>(pce::math::Min((quality_of_life / QUALITY_OF_LIFE_LEVELS_PER_STAGE).Value(), static_cast<f32>(QualityOfLifeStage::Extravagant)));
}
struct StateArchetype final : Archetype {
    pce::Parent planets;
    pce::Component<Market> markets;
    Entity Add(Entity planet);
    b8 Remove(Entity entity) override;
};
struct Sector : Archetype {
    pce::Parent planets;
};
struct FarmSectorArchetype final : Sector {
    pce::Component<FarmType> type;
    pce::Component<Finance> finances;
    Entity Add(Entity player, FarmType farm_type);
    b8 Remove(Entity entity) override;
};

struct PlayerArchetype final : Archetype {
    pce::Component<Money> moneys;
    pce::Component<ConstructionQueue> construction_queue;
    Entity Add(Money);
    b8 Remove(Entity entity) override;
};

enum class PlanetTemplate { Agriculture, Gaia, Playground, Barren};
struct PlanetArchetype final : Archetype {
    pce::Parent players;
    pce::Component<Tick> ages;
    pce::Component<Money> moneys;
    pce::Component<Population> populations;
    pce::Component<Money> population_balance;
    pce::Component<QualityOfLife> population_quality_of_life;
    pce::Component<ConstructionQueue> construction_queue;
    Entity Add(Tick, Money, Population);
    b8 Remove(Entity entity) override;
    Entity AddTemplate (Tick, PlanetTemplate);
};

inline PlayerArchetype player_archetype;
inline StateArchetype state_archetype;
inline FarmSectorArchetype farm_archetype;
inline PlanetArchetype planet_archetype;

} // namespace pcg
