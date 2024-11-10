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
using pce::Span;
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

struct Planet : Entity {
    constexpr Planet(const Entity entity) : Entity(entity) { } // NOLINT(*-explicit-constructor, *-explicit-conversions)

    [[nodiscard]] constexpr OptionalEntity<Player> Player() const { return OptionalEntity { pcg::Player { planet_archetype.players[index] } }; }

    [[nodiscard]] constexpr Money& Money() const { return planet_archetype.moneys[index]; }
    [[nodiscard]] constexpr ConstructionQueue& ConstructionQueue() const { return planet_archetype.construction_queue[index]; }
    [[nodiscard]] constexpr Tick& Age() const { return planet_archetype.ages[index]; }

    constexpr void Take(const pcg::Player player) { planet_archetype.players[index] = player; }
};

struct Farm : Entity {
    constexpr Farm(const Entity entity) : Entity(entity) { } // NOLINT(*-explicit-constructor, *-explicit-conversions)

    [[nodiscard]] constexpr Planet Planet() const { return farm_archetype.planets[index]; }

    [[nodiscard]] constexpr FARM_TYPE FarmType() const { return farm_archetype.type[index]; }
    [[nodiscard]] FarmStats FarmStats() const { return data.farm_types[FarmType()]; }
};

struct RevenueCapture {
    Component<Money> before_revenue;
    Component<Money> revenue;
    Component<Money> before_expenses;
    Component<Money> expenses;
    Component<Money> balance;
};

Table PrintRevenueCapture(const String& name, const Archetype& archetype, const RevenueCapture& revenue_capture, const List<String>& constructing) {
    constexpr u32 width = 20U;
    Table table(name, archetype.Count);
    table.AddColumn("Revenue ", revenue_capture.revenue);
    table.AddColumn("Expenses ", revenue_capture.expenses);
    table.AddColumn("Balance ", revenue_capture.balance);
    table.AddColumnFixed("Constructing ", constructing, width);
    return table;
}

static b8 TryQueueFarm(Planet planet, FARM_TYPE type, Money& money);
static void ProcessIncome(const Farm farm) {
    constexpr f32 tax_rate = .2F;
    Money income = Money { static_cast<f32>(farm.FarmStats().production) };
    const f32 tax = static_cast<f32>(math::FloorToU32(income.Value() * tax_rate));
    const Money playerTax { tax };
    income -= playerTax;
    farm.Planet().Money() += income;
    farm.Planet().Player().Entity().Money() += playerTax;
}
static void ProcessConstructionQueue(Planet player);
constexpr u16 BUILDING_TIME = 30U;

