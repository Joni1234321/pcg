// ReSharper disable CppNonExplicitConvertingConstructor
#include "g_game.hpp"
#include "r_engine.hpp"
#include "u_algorithm.hpp"
#include "g_components.hpp"

#include "u_logger.hpp"
#include "u_types.hpp"
#include "u_util.hpp"
#include "u_table.hpp"

namespace pcg {
using pce::Component;
using pce::String;
using pce::RandomKey;
using pce::Span;
using pce::List;
using pce::LoggerTable;
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
    [[nodiscard]] constexpr Population& Unemployed() const { return planet_archetype.unemployed[index]; }
    [[nodiscard]] constexpr Population& Employed() const { return planet_archetype.employed[index]; }
    [[nodiscard]] constexpr QualityOfLife& QualityOfLife() const { return planet_archetype.population_quality_of_life[index]; }
    [[nodiscard]] constexpr Tick& Age() const { return planet_archetype.ages[index]; }

    constexpr void TransferOwnerShipToPlayer(const pcg::Player player) const { planet_archetype.players[index] = player; }

    [[nodiscard]] constexpr Population TotalPopulation() const { return Employed() + Unemployed(); }
};

struct Farm : Entity {
    constexpr Farm(const Entity entity) : Entity(entity) { } // NOLINT(*-explicit-constructor, *-explicit-conversions)

    [[nodiscard]] constexpr Planet Planet() const { return farm_archetype.planets[index]; }

    [[nodiscard]] constexpr FarmType FarmType() const { return farm_archetype.types[index]; }
    [[nodiscard]] constexpr Finance& Finance() const { return farm_archetype.finances[index]; }
    [[nodiscard]] constexpr Money& PopulationBalance() const { return farm_archetype.population_balance[index]; }
    [[nodiscard]] Stats Stats() const { return data.farm_types[FarmType()]; }
};

struct RevenueCapture {
    Component<Money> before_revenue;
    Component<Money> revenue;
    Component<Money> before_expenses;
    Component<Money> expenses;
    Component<Money> balance;
};

void AddRevenueCapture(LoggerTable& table, const RevenueCapture& revenue_capture) {
    table.AddColumn("Revenue ", revenue_capture.revenue);
    table.AddColumn("Expenses ", revenue_capture.expenses);
    table.AddColumn("Balance ", revenue_capture.balance);
}

static QualityOfLife GetQualityOfLife(Planet planet);
static b8 TryQueueFarm(Planet planet, FarmType type, Money& money);
static void ProcessConstructionQueue(Planet player);
constexpr u32 TICKS_PER_YEAR = 12U;
constexpr f32 PER_MONTH = 1.0F / 12.0F;
constexpr f32 PER_THOUSAND = 1.0F / 1'000.0F;
constexpr f32 PER_MILLION = 1.0F / 1'000'000.0F;

constexpr u16 BUILDING_TIME = 30U;
constexpr f32 GROWTH_RATE_PER_MONTH = 0.0025F * PER_MONTH;


