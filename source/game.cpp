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
    [[nodiscard]] constexpr ConstructionQueue& ConstructionQueue() const { return player_archetype.construction_queue[index]; }
};

static bool TryQueueFarm(Player player, FARM_TYPE type);
static void ProcessIncome(const Farm farm) { farm.Player().Money().value += static_cast<f32>(farm.FarmStats().production); }
static void ProcessConstructionQueue(Player player);
constexpr u16 BUILDING_TIME = 30U;

void Game::PlayTick(struct Tick tick, bool debug) {
    for (const Player player : player_archetype) { ProcessConstructionQueue(player); }
    for (const Planet planet : planet_archetype) { ProcessConstructionQueue(planet); }

    const Component<Money> before_revenue = player_archetype.moneys;
    for (const Farm farm : farm_archetype) { ProcessIncome(farm); }
    const Component<Money> revenue = Select(player_archetype.moneys, before_revenue, math::Minus<Money>());

    const Component<Money> before_expenses = player_archetype.moneys;
    for (const Player player : player_archetype) { (void)TryQueueFarm(player, RandomKey(data.farm_types)); }
    const Component<Money> expenses = Select(player_archetype.moneys, before_expenses, math::Minus<Money>());

    for (Market& market : state_archetype.markets) { market.RecalculateMarkets(); }

    if (!debug) { return; }

    const List<String> constructing = Select(player_archetype.construction_queue, [] (const ConstructionQueue& construction_queue) -> String {
        return { "{} / {} Q:{}", math::Min(construction_queue.Size(), construction_queue.construction_capacity), construction_queue.construction_capacity, construction_queue.Size() };
    });
    Table table("Player", player_archetype.Count);
    table.AddColumn("Revenue", revenue);
    table.AddColumn("Expenses", expenses);
    table.AddColumn("Balance", player_archetype.moneys);
    table.AddColumnFixed("Constructing", constructing, 20);
    table.Print(logger, Table::COLOR_ENABLED);

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
        constexpr Money player_money_start = Money{ 100.0F };
        (void)player_archetype.Add(player_money_start);
        (void)farm_archetype.Add(i, FARM_TYPE::CONSTRUCTION);
        Player(i).ConstructionQueue().construction_capacity++;
    }
}

static bool TryQueueFarm(const Player player, FARM_TYPE type) {
    const FarmStats farm = data.farm_types[type];
    const Money cost(static_cast<f32>(farm.cost)); // NOLINT(*-narrowing-conversions)
    if (player.Money().value < cost.value) { return false; }
    player.Money().value -= cost.value;
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
