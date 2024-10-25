#include "game.hpp"
#include "components.hpp"

#include "util.hpp"

using namespace pcg;
using namespace pce;

struct PlayerEntity : Entity {
  PlayerEntity(Entity e) : Entity(e) { }
  Money &money() { return playerArchetype.moneys[index]; }
};

bool BuildFarm(const Entity player, const FarmTemplate farm, Money &money);

void Game::Tick(u32 i) {
  Logger logger;
  for (const PlayerEntity player : playerArchetype) {
    const FarmTemplate &randomBuilding = util::Random(settings.farmTypes);
    bool b= BuildFarm(player, randomBuilding, playerArchetype.moneys[player.index]);
  }
  // Earn profits
  for (Market &market : stateArchetype.markets) market.RecalculateMarkets();
}


Game::Game(u32 players , u32 markets, u32 factories) {
  pce::Logger logger;
  
  settings.farmTypes[FarmTypes::Wine] = FarmTemplate(1000, 250);
  settings.farmTypes[FarmTypes::Wheat] = FarmTemplate(90, 10);
  settings.farmTypes[FarmTypes::Fish] = FarmTemplate(10, 1);
  settings.farmTypes[FarmTypes::Cows] = FarmTemplate(300, 50);

  const u32 PLAYER_MONEY_START = 10000;

  for (u32 i = 0; i < players; i++) playerArchetype.Add(PLAYER_MONEY_START);
  for (u32 i = 0; i < markets; i++) stateArchetype.Add(100);
}

bool BuildFarm(const Entity player, const FarmTemplate farm, Money &money) {
  if (money < farm.in) return false;
  money -= farm.in;
  farmArchetype.Add();
  return true;
}
