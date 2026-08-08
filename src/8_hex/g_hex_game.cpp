#include <cassert>

#include "g_arcade.hpp"

#include "SDL3/SDL_keycode.h"

import std;

import pce.sdl;
import pce.std;
import pce.math;
import pce.globals;
import pce.window_state;
import pce.collections;
import pce.util;

import pcs.input;
import pcs.camera;
import pcs.render;
import pcs.node;
import pcs.node_data;
import pcs.debug;
import pcs.tick;
import pcs.animation;
import pcs.orchestra;

import hex.hex;
import hex.types;
import hex.terrain;
import hex.scenarios;
import hex.system;

namespace hex {
using namespace hex;
using namespace hex::ui;
namespace {

void HexStateUpdateOOB(HexState& hex_state) {
    hex_state.units_by_formation.clear();
    for (u32 i = 0; i < hex_state.units.size(); i++) {
        const Handle<Unit> unit_handle = hex_state.units.IndexToHandle(i);
        hex_state.units_by_formation[hex_state.units[unit_handle].formation].EmplaceBack(unit_handle);
    }

    constexpr std::array letters = {
        'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'K', 'L', 'M', 'N', 'O', 'P',
    };
    f32 hue_next = 0.0F;

    for (u32 i = 0; i < hex_state.unit_formations.size(); i++) {
        const Handle<UnitFormation> formation_handle = hex_state.unit_formations.IndexToHandle(i);
        UnitFormation& formation = hex_state.unit_formations[formation_handle];

        formation.color = Color::FromHsl(hue_next, 0.5F, 0.5F);
        hue_next = std::fmod(hue_next + 37.0F, 360.0F);

        // sub name is the position inside the formation, "III" or "A"
        const List<Handle<Unit>>& unit_handles = hex_state.units_by_formation[formation_handle];
        for (u32 j = 0; j < unit_handles.size(); j++) {
            Unit& unit = hex_state.units[unit_handles[j]];
            String name_sub;
            switch (unit.echelon) {
                case Echelon::ECHELON_COMPANY: name_sub = std::format("{}", letters[j % letters.size()]); break;
                case Echelon::ECHELON_BATTALION: name_sub = NumberToRomanNumerals(j + 1); break;
                default: break;
            }
            unit.name_div = formation.name;
            UnitNameSet(unit.name_sub, name_sub);
            unit.color = formation.color;
        }
    }
}
} // namespace

void arcade::RunHex() {
    Singleton::Get<WindowState>().clear_color = Color::FromHsl(180.0F, 0.5F, 0.20F);

    HexState& hex_state = Singleton::Get<HexState>();

    // HexScenarioAi(hex_state);
    HexScenarioDivisionClash(hex_state);

    CameraState& camera = Singleton::Get<CameraState>();
    camera.map_world_min = { 0.0F, 0.0F };
    camera.map_world_max = HexAxialToWorld(static_cast<int2>(hex_state.hex_map.map_size - uint2 { 1, 1 }));

    HexStateUpdateOOB(hex_state);

    for (u32 i = 0; i < hex_state.units.size(); i++) {
        Handle<Unit> unit_handle = hex_state.units.IndexToHandle(i);
        hex_state.units_by_axial[hex_state.units[unit_handle].axial].EmplaceBack(unit_handle);
    }
    HexTerrainSetBorder(hex_state);
    hex_state.units_by_axial.clear();

    // Systems
    Orchestra orchestra { };
    orchestra.Add<DebugSystem>();
    orchestra.Add<TickSystem>();

    orchestra.Add<InputSystem>();
    orchestra.Add<InputNodeSystem>();

    orchestra.Add<HexSystem>();

    orchestra.Add<AnimationSystem>();
    orchestra.Add<ParticleSystem>();

    orchestra.Add<CameraSystem>();
    orchestra.Add<RenderNodeSystem>();
    orchestra.Add<RenderWindowSystem>();

    while (!Singleton::Get<InputState>().quit && !Singleton::Get<InputState>().keys_down[SDLK_ESCAPE]) { orchestra.RunSystems(); }

    // Free TTF-owning state BEFORE Window destructor calls TTF_Quit/SDL_Quit,
    // otherwise Labels (TTF_Text) in CounterState are destroyed during static destruction
    // after SDL is dead, causing an access violation (0xC0000005).
    hex_state.counters.Destroy();
    hex_state.label_pool.Destroy();
    globalData.Get<ParticleEmitter>().clear();
    globalData.Get<NodeTree>().clear();
}
} // namespace hex
