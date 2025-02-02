#pragma once

#include "r_engine.hpp"
#include "r_ui.hpp"
#include "u_types.hpp"

namespace pcg::frame {
struct MainMenuFrame {
    pce::ui::NodeTree tree;
    explicit MainMenuFrame(pce::ui::UISystem& ui_system);
    void Tick(u32 i, pce::ui::UISystem& ui_system);
};

struct GameFrame {
    pce::ui::NodeTree tree;
    explicit GameFrame(pce::ui::UISystem& ui_system);
    [[nodiscard]] constexpr pce::ui::Node& GameArea() { return tree.GetNode(game_area.GetHandle()); }
    [[nodiscard]] constexpr pce::ui::Node& Box() { return tree.GetNode(box.GetHandle()); }
    void SetTime(const u32 time_ms) { tree.GetNode(time_label.GetHandle()).text = std::format("Time {:02}:{:02}.{:02}", time_ms / (1000U * 60U), time_ms / 1000U % 60U, time_ms % 100U ); }
    void SetScore(const u32 score) { tree.GetNode(score_label.GetHandle()).text = std::format("Score {:4}", score); }

private:
    pce::ui::Node::OptionalHandle time_label { };
    pce::ui::Node::OptionalHandle score_label { };
    pce::ui::Node::OptionalHandle box { };
    pce::ui::Node::OptionalHandle game_area { };
};

class TickFrame {
    pce::ui::Node::OptionalHandle tick_handle { };

public:
    pce::ui::NodeTree tree;
    explicit TickFrame(pce::ui::UISystem& ui_system) : tree { ui_system.text_engine, ui_system.font } {
        tick_handle = pce::ui::NodeBuilder(pce::ui::hug).Text("Tick", pce::ui::Fonts::tiny).Fill(pce::colors::radiant_orange).BuildRoot(tree, { 10U, 0U });
    }
    void SetTick(u32 tick) { tree.GetNode(tick_handle.GetHandle()).text = std::format("Tick {:6}", tick); }
};
struct TestFrame {
    std::vector<pce::ui::RectangleElement::Handle> elements { };
    pce::ui::NodeTree tree;
    explicit TestFrame(pce::ui::UISystem& ui_system);
    void Tick(u32 tick, pce::ui::UISystem& ui_system);
};
struct DebugFrame {
    pce::ui::NodeTree tree;
    explicit DebugFrame(pce::ui::UISystem& ui_system);
    void ShowElementStructure(const pce::ui::UISystem::HoveredType& hovered);
};
class OverviewFrame {
    pce::ui::TableElement::Handle table_handle;

public:
    explicit OverviewFrame(pce::ui::UISystem& ui_system);
    void Tick(pce::ui::UISystem& ui_system);
};
}
