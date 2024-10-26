#include "components.hpp"
#include "game.hpp"

#include "logger.hpp"
#include "types.hpp"
#include "util.hpp"

using namespace pcg;
using namespace pce;

struct PlayerEntity : Entity {
	PlayerEntity(Entity e) : Entity(e) {}
	Money &money() { return playerArchetype.moneys[index]; }
};

static bool BuildFarm(const Entity player, const FarmType type, Money &money);

void Game::Tick(u32 i) const {
	Logger logger;
	for (const PlayerEntity player : playerArchetype) {
		const FarmType &randomBuilding = util::RandomKey(Data.farmTypes);
		bool b = BuildFarm(player, randomBuilding, playerArchetype.moneys[player.index]);
	}
	// Earn profits
	for (Market &market : stateArchetype.markets) market.RecalculateMarkets();
}


Game::Game(u32 players, u32 markets, u32 factories) {
	pce::Logger logger;

	Data.farmTypes[FarmType::Wine] = FarmStats(1000, 250);
	Data.farmTypes[FarmType::Wheat] = FarmStats(90, 10);
	Data.farmTypes[FarmType::Fish] = FarmStats(10, 1);
	Data.farmTypes[FarmType::Cows] = FarmStats(300, 50);

	const u32 PLAYER_MONEY_START = 10000;

	for (u32 i = 0; i < players; i++) playerArchetype.Add(PLAYER_MONEY_START);
	for (u32 i = 0; i < markets; i++) stateArchetype.Add(100);
}

static bool BuildFarm(const Entity player, const FarmType type, Money &money) {
	FarmStats farm = Data.farmTypes[type];
	if (money < farm.in) return false;
	money -= farm.in;
	farmArchetype.Add(player, type);
	return true;
}