void Game::PlayTick(Tick tick, pce::ui::UISystem& ui_system, const pce::ui::FontCollection& font, const b8 debug) {
    // Construction
    for (const Player player : player_archetype) { ProcessConstructionQueue(player); }
    for (const Planet planet : planet_archetype) { ProcessConstructionQueue(planet); }
    // Population
    for (const Planet planet : planet_archetype) {
        Percentage growth_rate = Percentage { 1.0F + GROWTH_RATE_PER_MONTH };
        planet.Unemployed() = Population { planet.TotalPopulation().Value() * growth_rate.Value() };
    }
    RevenueCapture player_revenue_capture;
    RevenueCapture planet_revenue_capture;
    player_revenue_capture.before_revenue = player_archetype.moneys;
    planet_revenue_capture.before_revenue = planet_archetype.moneys;
    for (const Farm farm : farm_archetype) {
        // What i would love is homogen markets with supply and demand
        // and then create supply based on the pops income.
        // then i could also introduce heterogenic markets where some goods are luxury and crazes

        Money result = farm.Finance().NetIncome(farm.Stats(), 1U);

        constexpr Money wage { 0.05F };
        const Money wages = Money { farm.Finance().employees.Value() * wage.Value() };
        result -= wages;
        farm.PopulationBalance() += wages;
        farm.Finance().assets.financial -= wages;
        farm.Finance().equity -= wages;

        const Percentage depreciation_rate { 1.0F / static_cast<f32>(farm.Stats().property_plant_equipment_lifetime) };
        const Money depreciation { farm.Finance().assets.property_plant_equipment.Value() * depreciation_rate.Value() };
        farm.Finance().assets.property_plant_equipment -= depreciation;
        farm.Finance().equity -= depreciation;

        if (result > Money { 0.0F }) {
            constexpr Percentage tax_rate { .2F };
            const Money player_tax { static_cast<f32>(math::FloorToU32(result.Value() * tax_rate.Value())) };
            result -= player_tax;
            if (farm.Planet().Player().IsSome()) { farm.Planet().Player().Entity().Balance() += player_tax; } else { farm.Planet().Balance() += player_tax; }

            constexpr Percentage dividend_rate { .5F };
            const Money dividends = Money { result.Value() * dividend_rate.Value() };
            result -= dividends;
            farm.Planet().Balance() += dividends;

            // ALSO decide on what to invest in, either automatisation or increase the population
            farm.Finance().assets.others += result; // also reinvest in other companies and buy the up
            farm.Finance().equity += result;
        } else {
            farm.Finance().assets.others += result;
            farm.Finance().equity += result;
        }
        farm.Finance().last_result = result;
    }
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
        FarmType farm_type = RandomKey(data.farm_types);
        (void)TryQueueFarm(planet, farm_type, player.Balance());
    }
    // Settle, Exploit, Research
    for (const Player player : player_archetype) {
        if (pce::Rand() % 100U == 0U) {
            Planet planet = planet_archetype.AddTemplate(tick, PlanetTemplate::Barren);
            planet.TransferOwnerShipToPlayer(player);
        }
    }
    player_revenue_capture.expenses = Select(player_archetype.moneys, player_revenue_capture.before_expenses, math::Sub<Money>);
    player_revenue_capture.balance = player_archetype.moneys;

    // Buying goods
    for (const Planet planet : planet_archetype) { planet.QualityOfLife() = GetQualityOfLife(planet); }
    for (Market& market : state_archetype.markets) { market.RecalculateMarkets(); }

    Table table ("Fish");
    table.AddColumn("Assets       ", List (10, 1U));
    table.AddColumn("Fish         ", List (10, 100U));
    table.AddColumn("Texture      ", List (10, 420U + tick.Value()));

    DrawTable(ui_system, table, font);
    return;

    LoggerTable farm_table { "Farms", farm_archetype.Count };
    farm_table.AddColumn("Type ", farm_archetype.types);
    farm_table.AddColumn("Assets       ", Select(farm_archetype.finances, [] (const Finance& finance) -> Money { return finance.assets.Total(); }));
    farm_table.AddColumn("Equity       ", Select(farm_archetype.finances, [] (const Finance& finance) -> Money { return finance.equity; }));
    farm_table.AddColumn("Liabilities  ", Select(farm_archetype.finances, [] (const Finance& finance) -> Money { return finance.liabilities; }));
    farm_table.AddColumn("Last result  ", Select(farm_archetype.finances, [] (const Finance& finance) -> Money { return finance.last_result; }));
    farm_table.AddColumn("Population balance", farm_archetype.population_balance);
    //string = farm_table.WriteToLogger(logger, LoggerTable::COLOR_DISABLED);
    //(void)TTF_SetTextString(ui_system[info_text].text, string.CString(), string.size());
    logger.Print();

    return;
    if (!debug) { return; }

    auto construction_queue_to_string = [] (const ConstructionQueue& construction_queue) -> String {
        return { "{:3} / {:3} Q:{:3}", math::Min(construction_queue.Size(), construction_queue.construction_capacity), construction_queue.construction_capacity, construction_queue.Size() };
    };

    constexpr u32 width = 20U;

    LoggerTable player_table { "Player Economy", player_archetype.Count };
    AddRevenueCapture(player_table, player_revenue_capture);
    player_table.AddColumnFixed("Constructing ", Select(player_archetype.construction_queue, construction_queue_to_string), width);
    player_table.Print(logger, LoggerTable::COLOR_ENABLED);

    LoggerTable planet_table { "Planet Economy", planet_archetype.Count };
    planet_table.AddColumn("Employed", planet_archetype.employed);
    planet_table.AddColumn("Unemployed", planet_archetype.unemployed);
    //planet_table.AddColumn("QoL", planet_archetype.population_quality_of_life);
    planet_table.AddColumnFixed("Constructing ", Select(planet_archetype.construction_queue, construction_queue_to_string), width);
    planet_table.AddColumn("Owner", planet_archetype.players);
    planet_table.Print(logger, LoggerTable::COLOR_DISABLED);

    const List<BuildingUnderConstruction> display_construction = player_archetype.construction_queue[0U].Limit(5U); //.Limit(10);
    LoggerTable construction_table("ConstructionQueue", display_construction.Size());
    construction_table.AddColumn("Type", Select(display_construction, [] (const BuildingUnderConstruction& building) -> FarmType { return building.type; }));
    construction_table.AddColumn("Progress", Select(display_construction, [] (const BuildingUnderConstruction& building) -> u16 { return building.progress; }));
    construction_table.AddColumn("Required", Select(display_construction, [] ([[maybe_unused]] const BuildingUnderConstruction& building) -> u16 { return BUILDING_TIME; }));
    construction_table.Print(logger, LoggerTable::COLOR_DISABLED);
}

