#pragma once

#include <cstdlib>
#include <vector>

#include "components.hpp"
#include "types.hpp"
#include "util.hpp"

namespace pcg {
bool Player::Add(u32 money) {
  if (!Archetype::Add()) return false;

  moneys.emplace_back(money);

  return true;
}
bool Player::Remove(const Entity entity) {
  if (!Archetype::Remove(entity)) return false;

  pce::util::SwapPop(moneys, entity.index);

  return true;
}

bool State::Add(const Entity player) {
  if (!Archetype::Add()) return false;

  const u32 POP = 1000;
  players.emplace_back(player);
  markets.push_back(Market{rand() % POP});

  return true;
}
bool State::Remove(const Entity entity) {
  if (!Archetype::Remove(entity)) return false;

  pce::util::SwapPop(players, entity.index);
  pce::util::SwapPop(markets, entity.index);

  return true;
}

bool Farm::Add(const Entity player, const FarmTypes farmType) {
  if (!Archetype::Add()) return false;

  const u32 POP = 1000;
  players.emplace_back(player);
  buildings.push_back(farmType);

  return true;
}
bool Farm::Remove(const Entity entity) {
  if (!Archetype::Remove(entity)) return false;

  pce::util::SwapPop(players, entity.index);
  pce::util::SwapPop(markets, entity.index);

  return true;
}
}  // namespace pcg
