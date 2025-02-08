#include "g_arcade.hpp"

#include "r_engine.hpp"
#include "r_ui.hpp"
#include "r_ui_node.hpp"
#include "u_collections.hpp"

namespace pcg::cosmoclick {
using namespace pce;
using namespace pce::ui;

using Money = NamedType<u32, struct MoneyTag, Arithmetic, FormatLongNumber>;
using Cost = NamedType<u32, struct CostTag, Arithmetic, FormatLongNumber>;
using Income = NamedType<u32, struct IncomeTag, Arithmetic, FormatLongNumber>;
struct Building {
    Cost cost;
    Income production;
};
struct GameData {
    Money money;
};
enum class ValueUnitTextSize : u8 { small, normal, larger };
enum class Unit : u8 { cosmos, cosmos_per_second };
struct ValueUnit {
    NodeTree& tree;
    NodeHandleOptional node_handle;
    NodeHandleOptional value_handle;
    NodeHandleOptional unit_handle;
    constexpr void SetValue(const String& value);
    constexpr void SetUnit(Unit unit);

    ValueUnit(NodeTree& tree, NodeHandle parent_handle, const String& value, Unit unit, SDL_Color text_color, FontSizes font_sizes);
};
struct GameFrame {
    NodeTree tree;
    GameFrame();
    [[nodiscard]] constexpr b8 InsidePlanet(uint2 screen_position) { return tree.GetStyle(planet.GetHandle()).IsInside(screen_position); }
    void SetMoney (Money new_money){ money->SetValue(std::format("{}", new_money)); }
    void SetIncome (Income new_income){ income->SetValue(std::format("{}", new_income)); }

private:
    NodeHandleOptional planet;
    std::unique_ptr<ValueUnit> money;
    std::unique_ptr<ValueUnit> income;
};
enum class Scene { game, quit };
class CosmoClick {
    RenderSystem& render_system;
    TickSystem tick_system { };
    InputSystem input_system { };
    NodeRenderSystem node_render_system { render_system };
    DebugSystem debug_system { node_render_system };

    GameFrame game_frame { };

    GameData game_data { .money = Money { 100U } };

    Scene scene { Scene::game };

public:
    explicit CosmoClick(RenderSystem& render_system);
    void Tick();
    [[nodiscard]] constexpr b8 IsRunning() const { return tick_system.running; }

private:
    void GameLoop();
    Scene GameScene();
};
CosmoClick::CosmoClick(RenderSystem& render_system) : render_system(render_system) {
    clear_color = colors::yellow;
    node_render_system.node_trees.EmplaceBack(game_frame.tree);
}
void CosmoClick::Tick() {
    tick_system();
    input_system();
    debug_system(input_system, tick_system);
    node_render_system.HoverClickEvents(input_system);
    if (input_system.keys[SDLK_ESCAPE]) { tick_system.running = false; }

    GameLoop();

    render_system();
    node_render_system.RenderTrees(render_system.renderer);
    render_system.End();
    tick_system.End();
}
void CosmoClick::GameLoop() {
    switch (scene) {
        case Scene::game:
            scene = GameScene();
            break;
        case Scene::quit:
            Logger().Log("Quit requested");
            tick_system.running = false;
            break;
    }
}
Scene CosmoClick::GameScene() {
    if (input_system.LeftMouseDown() || input_system.LeftMouseUp()) {
        if (game_frame.InsidePlanet(input_system.MousePosition())) {
            constexpr Money click_money = Money { 5U };
            game_data.money += click_money;
            game_frame.tree.MarkDirty();
            game_frame.SetMoney(game_data.money);
            game_frame.SetIncome(Income { 0U });
        }
    }
    return Scene::game;
}

NodeHandle BuildItem(NodeTree& tree, NodeHandle parent_handle, const String& name) {
    const NodeHandle build_item = B(tree, parent_handle, { fill, hug }).Padding(10U).Direction(vertical).Center().Fill(colors::gray_tint).Build();
    const NodeHandle upper = B(tree, build_item, { fill, hug }).Center().GapAuto().Build();
    const NodeHandle lower = B(tree, build_item, { fill, hug }).Center().GapAuto().Build();
    B(tree, upper, hug).Text("0000", FontSizes::title).Fill(colors::black).Build();
    B(tree, lower, hug).Text(name, FontSizes::title).Fill(colors::black).Build();
    ValueUnit(tree, upper, "2000", Unit::cosmos, colors::black, FontSizes::h1);
    ValueUnit(tree, lower, "10", Unit::cosmos_per_second, colors::black, FontSizes::h1);
    return build_item;
}
constexpr String UnitToString(const Unit unit) {
    switch (unit) {
        case Unit::cosmos:
            return "¤";
        case Unit::cosmos_per_second:
            return "¤-s";
    }
    return "unknown unit";
}
ValueUnit::ValueUnit(NodeTree& tree, NodeHandle parent_handle, const String& value, const Unit unit, const SDL_Color text_color, const FontSizes font_sizes) : tree { tree } {
    node_handle = B(tree, parent_handle, hug).Alignment(top_right).Build();
    value_handle = B(tree, node_handle.GetHandle(), hug).Text(value, font_sizes).Fill(text_color).Build();
    unit_handle = B(tree, node_handle.GetHandle(), hug).Text(UnitToString(unit), static_cast<FontSizes>(static_cast<f32>(font_sizes) * 0.66F)).Fill(text_color).Build();
}
constexpr void ValueUnit::SetValue(const String& value) { tree.GetProperties(value_handle.GetHandle()).text = value; }
constexpr void ValueUnit::SetUnit(const Unit unit) { tree.GetProperties(unit_handle.GetHandle()).text = UnitToString(unit); }
GameFrame::GameFrame() {
    const NodeHandle frame = B(tree, fill, uint2 { 0U, 0U }).Build();
    const NodeHandle game = B(tree, frame, fill).Direction(vertical).Center().Build();
    const NodeHandle title = B(tree, game, hug).Fill(colors::white).Text("Cosmo Click", FontSizes::title).Build();
    money = std::make_unique<ValueUnit>(tree, game, "1000", Unit::cosmos, colors::white, FontSizes::h1);
    income = std::make_unique<ValueUnit>(tree, game, "1000", Unit::cosmos_per_second, colors::white, FontSizes::h4);
    const NodeHandle click = B(tree, game, fill).Padding2(uint2 { 0U, 100U }).Fill(colors::dark_grey).Alignment(top_center).Build();
    constexpr u32 planet_size = 200U;
    constexpr u32 planet_border_size = 10U;
    planet = B(tree, click, Layout { uint2 { planet_size + planet_border_size, planet_size + planet_border_size } }).Padding(5U).Fill(colors::white).Build();
    const NodeHandle planet_intra = B(tree, planet.GetHandle(), fill).Fill(colors::dark_navy_blue).Build();
    const NodeHandle build_menu = B(tree, frame, { 600U, hug }).Direction(vertical).Padding(10U).Gap(10U).Fill(colors::faded_green).Build();
    BuildItem(tree, build_menu, "Mine");
    BuildItem(tree, build_menu, "Factory");
    BuildItem(tree, build_menu, "Spaceport");
    BuildItem(tree, build_menu, "Off World Colony");
}
} // namespace pcg::cosmoclick

void pcg::arcade::RunCosmoClick(pce::RenderSystem& render_system) {
    cosmoclick::CosmoClick cosmo_click { render_system };
    while (cosmo_click.IsRunning()) { cosmo_click.Tick(); }
}