Game::Game(const NewGameSettings game) {
    constexpr Tick start_tick = Tick { 0U };

    data.farm_types[FarmType::Fish] = Stats { .input_goods = 1U, .output_goods = 3U, .employees_per_level = 10U, .property_plant_equipment_per_level = 150U, .property_plant_equipment_lifetime = 5U * TICKS_PER_YEAR };
    data.farm_types[FarmType::Cows] = Stats { .input_goods = 5U, .output_goods = 20U, .employees_per_level = 15U, .property_plant_equipment_per_level = 250U, .property_plant_equipment_lifetime = 7U * TICKS_PER_YEAR };
    data.farm_types[FarmType::Wine] = Stats { .input_goods = 8U, .output_goods = 40U, .employees_per_level = 20U, .property_plant_equipment_per_level = 300U, .property_plant_equipment_lifetime = 15U * TICKS_PER_YEAR };
    data.farm_types[FarmType::Wheat] = Stats { .input_goods = 2U, .output_goods = 10U, .employees_per_level = 8U, .property_plant_equipment_per_level = 100U, .property_plant_equipment_lifetime = 15U * TICKS_PER_YEAR };

    for (u32 i = 0U; i < game.players; i++) {
        constexpr Money player_money_start = Money { 10000.0F };
        (void)player_archetype.Add(player_money_start);
    }
    for (u32 i = 0U; i < game.planets; i++) {
        Planet planet = planet_archetype.AddTemplate(start_tick, PlanetTemplate::Playground);
        const Player owner { Entity { pce::Rand() % game.players } };
        planet.TransferOwnerShipToPlayer(owner);
    }
}

static b8 TryQueueFarm(const Planet planet, const FarmType type, Money& money) {
    const Stats stats = data.farm_types[type];
    const Money cost { static_cast<f32>(stats.property_plant_equipment_per_level) }; // NOLINT(*-narrowing-conversions)
    if (money < cost) { return false; }
    const Population employees { static_cast<float>(stats.employees_per_level) };
    if (planet.Unemployed() < employees) { return false; }
    planet.Unemployed() -= employees;
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
        Farm farm = farm_archetype.Add(player, building_under_construction.type);
        farm.Finance().assets.property_plant_equipment = Money { static_cast<f32>(farm.Stats().property_plant_equipment_per_level) };
        farm.Finance().equity = farm.Finance().assets.Total();
        farm.Finance().employees += Population { static_cast<f32>(farm.Stats().employees_per_level) };
    }
}

static QualityOfLife GetQualityOfLife(Planet planet) {
    return QualityOfLife { 0.0F };
    constexpr Money price_of_food = Money { 0.05F };
    constexpr Money price_of_restaurants = Money { 0.10F };
    constexpr Money price_of_cars = Money { 1.00F };
    constexpr Money price_of_watches = Money { 10.0F };
    constexpr Money price_of_ships = Money { 100.0F };
    constexpr Money price_of_dictator = Money { 1000.0F };

    const QualityOfLife quality_of_life = planet.QualityOfLife();
    const QualityOfLifeStage stage = GetQualityOfLifeStage(quality_of_life);
    const f32 inter_level = (quality_of_life - QualityOfLife { static_cast<f32>(stage) } * QUALITY_OF_LIFE_LEVELS_PER_STAGE).Value();

    Money balance = Money { 0.0F }; //Money { planet.PopulationBalance().Value() / planet.Employed().Value() };

    // VITAMINS
    // needs should be like vic 3
    // exponential
    switch (stage) {
        case QualityOfLifeStage::Dying:
            // life needs
            // food, water, heating
            // water is free
            balance -= Money { price_of_food.Value() * inter_level };
            break;
        case QualityOfLifeStage::Surviving: balance -= Money { (price_of_food.Value() * inter_level) + 5.0F };
            break;
        case QualityOfLifeStage::Struggling: balance -= Money { (price_of_food.Value() * inter_level) + 10.0F };
            break;
        case QualityOfLifeStage::Secure:
            // Basic needs
            // basic types of food
            balance -= Money { price_of_restaurants.Value() * inter_level };
            break;
        case QualityOfLifeStage::Comfortable:
            // nice to have
            // beer car
            // services
            balance -= Money { price_of_cars.Value() * inter_level };
            break;
        case QualityOfLifeStage::Lavish:
            // luxury
            // wine, watches luxuries
            // more service
            balance -= Money { price_of_watches.Value() * inter_level };
            break;
        case QualityOfLifeStage::Extravagant:
            // hard luxuries
            // ships spaceships excessive housing
            // servitors
            balance -= Money { price_of_ships.Value() * inter_level };
            break;
        case QualityOfLifeStage::Dictator:
            // richest guy on earth
            // enslaving
            balance -= Money { price_of_dictator.Value() * quality_of_life.Value() };
            break;
    }
    const QualityOfLife quality_of_life_change { static_cast<f32>(balance > Money { 0.0F }) - static_cast<f32>(balance < Money { 0.0F }) };
    return quality_of_life + quality_of_life_change;
}
} // namespace pcg
