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
		construction.emplace_back();

		return true;
	}
	bool Player::Remove(Entity entity) {
		if (!Archetype::Remove(entity)) return false;

		pce::util::SwapPop(moneys, entity.index);

		return true;
	}

	bool State::Add(Entity player) {
		if (!Archetype::Add()) return false;

		const u32 POP = 1000;
		players.emplace_back(player);
		markets.push_back(Market{ rand() % POP });

		return true;
	}
	bool State::Remove(Entity entity) {
		if (!Archetype::Remove(entity)) return false;

		pce::util::SwapPop(players, entity.index);
		pce::util::SwapPop(markets, entity.index);

		return true;
	}

	bool Farm::Add(Entity player, FarmType farmType) {
		if (!Archetype::Add()) return false;

		players.emplace_back(player);
		type.emplace_back(farmType);

		return true;
	}
	bool Farm::Remove(Entity entity) {
		if (!Archetype::Remove(entity)) return false;

		pce::util::SwapPop(players, entity.index);
		pce::util::SwapPop(type, entity.index);

		return true;
	}
}  // namespace pcg