void Game::PlayTick(Tick tick, const b8 debug) {
    // Construction
    for (const Player player : player_archetype) { ProcessConstructionQueue(player); }
    for (const Planet planet : planet_archetype) { ProcessConstructionQueue(planet); }

    RevenueCapture player_revenue_capture;
    RevenueCapture planet_revenue_capture;
    player_revenue_capture.before_revenue = player_archetype.moneys;
    planet_revenue_capture.before_revenue = planet_archetype.moneys;
    // Income Farms
    for (const Farm farm : farm_archetype) { ProcessIncome(farm); }
    player_revenue_capture.revenue = Select(player_archetype.moneys, player_revenue_capture.before_revenue, math::Sub<Money>);
    planet_revenue_capture.revenue = Select(planet_archetype.moneys, planet_revenue_capture.before_revenue, math::Sub<Money>);

    planet_revenue_capture.before_expenses = planet_archetype.moneys;
    // Buy Farms
    for (const Planet planet : planet_archetype) {
        if (planet.Player().IsNone()) { continue; }
        FARM_TYPE farm_type = RandomKey(data.farm_types);
        (void)TryQueueFarm(planet, farm_type, planet.Money());
    }
    planet_revenue_capture.expenses = Select(planet_archetype.moneys, planet_revenue_capture.before_expenses, math::Sub<Money>);
    planet_revenue_capture.balance = planet_archetype.moneys;

    player_revenue_capture.before_expenses = player_archetype.moneys;
    // Buy Farms
    for (const Player player : player_archetype) {
        if (planet_archetype.Count == 0U) { continue; }
        Planet planet = Planet { Entity { pce::Rand() % planet_archetype.Count } };
        if (planet.Player().IsNone() || planet.Player().Entity() != player) { continue; }
        FARM_TYPE farm_type = RandomKey(data.farm_types);
        (void)TryQueueFarm(planet, farm_type, player.Money());
    }
    // Settle, Exploit, Research
    for (const Player player : player_archetype) {
        if (pce::Rand() % 100U == 0U) {
            OptionalEntity<> optional = planet_archetype.Add(tick, Money { 0.0F });
            if (optional.IsNone()) { continue; }
            Planet(optional.Entity()).Take(player);
        }
    }
    player_revenue_capture.expenses = Select(player_archetype.moneys, player_revenue_capture.before_expenses, math::Sub<Money>);
    player_revenue_capture.balance = player_archetype.moneys;

    for (Market& market : state_archetype.markets) { market.RecalculateMarkets(); }

    if (!debug) { return; }

    List<String> constructing = Select(player_archetype.construction_queue, [] (const ConstructionQueue& construction_queue) -> String {
        return { "{:3} / {:3} Q:{:3}", math::Min(construction_queue.Size(), construction_queue.construction_capacity), construction_queue.construction_capacity, construction_queue.Size() };
    });

    Table player_table = PrintRevenueCapture("Player Economy", player_archetype, player_revenue_capture, constructing);
    player_table.Print(logger, Table::COLOR_ENABLED);

    constructing = Select(planet_archetype.construction_queue, [] (const ConstructionQueue& construction_queue) -> String {
        return { "{:3} / {:3} Q:{:3}", math::Min(construction_queue.Size(), construction_queue.construction_capacity), construction_queue.construction_capacity, construction_queue.Size() };
    });

    Table planet_table = PrintRevenueCapture("Planet Economy", planet_archetype, planet_revenue_capture, constructing);\
    planet_table.AddColumn("Owner", Span<Entity>(planet_archetype.players));
    planet_table.Print(logger, Table::COLOR_DISABLED);

    if (!debug) { return; }

    const List<BuildingUnderConstruction> display_construction = player_archetype.construction_queue[0U].Limit(5); //.Limit(10);
    Table construction_table("ConstructionQueue", display_construction.Size());
    construction_table.AddColumn("Type", Select(display_construction, [] (const BuildingUnderConstruction& building) -> FARM_TYPE { return building.type; }));
    construction_table.AddColumn("Progress", Select(display_construction, [] (const BuildingUnderConstruction& building) -> u16 { return building.progress; }));
    construction_table.AddColumn("Required", Select(display_construction, [] (const BuildingUnderConstruction& building) -> u16 { return BUILDING_TIME; }));
    construction_table.Print(logger, Table::COLOR_DISABLED);
}

Game::Game(const NewGameSettings game) {
    constexpr Tick start_tick = Tick { 0U };

    data.farm_types[FARM_TYPE::CONSTRUCTION] = FarmStats { .cost = 200U, .production = 0U };
    data.farm_types[FARM_TYPE::FISH] = FarmStats { .cost = 100U, .production = 1U };
    data.farm_types[FARM_TYPE::WHEAT] = FarmStats { .cost = 200U, .production = 3U };
    data.farm_types[FARM_TYPE::COWS] = FarmStats { .cost = 400U, .production = 10U };
    data.farm_types[FARM_TYPE::WINE] = FarmStats { .cost = 1000U, .production = 50U };

    for (u32 i = 0U; i < game.players; i++) {
        const Entity player { i };
        constexpr Money player_money_start = Money { 10000.0F };
        (void)player_archetype.Add(player_money_start);
        (void)farm_archetype.Add(player, FARM_TYPE::CONSTRUCTION);
        Player(player).ConstructionQueue().construction_capacity++;
    }

    for (u32 i = 0U; i < game.planets; i++) {
        const Entity planet { i };
        constexpr Money planet_money_start = Money { 100.0F };
        (void)planet_archetype.Add(start_tick, planet_money_start);
        (void)farm_archetype.Add(planet, FARM_TYPE::CONSTRUCTION);
        Planet(planet).ConstructionQueue().construction_capacity++;

        const Player owner { Entity { pce::Rand() % game.players } };
        Planet(planet).Take(owner);
    }
}

static b8 TryQueueFarm(const Planet planet, const FARM_TYPE type, Money& money) {
    const FarmStats farm = data.farm_types[type];
    const Money cost(static_cast<f32>(farm.cost)); // NOLINT(*-narrowing-conversions)
    if (money < cost) { return false; }
    money -= cost;
    (void)planet.ConstructionQueue().EmplaceBack(type);
    return true;
}

static void ProcessConstructionQueue(const Planet player) {
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
