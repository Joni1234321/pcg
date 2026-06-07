module;

export module pcg.g_commandstrike;

import pcg.g_arcade;

import pce.collections;
import pce.systems.i_input_system;
import pce.systems.r_render;
import pce.systems.r_ui_node;
import pce.systems.t_debug_system;
import pce.systems.t_tick_system;
import pce.systems.u_animation_system;
import pce.systems.u_orchestra;

import pce.colors;
import pce.std;

namespace pcg::commandstrike {
using namespace pce;
using namespace pce::ui;

// data
struct GameDefines { };
struct GameState { };

// systems
struct CommandStrikeSystem {
    void operator()() const { }
};

struct CommandStrikeUISystem {
    void operator()() const { }
};

struct GameFrame : Frame {
    Handle<Node> root { B(frame).Node(fill).Gap(10U).Padding(10U).Build() };

    Handle<Node> actions { B(root).Node(349U, fill).Build() };
    Handle<Node> resources { B(actions).Node(fill, hug).Fill(colors::COLOR_SOFT_BLUE).Build() };
    Handle<Node> build { B(actions).Node(fill).Fill(colors::COLOR_SOFT_BLUE).Build() };
    Handle<Node> upgrades { B(actions).Node(349U, fill).Fill(colors::COLOR_SOFT_BLUE).Build() };

    Handle<Node> map { B(root).Node(fill).Build() };
    Handle<Node> sky { B(map).Node(fill).Fill(colors::COLOR_SKY_CYAN).Build() };
    Handle<Node> ground { B(map).Node(fill, 600U).Fill(colors::COLOR_FOREST_GREEN).Build() };
};
} // namespace pcg::commandstrike

void pcg::arcade::RunCommandStrike() {
    using namespace pce;
    using namespace commandstrike;

    // data
    Singleton::Get<WindowState>().clear_color = colors::COLOR_FADED_GREEN;

    // systems
    Orchestra orchestra { };

    orchestra.Add<DebugSystem>();

    orchestra.Add<TickSystem>();
    orchestra.Add<InputSystem>();
    orchestra.Add<InputNodeSystem>();

    orchestra.Add<CommandStrikeSystem>();
    orchestra.Add<CommandStrikeUISystem>();

    orchestra.Add<AnimationSystem>();
    orchestra.Add<RenderNodeSystem>();
    orchestra.Add<ParticleSystem>();
    orchestra.Add<RenderWindowSystem>();

    while (!Singleton::Get<InputState>().quit && !Singleton::Get<InputState>().keys_down[SDLK_ESCAPE]) { orchestra.RunSystems(); }
}
