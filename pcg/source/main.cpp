#include <iostream>
#include <ranges>
#include <vector>

#include "collections.hpp"
#include "components.hpp"
#include "game.hpp"
#include "logger.hpp"
#include "types.hpp"

using namespace pcg;
using namespace pce;

template<typename TO, typename FROM>
constexpr TO& Reinterpret(FROM& from) { return *reinterpret_cast<TO*>(&from); }

u32 main() {
	u32 i = 0;
	while (true) {
		for (u32 newLimit = i + 10; i < newLimit; i++) {
			Game.logger.LogLine();
			Game.logger.Log("Turn {:4}", i);
			Game.Tick(i);
		}

		Game.logger.LogVectorStats(Reinterpret<List<f32>>(playerArchetype.moneys));
		Game.logger.Print();
		std::cin.ignore();
	}

	return 0;
}
