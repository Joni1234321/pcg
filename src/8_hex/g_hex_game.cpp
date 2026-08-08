#include <cassert>

#include "g_arcade.hpp"

#include "SDL3/SDL_keycode.h"
#include "SDL3_ttf/SDL_ttf.h"

import std;

import pce.colors;
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

HandleOptional<Unit> UnitGetParentWithEchelon(HexState& hex_state, Handle<Unit> unit_handle, Echelon echelon) {
    while (hex_state.units[unit_handle].parent.IsValid()) {
        Handle<Unit> unit_handle_parent = hex_state.units[unit_handle].parent.GetHandle();
        Unit& unit_parent = hex_state.units[unit_handle_parent];
        if (unit_parent.echelon >= echelon) { return hex_state.units[unit_handle].parent; }
        unit_handle = unit_handle_parent;
    }
    return std::nullopt;
}

void HexStateUpdateOOB(HexState& hex_state) {
    f32 hue_next = 0.0F;
    u32 number_army = 1;
    u32 number_corps = 15;
    u32 number_div = 100;

    // generate oob.
    hex_state.units_oob.clear();
    for (u32 i = 0; i < hex_state.units.size(); i++) {
        const Handle<Unit> unit_handle = hex_state.units.IndexToHandle(i);
        const Unit& unit = hex_state.units[unit_handle];
        hex_state.units_oob[unit.parent.GetHandle()].EmplaceBack(unit_handle);
    }

    // create colors and names
    auto get_unit_color = [&](const Unit& unit) -> Color {
        if (unit.icon != UnitIcon::ICON_HQ && unit.parent.IsValid()) { return hex_state.units[unit.parent.GetHandle()].color; }
        const Color color = Color::FromHsl(hue_next, 0.5F, 0.5F);
        hue_next = std::fmod(hue_next + 37.0F, 360.0F);
        return color;
    };

    const std::array letters = {
        'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'K', 'L', 'M', 'N', 'O', 'P',
    };
    // parent name is the designation of the formation itself, "333rd"
    auto get_unit_parent_name = [&](const Unit& unit) -> String {
        switch (unit.echelon) {
            case Echelon::ECHELON_COMPANY:
            case Echelon::ECHELON_BATTALION: {
                return !unit.parent.IsValid() ? "dtc" : UnitNameToString(hex_state.units[unit.parent.GetHandle()].name_div);
            }
            case Echelon::ECHELON_REGIMENT: {
                u32 regiment_number = 0;
                switch (unit.tag) {
                    case CountryTag::TAG_GER: regiment_number = Rand(200U, 500U); break;
                    case CountryTag::TAG_SOV: regiment_number = Rand(600U, 900U); break;
                    default: regiment_number = Rand(200U, 999U); break;
                }
                return std::format("{}{}", regiment_number, NumberToOrdinal(regiment_number));
            }
            case Echelon::ECHELON_DIVISION: ++number_div; return std::format("{}{}", number_div, NumberToOrdinal(number_div));
            case Echelon::ECHELON_CORPS: ++number_corps; return std::format("{}{}", number_corps, NumberToOrdinal(number_corps));
            case Echelon::ECHELON_ARMY: ++number_army; return std::format("{}{}", number_army, NumberToOrdinal(number_army));
            default: return "fail";
        }
    };

    // sub name is the position inside the parent formation, "III" or "A"
    auto get_unit_sub_name = [&](const Unit& unit, u32 i) -> String {
        switch (unit.echelon) {
            case Echelon::ECHELON_COMPANY: return std::format("{}", letters[i % letters.size()]);
            case Echelon::ECHELON_BATTALION: return NumberToRomanNumerals(i + 1);
            default: return "";
        }
    };

    Queue<Handle<Unit>> queue;
    queue.EmplaceBack(HandleOptional<Unit> { std::nullopt }.GetHandle());

    while (!queue.empty()) {
        List<Handle<Unit>> unit_handles_children = hex_state.units_oob[queue.Front()];
        queue.Pop();
        for (u32 i = 0; i < unit_handles_children.size(); i++) {
            Handle<Unit> unit_handle_child = unit_handles_children[i];
            queue.EmplaceBack(unit_handle_child);
            Unit& unit_child = hex_state.units[unit_handle_child];
            UnitNameSet(unit_child.name_div, get_unit_parent_name(unit_child));
            UnitNameSet(unit_child.name_sub, get_unit_sub_name(unit_child, i));
            unit_child.color = get_unit_color(unit_child);
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
    globalData.Get<NodeTree>().clear();
}
} // namespace hex
