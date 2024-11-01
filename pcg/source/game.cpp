#include "components.hpp"
#include "algorithm.hpp"
#include "game.hpp"

#include "logger.hpp"
#include "types.hpp"
#include "util.hpp"
#include "formatting.hpp"

using namespace pcg;
using namespace pce;

struct PlayerEntity : Entity {
	constexpr PlayerEntity(Entity e) : Entity(e) {}
	constexpr Money& Money() { return playerArchetype.moneys[index]; }
	constexpr ConstructionQueue& Construction() { return playerArchetype.construction[index]; }
};

struct FarmEntity : Entity {
	constexpr FarmEntity(Entity e) : Entity(e) {}

	constexpr PlayerEntity Player() { return farmArchetype.players[index]; }

	constexpr FarmType FarmType() { return farmArchetype.type[index]; }
	const FarmStats FarmStats() { return Data.farmTypes[FarmType()]; }
};

static bool BuildFarm(PlayerEntity player, FarmType type);
static void AddIncome(FarmEntity farm) { farm.Player().Money() += farm.FarmStats().out; }
static void Construct(PlayerEntity player);

void _Game::Tick(u32 i) {
	for (PlayerEntity player : playerArchetype) Construct(player);

	Component<Money> beforeRevenue = playerArchetype.moneys;
	for (FarmEntity farm : farmArchetype) AddIncome(farm);
	Component<Money> revenue = select(playerArchetype.moneys, beforeRevenue, util::minus<Money>());

	Component<Money> beforeExpenses = playerArchetype.moneys;
	for (PlayerEntity player : playerArchetype) BuildFarm(player, util::RandomKey(Data.farmTypes));
	Component<Money> expenses = select(playerArchetype.moneys, beforeExpenses, util::minus<Money>());

	for (Market& market : stateArchetype.markets) market.RecalculateMarkets();
	
	List<u32> constructing = select(playerArchetype.construction, util::size<ConstructionQueue>());

	Table table("Player", playerArchetype.n);
	table.AddColumn("Revenue", revenue);
	table.AddColumn("Expenses", expenses);
	table.AddColumn("Balance", playerArchetype.moneys);
	table.AddColumn("Constructing", constructing);
	table.Print(logger);

	const u32 N = 5;
	Table cqueue("ConstructionQueue", N);
	// FarmType
	List<u32> types = select(playerArchetype.construction[0], [](BuildingUnderConstruction& c) { return c.progress; });
	cqueue.AddColumn("Type", types);
}

void _Game::LogMoney(const std::string& label, const List<Money>& money) {
	for (u32 i = 0; i < money.size(); ++i) {
		logger.SetColor(172 + i * 3);
		logger.Write("Player [{}]\t{} {}\n", i, label, money[i]);
	}
	logger.ClearColor();
}

_Game::_Game(u32 players, u32 markets, u32 factories) : logger() {
	Data.farmTypes[FarmType::Construction] = FarmStats(5, 0);
	Data.farmTypes[FarmType::Wine] = FarmStats(1000, 250);
	Data.farmTypes[FarmType::Wheat] = FarmStats(90, 10);
	Data.farmTypes[FarmType::Fish] = FarmStats(10, 1);
	Data.farmTypes[FarmType::Cows] = FarmStats(300, 50);

	const u32 PLAYER_MONEY_START = 100;

	for (u32 i = 0; i < players; i++) {
		playerArchetype.Add(PLAYER_MONEY_START);
		farmArchetype.Add(i, FarmType::Construction);
	}
}

static bool BuildFarm(PlayerEntity player, FarmType type) {
	FarmStats farm = Data.farmTypes[type];
	if (player.Money() < farm.in) return false;
	player.Money() -= farm.in;
	player.Construction().emplace_back(type, 30);
	return true;
}

static void Construct(PlayerEntity player) {
	if (player.Construction().empty()) return;
	BuildingUnderConstruction& buildingUnderConstruction = player.Construction().front();
	buildingUnderConstruction.progress += 1;
	if (buildingUnderConstruction.progress == buildingUnderConstruction.required) {
		player.Construction().pop(); 
		farmArchetype.Add(player, buildingUnderConstruction.type);
	}
}