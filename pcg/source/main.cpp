#include "collections.hpp"
#include "components.hpp"
#include "game.hpp"
#include "logger.hpp"
#include "types.hpp"
#include <iostream>
#include <ranges>
#include <vector>

u32 main() {
	pcg::game::Setup();

	u32 i = 0;
	while (true) {
		pce::Logger logger;

		const u32 newLimit = i + 100;
		for (; i < newLimit; i++) {
			logger.Log("Turn {:4}", i);
			pcg::game::Tick(i);
			//logger.LogListOneLne("Player", "Money", pce::Span<pcg::Money>(pcg::player::moneys));
		}


		std::vector<f32> vals = pcg::player::moneys
			| std::views::transform([](const pcg::Money &money) { return money.Value; })
			| std::ranges::to<std::vector>();

		std::vector<f32> *v = reinterpret_cast<std::vector<f32>*>(&pcg::player::moneys);
		logger.LogVectorStats(*v);
		//pcg::PrintFactory(0);
		//std::cin.ignore();
		// pcg::PrintStates(logger);
		logger.LogList("Player", "Money", pce::Span<pcg::Money>(pcg::player::moneys));
		logger.LogLine();
		logger.Print();
		std::cin.ignore();
	}

	return 0;
}
