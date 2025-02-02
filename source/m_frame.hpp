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
    void Tick(u32 score, u32 time);
private:
    pce::ui::Node::OptionalHandle time_label { };
    pce::ui::Node::OptionalHandle score_label { };
};

class TickFrame {
    pce::ui::Node::OptionalHandle tick_handle { };

public:
    pce::ui::NodeTree tree;
    explicit TickFrame(pce::ui::UISystem& ui_system) : tree { ui_system.text_engine, ui_system.font } {
        tick_handle = pce::ui::NodeBuilder(pce::ui::hug).Text("Tick", pce::ui::Fonts::tiny).Fill(pce::colors::radiant_orange).BuildRoot(tree, { 10U, 0U });
    }
    void Tick(u32 i) {
        tree.GetNode(tick_handle.GetHandle()).text = std::format("Tick {:6}", i);
        tree.MarkDirty();
    }
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
    void Tick(u32 tick, pce::ui::UISystem& ui_system);
};
class OverviewFrame {
    pce::ui::TableElement::Handle table_handle;

public:
    explicit OverviewFrame(pce::ui::UISystem& ui_system);
    void Tick(pce::ui::UISystem& ui_system);
};
}
