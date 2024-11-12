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
    [[nodiscard]] constexpr Money& Balance() const { return player_archetype.moneys[index]; }
    [[nodiscard]] constexpr ConstructionQueue& ConstructionQueue() const { return player_archetype.construction_queue[index]; }
};

struct Planet : Entity {
    constexpr Planet(const Entity entity) : Entity(entity) { } // NOLINT(*-explicit-constructor, *-explicit-conversions)

    [[nodiscard]] constexpr OptionalEntity<Player> Player() const { return OptionalEntity { pcg::Player { planet_archetype.players[index] } }; }

    [[nodiscard]] constexpr Money& Balance() const { return planet_archetype.moneys[index]; }
    [[nodiscard]] constexpr ConstructionQueue& ConstructionQueue() const { return planet_archetype.construction_queue[index]; }
    [[nodiscard]] constexpr Population& Population() const { return planet_archetype.populations[index]; }
    [[nodiscard]] constexpr Money& PopulationBalance() const { return planet_archetype.population_balance[index]; }
    [[nodiscard]] constexpr QualityOfLife& QualityOfLife() const { return planet_archetype.population_quality_of_life[index]; }
    [[nodiscard]] constexpr Tick& Age() const { return planet_archetype.ages[index]; }

    constexpr void TransferOwnerShipToPlayer(const pcg::Player player) const { planet_archetype.players[index] = player; }
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

void AddRevenueCapture(Table& table, const RevenueCapture& revenue_capture) {
    table.AddColumn("Revenue ", revenue_capture.revenue);
    table.AddColumn("Expenses ", revenue_capture.expenses);
    table.AddColumn("Balance ", revenue_capture.balance);
}

static QualityOfLife GetQualityOfLife(Planet planet);
static b8 TryQueueFarm(Planet planet, FARM_TYPE type, Money& money);
static void ProcessIncome(const Farm farm) {
    constexpr Percentage tax_rate { .2F };
    Money result = farm.FarmStats().FinancialResult(1U);
    if (result > Money { 0.0F } && farm.Planet().Player().IsSome()) {
        const f32 tax = static_cast<f32>(math::FloorToU32(result.Value() * tax_rate.Value()));
        const Money player_tax { tax };
        result -= player_tax;
        farm.Planet().Player().Entity().Balance() += player_tax;
    }
    farm.Planet().Balance() += result;
}
static void ProcessConstructionQueue(Planet player);
constexpr f32 PER_MONTH = 1.0F / 12.0F;
constexpr f32 PER_THOUSAND = 1.0F / 1'000.0F;
constexpr f32 PER_MILLION = 1.0F / 1'000'000.0F;

constexpr u16 BUILDING_TIME = 30U;
constexpr f32 GROWTH_RATE_PER_MONTH = 0.0025F * PER_MONTH;
constexpr f32 PASSIVE_INCOME = 0.2F;

void Game::PlayTick(Tick tick, const b8 debug) {
    // Construction
    for (const Player player : player_archetype) { ProcessConstructionQueue(player); }
    for (const Planet planet : planet_archetype) { ProcessConstructionQueue(planet); }
    // Population
    for (const Planet planet : planet_archetype) {
        Percentage growth_rate = Percentage { 1.0F + GROWTH_RATE_PER_MONTH};
        Population population = planet.Population();
        planet.Population() = Population { population.Value() * growth_rate.Value() };
    }
    RevenueCapture player_revenue_capture;
    RevenueCapture planet_revenue_capture;
    player_revenue_capture.before_revenue = player_archetype.moneys;
    planet_revenue_capture.before_revenue = planet_archetype.moneys;
    // Passive income
    for (const Planet planet : planet_archetype) { planet.PopulationBalance() += Money { planet.Population().Value() * PASSIVE_INCOME }; }
    // Income Farms
    for (const Farm farm : farm_archetype) { ProcessIncome(farm); }
    player_revenue_capture.revenue = Select(player_archetype.moneys, player_revenue_capture.before_revenue, math::Sub<Money>);
    planet_revenue_capture.revenue = Select(planet_archetype.moneys, planet_revenue_capture.before_revenue, math::Sub<Money>);

    planet_revenue_capture.before_expenses = planet_archetype.moneys;
    // Buy Farms
    for (const Planet planet : planet_archetype) { (void)TryQueueFarm(planet, RandomKey(data.farm_types), planet.Balance()); }
    planet_revenue_capture.expenses = Select(planet_archetype.moneys, planet_revenue_capture.before_expenses, math::Sub<Money>);
    planet_revenue_capture.balance = planet_archetype.moneys;

    player_revenue_capture.before_expenses = player_archetype.moneys;
    // Buy Farms
    for (const Player player : player_archetype) {
        if (planet_archetype.Count == 0U) { continue; }
        Planet planet = Planet { Entity { pce::Rand() % planet_archetype.Count } };
        if (planet.Player().IsNone() || planet.Player().Entity() != player) { continue; }
        FARM_TYPE farm_type = RandomKey(data.farm_types);
        (void)TryQueueFarm(planet, farm_type, player.Balance());
    }
    // Settle, Exploit, Research
    for (const Player player : player_archetype) {
        if (pce::Rand() % 100U == 0U) {
            constexpr f32 start_money { 0.0F };
            constexpr f32 starting_population { 10'000.0F };
            Planet planet = planet_archetype.Add(tick, Money { start_money }, Population { starting_population });
            planet.TransferOwnerShipToPlayer(player);
        }
    }
    player_revenue_capture.expenses = Select(player_archetype.moneys, player_revenue_capture.before_expenses, math::Sub<Money>);
    player_revenue_capture.balance = player_archetype.moneys;

    // Buying goods
    Component<Money> population_balance = planet_archetype.population_balance;
    for (const Planet planet : planet_archetype) {
        planet.QualityOfLife() = GetQualityOfLife(planet);
        planet.PopulationBalance() = Money { 0.0F };
    }
    for (Market& market : state_archetype.markets) { market.RecalculateMarkets(); }

    if (!debug) { return; }

    auto construction_queue_to_string = [] (const ConstructionQueue& construction_queue) -> String {
        return { "{:3} / {:3} Q:{:3}", math::Min(construction_queue.Size(), construction_queue.construction_capacity), construction_queue.construction_capacity, construction_queue.Size() };
    };

    constexpr u32 width = 20U;

    Table player_table { "Player Economy", player_archetype.Count };
    AddRevenueCapture(player_table, player_revenue_capture);
    player_table.AddColumnFixed("Constructing ", Select(player_archetype.construction_queue, construction_queue_to_string), width);
    player_table.Print(logger, Table::COLOR_ENABLED);

    Table planet_table { "Planet Economy", planet_archetype.Count };
    planet_table.AddColumn("Population", planet_archetype.populations);
    planet_table.AddColumn("Pop Finances", population_balance);
    planet_table.AddColumn("QoL", planet_archetype.population_quality_of_life);
    planet_table.AddColumnFixed("Constructing ", Select(planet_archetype.construction_queue, construction_queue_to_string), width);

    planet_table.AddColumn("Owner", planet_archetype.players);
    planet_table.Print(logger, Table::COLOR_DISABLED);

    if (!debug) { return; }

    const List<BuildingUnderConstruction> display_construction = player_archetype.construction_queue[0U].Limit(5U); //.Limit(10);
    Table construction_table("ConstructionQueue", display_construction.Size());
    construction_table.AddColumn("Type", Select(display_construction, [] (const BuildingUnderConstruction& building) -> FARM_TYPE { return building.type; }));
    construction_table.AddColumn("Progress", Select(display_construction, [] (const BuildingUnderConstruction& building) -> u16 { return building.progress; }));
    construction_table.AddColumn("Required", Select(display_construction, [] ([[maybe_unused]] const BuildingUnderConstruction& building) -> u16 { return BUILDING_TIME; }));
    construction_table.Print(logger, Table::COLOR_DISABLED);
}

Game::Game(const NewGameSettings game) {
    constexpr Tick start_tick = Tick { 0U };

    data.farm_types[FARM_TYPE::CONSTRUCTION] = FarmStats { .assets = 200U, .production = 0U, .depreciation = 1U };
    data.farm_types[FARM_TYPE::FISH] = FarmStats { .assets = 100U, .production = 1U, .depreciation = 1U };
    data.farm_types[FARM_TYPE::WHEAT] = FarmStats { .assets = 200U, .production = 3U, .depreciation = 1U };
    data.farm_types[FARM_TYPE::COWS] = FarmStats { .assets = 400U, .production = 10U, .depreciation = 5U };
    data.farm_types[FARM_TYPE::WINE] = FarmStats { .assets = 1000U, .production = 50U, .depreciation = 20U };

    for (u32 i = 0U; i < game.players; i++) {
        constexpr Money player_money_start = Money { 10000.0F };
        Player player = player_archetype.Add(player_money_start);
        (void)farm_archetype.Add(player, FARM_TYPE::CONSTRUCTION);
        player.ConstructionQueue().construction_capacity++;
    }

    for (u32 i = 0U; i < game.planets; i++) {
        Planet planet = planet_archetype.Add(start_tick, Money { 100.0F }, Population { 100'000'000.0F });
        (void)farm_archetype.Add(planet, FARM_TYPE::CONSTRUCTION);
        planet.ConstructionQueue().construction_capacity++;
        const Player owner { Entity { pce::Rand() % game.players } };
        planet.TransferOwnerShipToPlayer(owner);
    }
}

static b8 TryQueueFarm(const Planet planet, const FARM_TYPE type, Money& money) {
    const FarmStats farm = data.farm_types[type];
    const Money cost(static_cast<f32>(farm.assets)); // NOLINT(*-narrowing-conversions)
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

static QualityOfLife GetQualityOfLife(Planet planet) {
    constexpr Money price_of_food = Money { 0.05F };
    constexpr Money price_of_restaurants = Money { 0.10F };
    constexpr Money price_of_cars = Money { 1.00F };
    constexpr Money price_of_watches = Money { 10.0F };
    constexpr Money price_of_ships = Money { 100.0F };
    constexpr Money price_of_dictator = Money { 1000.0F };

    const QualityOfLife quality_of_life = planet.QualityOfLife();
    const QUALITY_OF_LIFE_STAGE stage = GetQualityOfLifeStage(quality_of_life);
    const f32 inter_level = (quality_of_life - QualityOfLife { static_cast<f32>(stage) } * QUALITY_OF_LIFE_LEVELS_PER_STAGE).Value();

    Money balance = Money { planet.PopulationBalance().Value() / planet.Population().Value() };

    // VITAMINS
    // needs should be like vic 3
    // exponential
    switch (stage) {
        case QUALITY_OF_LIFE_STAGE::DYING:
            // life needs
            // food, water, heating
            // water is free
            balance -= Money { price_of_food.Value() * inter_level };
            break;
        case QUALITY_OF_LIFE_STAGE::SURVIVING: balance -= Money { (price_of_food.Value() * inter_level) + 5.0F };
            break;
        case QUALITY_OF_LIFE_STAGE::STRUGGLING: balance -= Money { (price_of_food.Value() * inter_level) + 10.0F };
            break;
        case QUALITY_OF_LIFE_STAGE::SECURE:
            // Basic needs
            // basic types of food
            balance -= Money { price_of_restaurants.Value() * inter_level };
            break;
        case QUALITY_OF_LIFE_STAGE::COMFORTABLE:
            // nice to have
            // beer car
            // services
            balance -= Money { price_of_cars.Value() * inter_level };
            break;
        case QUALITY_OF_LIFE_STAGE::LAVISH:
            // luxury
            // wine, watches luxuries
            // more service
            balance -= Money { price_of_watches.Value() * inter_level };
            break;
        case QUALITY_OF_LIFE_STAGE::EXTRAVAGANT:
            // hard luxuries
            // ships spaceships excessive housing
            // servitors
            balance -= Money { price_of_ships.Value() * inter_level };
            break;
        case QUALITY_OF_LIFE_STAGE::DICTATOR:
            // richest guy on earth
            // enslaving
            balance -= Money { price_of_dictator.Value() * quality_of_life.Value() };
            break;
    }
    const QualityOfLife quality_of_life_change { static_cast<f32>(balance > Money { 0.0F }) - static_cast<f32>(balance < Money { 0.0F }) };
    return quality_of_life + quality_of_life_change;
}
} // namespace pcg
