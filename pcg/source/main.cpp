#include "collections.hpp"
#include "components.hpp"
#include "game.hpp"
#include "logger.hpp"
#include "prints.hpp"
#include <iostream>

i32 main() {
	pcg::game::Setup();

	i32 i = 0;
	while (true) {
		pce::Logger logger;

		const i32 newLimit = i + 100;
		for (; i < newLimit; i++) {
			logger.Log("Turn {:4}", i);
			pcg::game::Tick(i);
			logger.LogListOneLne("Player", "Money", pce::Span<f32>(pcg::player::moneys));
		}

		logger.LogVectorStats(pcg::factory::moneys);
		//pcg::PrintFactory(0);
		//std::cin.ignore();
		pcg::PrintStates(logger);
		logger.LogList("Player", "Money", pce::Span<f32>(pcg::player::moneys));
		logger.LogLine();
		logger.Print();
		std::cin.ignore();
	}


	return 0;
}
