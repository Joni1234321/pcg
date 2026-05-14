#include "g_arcade.hpp"

#include "0_engine/u_collections.hpp"
#include "0_engine/u_colors.hpp"
#include "0_engine/u_texture.hpp"
#include "0_engine/u_types.hpp"

#include "1_systems/i_input_system.hpp"
#include "1_systems/r_render.hpp"
#include "1_systems/r_ui_node.hpp"
#include "1_systems/t_debug_system.hpp"
#include "1_systems/t_tick_system.hpp"
#include "1_systems/u_animation_system.hpp"
#include "1_systems/u_orchestra.hpp"


namespace pcg::commandstrike {
using namespace pce;
using namespace pce::ui;

// data
struct GameDefines {

};
struct GameState {

};

// systems
struct CommandStrikeSystem {
    void operator()() const {

    }
};

struct CommandStrikeUISystem {
    void operator()() const {

    }
};


struct GameFrame : Frame {
    Handle<Node> root { B(frame).Node(fill).Gap(10U).Padding(10U).Build() };

    Handle<Node> actions { B(root).Node(349U, fill).Build() };
    Handle<Node> resources { B(actions).Node(fill, hug).Fill(colors::soft_blue).Build() };
    Handle<Node> build { B(actions).Node(fill).Fill(colors::soft_blue).Build() };
    Handle<Node> upgrades { B(actions).Node(349U, fill).Fill(colors::soft_blue).Build() };

    Handle<Node> map { B(root).Node(fill).Build() };
    Handle<Node> sky { B(map).Node(fill).Fill(colors::sky_cyan).Build() };
    Handle<Node> ground { B(map).Node(fill, 600U).Fill(colors::forest_green).Build() };


};
} // namespace pcg::commandstrike
void pcg::arcade::RunCommandStrike() {
    pce::Logger().Log("Running command strike");
    using namespace pce;
    using namespace commandstrike;

    // data
    singleton.Get<WindowState>().clear_color = colors::dark_grey;

    // systems
    Orchestra orchestra { };

    orchestra.Add<DebugSystem>();

    orchestra.Add<TickSystem>();
    orchestra.Add<InputSystem>();
    orchestra.Add<NodeInputSystem>();

    orchestra.Add<CommandStrikeSystem>();
    orchestra.Add<CommandStrikeUISystem>();

    orchestra.Add<AnimationSystem>();
    orchestra.Add<NodeRenderSystem>();
    orchestra.Add<ParticleSystem>();
    orchestra.Add<PresentSystem>();

    while (!singleton.Get<InputState>().quit && !singleton.Get<InputState>().keys_down[SDLK_ESCAPE]) { orchestra.RunSystems(); }
}