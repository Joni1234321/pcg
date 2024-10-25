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

u32 main() {
  Game game;

  u32 i = 0;
  while (true) {
    Logger logger;

    const u32 newLimit = i + 100;
    for (; i < newLimit; i++) {
      logger.Log("Turn {:4}", i);
      game.Tick(i);
    }

    std::vector<f32> vals = playerArchetype.moneys |
                            std::views::transform([](const pcg::Money &money) {
                              return money.Value;
                            }) |
                            std::ranges::to<std::vector>();

    std::vector<f32> *v =
        reinterpret_cast<std::vector<f32> *>(&pcg::playerArchetype.moneys);
    logger.LogVectorStats(*v);
    // pcg::PrintFactory(0);
    // std::cin.ignore();
    //  pcg::PrintStates(logger);
    logger.LogList("Player", "Money",
                   pce::Span<pcg::Money>(pcg::playerArchetype.moneys));
    logger.LogLine();
    logger.Print();
    std::cin.ignore();
  }

  return 0;
}
