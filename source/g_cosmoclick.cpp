#include "g_arcade.hpp"

#include "m_frame.hpp"
#include "r_engine.hpp"
#include "r_ui.hpp"
#include "r_ui_node.hpp"
#include "u_collections.hpp"
#include "u_util.hpp"

namespace pcg::cosmoclick {
using namespace pce;
using namespace pce::frame;

enum class Scene { game, main_menu, game_over, quit };
class CosmoClick {
    RenderSystem& render_system;
    TickSystem tick_system { };
    InputSystem input_system { };
    ui::NodeRenderSystem node_render_system { render_system };

    Scene scene { Scene::main_menu };

public:
    TickFrame tick_frame { };
    InspectorFrame debug_frame { };

    explicit CosmoClick(RenderSystem& render_system);
    void Tick();
    [[nodiscard]] constexpr b8 IsRunning() const { return tick_system.running; }

private:
    void GameLoop();
    // Scene MainMenuScene();
    // Scene GameScene();
    // Scene GameOverScene();
};
CosmoClick::CosmoClick(RenderSystem& render_system) : render_system(render_system) {

}
void CosmoClick::Tick() {
    tick_system();
    input_system();
    node_render_system.HoverClickEvents(input_system);

    GameLoop();

    render_system();
    node_render_system.RenderTrees(render_system.renderer);
    render_system.End();
    tick_system.End();
}
void CosmoClick::GameLoop() {
    if (input_system.keys[SDLK_ESCAPE]) { tick_system.running = false; }

    if (input_system.LeftMouseDown()) {
        debug_frame.tree.MarkDirty();
        debug_frame.ShowElementStructure(node_render_system.hovered);
    }
    tick_frame.tree.MarkDirty();
    tick_frame.SetInfo(tick_system.tick.Value(), 1.0F / tick_system.tick_time, 1.0F / tick_system.delta_time);

    switch (scene) {
        // case Scene::game:
        //     scene = GameScene();
        // break;
        // case Scene::main_menu:
        //     scene = MainMenuScene();
        // break;
        // case Scene::game_over:
        //     scene = GameOverScene();
        break;
        case Scene::quit:
            Logger().Log("Quit requested");
        tick_system.running = false;
        return;
    }
}


} // namespace pcg::cosmoclick

void pcg::arcade::RunCosmoClick(pce::RenderSystem& render_system) {
    cosmoclick::CosmoClick cosmo_click { render_system };
    while (cosmo_click.IsRunning()) { cosmo_click.Tick(); }
}
