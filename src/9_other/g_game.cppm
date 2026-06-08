module;

export module pcg.g_game;
import pcs.tick;
import pce.logger;
import pce.std;

export namespace hex::ui {
struct RenderNodeSystem;
}

export namespace hex {
struct Data;

struct NewGameSettings {
    u32 players;
    u32 planets;
};
struct Game {
    hex::Logger logger;
    explicit Game(NewGameSettings);
    void PlayTick(hex::Tick tick, hex::ui::RenderNodeSystem& node_render_system, b8 debug);
};

constexpr NewGameSettings GAME_SETTINGS_CHALLENGE = { .players = 2U, .planets = 4U };
constexpr NewGameSettings GAME_SETTINGS_UTOPIA = { .players = 1U, .planets = 1U };

extern Data data; // NOLINT(*-avoid-non-const-global-variables)
extern Game game; // NOLINT(*-avoid-non-const-global-variables)

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

