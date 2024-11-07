// ReSharper disable CppNonExplicitConvertingConstructor
#include "game.hpp"
#include "algorithm.hpp"
#include "components.hpp"

#include "logger.hpp"
#include "types.hpp"
#include "util.hpp"

namespace pcg {
using pce::Component;
using pce::String;
using pce::RandomKey;
using pce::List;
using pce::Table;
using pce::Select;
using pce::Logger;
namespace math = pce::math;

struct Player : Entity {
    constexpr Player(const Entity entity) : Entity(entity) { } // NOLINT(*-explicit-constructor, *-explicit-conversions)
    [[nodiscard]] constexpr Money& Money() const { return player_archetype.moneys[index]; }
    [[nodiscard]] constexpr ConstructionQueue& ConstructionQueue() const { return player_archetype.construction_queue[index]; }
};

struct Farm : Entity {
    constexpr Farm(const Entity entity) : Entity(entity) { } // NOLINT(*-explicit-constructor, *-explicit-conversions)

    [[nodiscard]] constexpr Player Player() const { return farm_archetype.players[index]; }

    [[nodiscard]] constexpr FARM_TYPE FarmType() const { return farm_archetype.type[index]; }
    [[nodiscard]] FarmStats FarmStats() const { return data.farm_types[FarmType()]; }
};

struct Planet : Entity {
    constexpr Planet(const Entity entity) : Entity(entity) { } // NOLINT(*-explicit-constructor, *-explicit-conversions)
    [[nodiscard]] constexpr Money& Money() const { return planet_archetype.moneys[index]; }
    [[nodiscard]] constexpr ConstructionQueue& ConstructionQueue() const { return planet_archetype.construction_queue[index]; }
    [[nodiscard]] constexpr Tick& Age() const { return planet_archetype.ages[index]; }
};

struct RevenueCapture {
    Component<Money> before_revenue;
    Component<Money> revenue;
    Component<Money> before_expenses;
    Component<Money> expenses;
    Component<Money> balance;
};

void PrintRevenueCapture(Logger& logger, const String name, Archetype& archetype, RevenueCapture& revenue_capture, List<String> constructing) {
    Table player_table(name, archetype.Count);
    player_table.AddColumn("Revenue", revenue_capture.revenue);
    player_table.AddColumn("Expenses", revenue_capture.expenses);
    player_table.AddColumn("Balance", revenue_capture.balance);
    player_table.AddColumnFixed("Constructing", constructing, 20);
    player_table.Print(logger, Table::COLOR_ENABLED);
}

static bool TryQueueFarm(Player player, FARM_TYPE type);
static void ProcessIncome(const Farm farm) { farm.Player().Money() += Money { static_cast<f32>(farm.FarmStats().production) }; }
static void ProcessConstructionQueue(Player player);
constexpr u16 BUILDING_TIME = 30U;

void Game::PlayTick(Tick tick, const bool debug) {
    RevenueCapture player_revenue_capture, planet_revenue_capture;
    for (const Player player : player_archetype) { ProcessConstructionQueue(player); }
    for (const Planet planet : planet_archetype) { ProcessConstructionQueue(planet); }

    player_revenue_capture.before_revenue = player_archetype.moneys;
    planet_revenue_capture.before_revenue = planet_archetype.moneys;
    for (const Farm farm : farm_archetype) { ProcessIncome(farm); }
    player_revenue_capture.revenue = Select(player_archetype.moneys, player_revenue_capture.before_revenue, math::Minus<Money>());
    planet_revenue_capture.revenue = Select(planet_archetype.moneys, planet_revenue_capture.before_revenue, math::Minus<Money>());

    player_revenue_capture.before_expenses = player_archetype.moneys;
    planet_revenue_capture.before_expenses = planet_archetype.moneys;
    for (const Player player : player_archetype) { (void)TryQueueFarm(player, RandomKey(data.farm_types)); }
    for (const Planet planet : planet_archetype) { (void)TryQueueFarm(planet, RandomKey(data.farm_types)); }
    player_revenue_capture.expenses = Select(player_archetype.moneys, player_revenue_capture.before_expenses, math::Minus<Money>());
    planet_revenue_capture.expenses = Select(planet_archetype.moneys, planet_revenue_capture.before_expenses, math::Minus<Money>());
    player_revenue_capture.balance = player_archetype.moneys;
    planet_revenue_capture.balance = planet_archetype.moneys;

    for (Market& market : state_archetype.markets) { market.RecalculateMarkets(); }

    if (!debug) { return; }

    List<String> constructing = Select(player_archetype.construction_queue, [] (const ConstructionQueue& construction_queue) -> String {
        return { "{} / {} Q:{}", math::Min(construction_queue.Size(), construction_queue.construction_capacity), construction_queue.construction_capacity, construction_queue.Size() };
    });

    PrintRevenueCapture(logger, "Player", player_archetype, player_revenue_capture, constructing);

    constructing = Select(planet_archetype.construction_queue, [] (const ConstructionQueue& construction_queue) -> String {
        return { "{} / {} Q:{}", math::Min(construction_queue.Size(), construction_queue.construction_capacity), construction_queue.construction_capacity, construction_queue.Size() };
    });

    PrintRevenueCapture(logger, "Planet", planet_archetype, planet_revenue_capture, constructing);

    if (!debug) { return; }

    const List<BuildingUnderConstruction> display_construction = player_archetype.construction_queue[0U].Limit(5); //.Limit(10);
    Table construction_table("ConstructionQueue", display_construction.Size());
    construction_table.AddColumn("Type", Select(display_construction, [] (const BuildingUnderConstruction& building) -> FARM_TYPE { return building.type; }));
    construction_table.AddColumn("Progress", Select(display_construction, [] (const BuildingUnderConstruction& building) -> u16 { return building.progress; }));
    construction_table.AddColumn("Required", Select(display_construction, [] (const BuildingUnderConstruction& building) -> u16 { return BUILDING_TIME; }));
    construction_table.Print(logger, Table::COLOR_DISABLED);
}

Game::Game(const u32 players) {
    data.farm_types[FARM_TYPE::CONSTRUCTION] = FarmStats { .cost = 200U, .production = 0U };
    data.farm_types[FARM_TYPE::FISH] = FarmStats { .cost = 100U, .production = 1U };
    data.farm_types[FARM_TYPE::WHEAT] = FarmStats { .cost = 200U, .production = 3U };
    data.farm_types[FARM_TYPE::COWS] = FarmStats { .cost = 400U, .production = 10U };
    data.farm_types[FARM_TYPE::WINE] = FarmStats { .cost = 1000U, .production = 50U };

    for (u32 i = 0U; i < players; i++) {
        constexpr Money player_money_start = Money { 100.0F };
        (void)player_archetype.Add(player_money_start);
        (void)farm_archetype.Add(i, FARM_TYPE::CONSTRUCTION);
        Player(i).ConstructionQueue().construction_capacity++;
    }
}

static bool TryQueueFarm(const Player player, FARM_TYPE type) {
    const FarmStats farm = data.farm_types[type];
    const Money cost(static_cast<f32>(farm.cost)); // NOLINT(*-narrowing-conversions)
    if (player.Money() < cost) { return false; }
    player.Money() -= cost;
    (void)player.ConstructionQueue().EmplaceBack(type);
    return true;
}

static void ProcessConstructionQueue(const Player player) {
    ConstructionQueue& construction_queue = player.ConstructionQueue();
    if (construction_queue.Empty()) { return; }
    const u32 limit = math::Min(construction_queue.construction_capacity, construction_queue.Size()); // explicit copy
    for (u32 i = 0U; i < limit; i++) { construction_queue[i].progress += 1U; }
    for (u32 i = 0U; i < limit; i++) {
        const u32 idx = limit - i - 1U;
        const BuildingUnderConstruction building_under_construction = construction_queue[idx];
        if (building_under_construction.progress != BUILDING_TIME) { continue; }
        construction_queue.RemoveAt(idx);
        if (building_under_construction.type == FARM_TYPE::CONSTRUCTION) { construction_queue.construction_capacity += 1U; }
        (void)farm_archetype.Add(player, building_under_construction.type);
    }
}
} // namespace pcg
