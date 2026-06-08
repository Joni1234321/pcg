module;

export module pcg.g_components;

import pce.math;
import pce.u_ecs;
import pce.systems.t_tick_system;
import pce.collections;
import pce.std;

export namespace pcg {
using Percentage = pce::StrongType<f32, struct PercentageTag, pce::Arithmetic>;

using Money = pce::StrongType<f32, struct MoneyTag, pce::Arithmetic, pce::FormatLongNumber>;
using Population = pce::StrongType<f32, struct PopulationTag, pce::Arithmetic, pce::FormatLongNumber>;
using Production = pce::StrongType<u32, struct ProductionTag, pce::Arithmetic>;

using QualityOfLife = pce::StrongType<f32, struct QualityOfLifeTag, pce::Arithmetic>;
// theory aktiver og passiver. aktiver is the initial cost of investments
// https://ordbog.ku.dk/pdf/financial_terminology.pdf
// https://uwaterloo.ca/economics/sites/default/files/uploads/documents/econ-101-syllabus.pdf
// https://en.wikipedia.org/wiki/Supply_and_demand
// https://corporatefinanceinstitute.com/resources/accounting/types-of-assets/

struct FinancialAssets {
    Money inventory;
    Money financial;
    Money property_plant_equipment;
    Money others;
    [[nodiscard]] Money Total() const { return inventory + financial + property_plant_equipment + others; }
};
struct Stats {
    u32 input_goods;
    u32 output_goods;
    u32 employees_per_level;
    u32 property_plant_equipment_per_level;
    u32 property_plant_equipment_lifetime;
};
struct Finance {
    FinancialAssets assets;
    u32 level;
    Money liabilities;
    Money equity;
    Money last_result;
    Population employees;

    [[nodiscard]] Money NetIncome(const Stats& stats, const u32 cost_of_good) const { return Income(stats, cost_of_good) - Expenses(stats); }                                        // resultat
    [[nodiscard]] Percentage ReturnOnAssets(const Stats& stats, const u32 cost_of_good) const { return Percentage { NetIncome(stats, cost_of_good).value / assets.Total().value }; } // afkastningsgrad
private:
    [[nodiscard]] static Money Expenses(const Stats& stats) { return Money { 0.0F }; }
    [[nodiscard]] static Money Income(const Stats& stats, const u32 cost_of_good) { return Money { static_cast<f32>(stats.output_goods * cost_of_good) }; }
};
enum class FarmType : u8 { Wine, Wheat, Fish, Cows };
enum class ResourceBuildings : u8 { Wood, Fe, Ag, Au };
enum class Good : u8 { Wood, Iron, Food, Steel, Coal };
enum class QualityOfLifeStage : u8 { Dying, Surviving, Struggling, Secure, Comfortable, Lavish, Dictator, Extravagant };

struct Data {
    pce::UnorderedMap<FarmType, Stats> farm_types;
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
QualityOfLifeStage GetQualityOfLifeStage(const QualityOfLife quality_of_life) { return static_cast<QualityOfLifeStage>(pce::math::Min((quality_of_life / QUALITY_OF_LIFE_LEVELS_PER_STAGE).value, static_cast<f32>(QualityOfLifeStage::Extravagant))); }
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
    pce::Component<FarmType> types;
    pce::Component<Finance> finances;
    pce::Component<Money> population_balance;
    Entity Add(Entity player, FarmType farm_type);
    b8 Remove(Entity entity) override;
};

struct PlayerArchetype final : Archetype {
    pce::Component<Money> moneys;
    pce::Component<ConstructionQueue> construction_queue;
    Entity Add(Money);
    b8 Remove(Entity entity) override;
};

enum class PlanetTemplate { Agriculture, Gaia, Playground, Barren };
struct PlanetArchetype final : Archetype {
    pce::Parent players;
    pce::Component<pce::Tick> ages;
    pce::Component<Money> moneys;
    pce::Component<Population> employed;
    pce::Component<Population> unemployed;
    pce::Component<QualityOfLife> population_quality_of_life;
    pce::Component<ConstructionQueue> construction_queue;
    Entity Add(pce::Tick, Money, Population);
    b8 Remove(Entity entity) override;
    Entity AddTemplate(pce::Tick, PlanetTemplate);
};

inline PlayerArchetype player_archetype;
inline StateArchetype state_archetype;
inline FarmSectorArchetype farm_archetype;
inline PlanetArchetype planet_archetype;
} // namespace pcg
