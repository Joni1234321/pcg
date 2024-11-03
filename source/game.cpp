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
    for (const PlayerEntity player : player_archetype) { (void)BuildFarm(player, RandomKey(data.farm_types)); }
    const Component<Money> expenses = Select(player_archetype.moneys, before_expenses, Minus<Money>());

    for (Market& market : state_archetype.markets) { market.RecalculateMarkets(); }

    const List<u32> constructing = Select(player_archetype.construction, Size<ConstructionQueue>());
    Table table("Player", player_archetype.Count);
    table.AddColumn("Revenue", revenue);
    table.AddColumn("Expenses", expenses);
    table.AddColumn("Balance", player_archetype.moneys);
    table.AddColumn("Constructing", constructing);
    table.Print(logger, Table::COLOR_ENABLED);

    const List<BuildingUnderConstruction> display_construction = player_archetype.construction[0U]; //.Limit(10);
    Table construction_table("ConstructionQueue", display_construction.Size());
    construction_table.AddColumn("Type", Select(display_construction, [] (const BuildingUnderConstruction& building) -> FARM_TYPE { return building.type; }));
    construction_table.AddColumn("Progress", Select(display_construction, [] (const BuildingUnderConstruction& building) -> u16 { return building.progress; }));
    construction_table.AddColumn("Required", Select(display_construction, [] (const BuildingUnderConstruction& building) -> u16 { return 30U; }));
    construction_table.Print(logger, Table::COLOR_DISABLED);
}

Game::Game(const u32 players) {
    data.farm_types[FARM_TYPE::CONSTRUCTION] = FarmStats { .cost = 5U, .production = 0U };
    data.farm_types[FARM_TYPE::WINE] = FarmStats { .cost = 1000U, .production = 250U };
    data.farm_types[FARM_TYPE::WHEAT] = FarmStats { .cost = 90U, .production = 10U };
    data.farm_types[FARM_TYPE::FISH] = FarmStats { .cost = 10U, .production = 1U };
    data.farm_types[FARM_TYPE::COWS] = FarmStats { .cost = 300U, .production = 50U };

    for (u32 i = 0U; i < players; i++) {
        constexpr Money player_money_start = 100.0F;
        (void)player_archetype.Add(player_money_start);
        (void)farm_archetype.Add(i, FARM_TYPE::CONSTRUCTION);
    }
}

static bool BuildFarm(const PlayerEntity player, FARM_TYPE type) {
    constexpr u32 construction_time = 30U;
    const FarmStats farm = data.farm_types[type];
    const Money cost(static_cast<f32>(farm.cost)); // NOLINT(*-narrowing-conversions)
    if (player.Money() < cost) { return false; }

    player.Money() -= cost;
    (void)player.Construction().EmplaceBack(type);
    return true;
}

static void Construct(const PlayerEntity player) {
    if (player.Construction().Empty()) { return; }
    BuildingUnderConstruction& building = player.Construction().Front();
    building.progress += 1;
    constexpr u16 required = 30U;
    if (building.progress == required) {
        player.Construction().Pop();
        (void)farm_archetype.Add(player, building.type);
    }
}
} // namespace pcg
