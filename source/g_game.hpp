#pragma once

#include "g_components.hpp"
#include "engine/r_engine.hpp"
#include "r_ui.hpp"
#include "u_logger.hpp"
#include "engine/u_types.hpp"

namespace pcg {
struct NewGameSettings {
    u32 players;
    u32 planets;
};
struct Game {
    pce::Logger logger;
    explicit Game(NewGameSettings);
    void PlayTick(pce::Tick tick, pce::ui::NodeRenderSystem& node_render_system, const b8 debug);
};

constexpr NewGameSettings GAME_SETTINGS_CHALLENGE = { .players = 2U, .planets = 4U };
constexpr NewGameSettings GAME_SETTINGS_UTOPIA = { .players = 1U, .planets = 1U };

inline Data data;                       // NOLINT(*-err58-cpp, *-avoid-non-const-global-variables)
inline Game game(GAME_SETTINGS_UTOPIA); // NOLINT(*-err58-cpp, *-avoid-non-const-global-variables)

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
} // namespace pcg
