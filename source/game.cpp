#include "game.hpp"
#include "algorithm.hpp"
#include "components.hpp"

#include "logger.hpp"
#include "types.hpp"
#include "util.hpp"

using namespace pcg;
using namespace pce;

struct PlayerEntity : Entity {
    constexpr PlayerEntity(Entity e) : Entity(e) { }
    constexpr Money& Money() const { return player_archetype.moneys[index]; }
    constexpr ConstructionQueue& Construction() const { return player_archetype.construction[index]; }
};

struct FarmEntity : Entity {
    constexpr FarmEntity(Entity e) : Entity(e) { }

    constexpr PlayerEntity Player() { return farm_archetype.players[index]; }

    constexpr FARM_TYPE FarmType() { return farm_archetype.type[index]; }
    FarmStats FarmStats() { return Data.farmTypes[FarmType()]; }
};

static bool BuildFarm(PlayerEntity player, FARM_TYPE type);
static void AddIncome(FarmEntity farm) { farm.Player().Money() += static_cast<f32>(farm.FarmStats().out); }
static void Construct(PlayerEntity player);

void _Game::Tick(u32 i) {
    for (const PlayerEntity player : player_archetype) Construct(player);

    const Component<Money> before_revenue = player_archetype.moneys;
    for (const FarmEntity farm : farm_archetype) AddIncome(farm);
    const Component<Money> revenue = select(player_archetype.moneys, before_revenue, minus<Money>());

    const Component<Money> before_expenses = player_archetype.moneys;
    for (const PlayerEntity player : player_archetype) BuildFarm(player, util::RandomKey(Data.farmTypes));
    const Component<Money> expenses = select(player_archetype.moneys, before_expenses, minus<Money>());

    for (Market& market : state_archetype.markets) market.RecalculateMarkets();

    const List<u32> constructing = select(player_archetype.construction, size<ConstructionQueue>());
    Table table("Player", player_archetype.Count);
    table.AddColumn("Revenue", revenue);
    table.AddColumn("Expenses", expenses);
    table.AddColumn("Balance", player_archetype.moneys);
    table.AddColumn("Constructing", constructing);
    table.Print(logger);

    const List<BuildingUnderConstruction> display_construction = player_archetype.construction[0]; //.Limit(10);
    Table cQueue("ConstructionQueue", display_construction.size());
    cQueue.AddColumn("Type", select(display_construction, [](const BuildingUnderConstruction& c) { return c.type; }));
    cQueue.AddColumn("Progress", select(display_construction, [](const BuildingUnderConstruction& c) { return c.progress; }));
    cQueue.AddColumn("Required", select(display_construction, [](const BuildingUnderConstruction& c) { return c.required; }));
    cQueue.Print(logger);
}

void _Game::LogMoney(const std::string& label, const List<Money>& money) {
    for (u32 i = 0; i < money.size(); ++i) {
        logger.SetColor(172 + i * 3);
        logger.Write("Player [{}]\t{} {}\n", i, label, money[i]);
    }
    logger.ClearColor();
}

_Game::_Game(u32 players, u32 markets, u32 factories) {
    Data.farmTypes[FARM_TYPE::CONSTRUCTION] = FarmStats(5, 0);
    Data.farmTypes[FARM_TYPE::WINE] = FarmStats(1000, 250);
    Data.farmTypes[FARM_TYPE::WHEAT] = FarmStats(90, 10);
    Data.farmTypes[FARM_TYPE::FISH] = FarmStats(10, 1);
    Data.farmTypes[FARM_TYPE::COWS] = FarmStats(300, 50);

    for (u32 i = 0; i < players; i++) {
        constexpr u32 player_money_start = 100;
        player_archetype.Add(player_money_start);
        farm_archetype.Add(i, FARM_TYPE::CONSTRUCTION);
    }
}

static bool BuildFarm(PlayerEntity player, FARM_TYPE type) {
    const FarmStats farm = Data.farmTypes[type];
    if (player.Money() < farm.in) return false;
    player.Money() -= farm.in;
    player.Construction().EmplaceBack(type, 30);
    return true;
}

static void Construct(PlayerEntity player) {
    if (player.Construction().Empty()) return;
    BuildingUnderConstruction& building_under_construction = player.Construction().Front();
    building_under_construction.progress += 1;
    if (building_under_construction.progress == building_under_construction.required) {
        player.Construction().Pop();
        farm_archetype.Add(player, building_under_construction.type);
    }
}
