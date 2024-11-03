// ReSharper disable CppNonExplicitConvertingConstructor
#include "game.hpp"
#include "algorithm.hpp"
#include "components.hpp"

#include "logger.hpp"
#include "types.hpp"
#include "util.hpp"

namespace pcg {

using namespace pce; // NOLINT(*-build-using-namespace)

struct PlayerEntity : Entity {
    constexpr PlayerEntity(const Entity entity) : Entity(entity) { } // NOLINT(*-explicit-constructor, *-explicit-conversions)
    [[nodiscard]] constexpr Money& Money() const { return player_archetype.moneys[index]; }
    [[nodiscard]] constexpr ConstructionQueue& Construction() const { return player_archetype.construction[index]; }
};

struct FarmEntity : Entity {
    constexpr FarmEntity(const Entity entity) : Entity(entity) { } // NOLINT(*-explicit-constructor, *-explicit-conversions)

    [[nodiscard]] constexpr PlayerEntity Player() const { return farm_archetype.players[index]; }

    [[nodiscard]] constexpr FARM_TYPE FarmType() const { return farm_archetype.type[index]; }
    [[nodiscard]] FarmStats FarmStats() const { return data.farm_types[FarmType()]; }
};

static bool BuildFarm(PlayerEntity player, FARM_TYPE type);
static void AddIncome(const FarmEntity farm) { farm.Player().Money() += static_cast<f32>(farm.FarmStats().production); }
static void Construct(PlayerEntity player);

void Game::Tick(u32 tick) {
    for (const PlayerEntity player : player_archetype) { Construct(player); }

    const Component<Money> before_revenue = player_archetype.moneys;
    for (const FarmEntity farm : farm_archetype) { AddIncome(farm); }
    const Component<Money> revenue = Select(player_archetype.moneys, before_revenue, Minus<Money>());

    const Component<Money> before_expenses = player_archetype.moneys;
    for (const PlayerEntity player : player_archetype) { BuildFarm(player, util::RandomKey(data.farm_types)); }
    const Component<Money> expenses = Select(player_archetype.moneys, before_expenses, Minus<Money>());

    for (Market& market : state_archetype.markets) { market.RecalculateMarkets(); }

    const List<u32> constructing = Select(player_archetype.construction, Size<ConstructionQueue>());
    Table table("Player", player_archetype.Count);
    table.AddColumn("Revenue", revenue);
    table.AddColumn("Expenses", expenses);
    table.AddColumn("Balance", player_archetype.moneys);
    table.AddColumn("Constructing", constructing);
    table.Print(logger, Table::COLOR_ENABLED);

    const List<BuildingUnderConstruction> display_construction = player_archetype.construction[0]; //.Limit(10);
    Table cQueue("ConstructionQueue", display_construction.size());
    cQueue.AddColumn("Type", Select(display_construction, [] (const BuildingUnderConstruction& building) -> FARM_TYPE { return building.type; }));
    cQueue.AddColumn("Progress", Select(display_construction, [] (const BuildingUnderConstruction& building) -> u16 { return building.progress; }));
    cQueue.AddColumn("Required", Select(display_construction, [] (const BuildingUnderConstruction& building) -> u16 { return building.required; }));
    cQueue.Print(logger, Table::COLOR_DISABLED);
}

Game::Game(const u32 players) {
    data.farm_types[FARM_TYPE::CONSTRUCTION] = FarmStats(5U, 0U);
    data.farm_types[FARM_TYPE::WINE] = FarmStats(1000U, 250U);
    data.farm_types[FARM_TYPE::WHEAT] = FarmStats(90, 10U);
    data.farm_types[FARM_TYPE::FISH] = FarmStats(10, 1);
    data.farm_types[FARM_TYPE::COWS] = FarmStats(300, 50);

    for (u32 i = 0; i < players; i++) {
        constexpr Money player_money_start = 100.0F;
        (void)player_archetype.Add(player_money_start);
        (void)farm_archetype.Add(i, FARM_TYPE::CONSTRUCTION);
    }
}

static bool BuildFarm(const PlayerEntity player, FARM_TYPE type) {
    constexpr u32 construction_time = 30U;
    const FarmStats farm = data.farm_types[type];
    const Money cost = static_cast<Money>(farm.cost); // NOLINT(*-narrowing-conversions)
    if (player.Money() < cost) { return false; }

    player.Money() -= cost;
    player.Construction().EmplaceBack(type, construction_time);
    return true;
}

static void Construct(const PlayerEntity player) {
    if (player.Construction().Empty()) { return; }

    BuildingUnderConstruction& building_under_construction = player.Construction().Front();
    building_under_construction.progress += 1;
    if (building_under_construction.progress == building_under_construction.required) {
        player.Construction().Pop();
        farm_archetype.Add(player, building_under_construction.type);
    }
}
} // namespace pcg
