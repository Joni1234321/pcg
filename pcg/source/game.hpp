#pragma once

#include <cstdlib>
#include <unordered_map>

#include "components.hpp"
#include "logger.hpp"
#include "types.hpp"

namespace pcg {

struct FarmStats {
	u32 in, out;
};

struct _Data {
	std::unordered_map<FarmType, FarmStats> farmTypes;
};
struct _Game {
	pce::Logger logger;
	_Game(u32 players = 2, u32 markets = 10, u32 factories = 100);
	void Tick(u32 i);

private:
	void LogMoney(const std::string& label, const pce::List<Money>& money);
};

inline _Data Data;
inline _Game Game;

// map power
// isolated small island homogen culture
// ancient populus culture
// technological wonderland
// island rich country
//
// roles
// engineers
// soldiers
// workers
// farmers
// scientists
// bankers
// health
// owners

// cultures
//

// terrain
// blocking (mountain) expensive transportation
// movable (river) cheaper transportation
// map gen

// Resources
// Grown: wheat, fish, cows
// Natural: wood, oil
// Rocks: Fe, Au, Ag, Al
// Deposit mechanic
// Scouting, Purity, Levels

// phases land -> build -> profit
// 2 land -> build -> buy / sell goods for maybe profit

// factories
// Sells goods at profit

// Services
// Uses goods to sell them more expensive

// Resource list
}  // namespace pcg