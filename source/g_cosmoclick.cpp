#include "g_arcade.hpp"

#include "r_engine.hpp"
#include "r_ui.hpp"
#include "r_ui_node.hpp"
#include "u_collections.hpp"

namespace pcg::cosmoclick {
using namespace pce;
using namespace pce::ui;

using Money = NamedType<u32, struct MoneyTag, Arithmetic, FormatLongNumber>;
using Income = NamedType<u32, struct IncomeTag, Arithmetic, FormatLongNumber>;
struct Building {
    String name;
    Money cost;
    Income income;
};
struct GameData {
    Money money;
    Income income;
};
List<Building> buildings = {
    { "Mine", Money { 100 }, Income { 1 } }, { "Factory", Money { 500 }, Income { 10 } }, { "Spaceport", Money { 2000 }, Income { 60 } }, { "Off World Colony", Money { 10000 }, Income { 500 } } };
enum class ValueUnitTextSize : u8 { small, normal, larger };
enum class Unit : u8 { cosmos, cosmos_per_second };
constexpr String UnitToString(const Unit unit) {
    switch (unit) {
        case Unit::cosmos:
            return "¤";
        case Unit::cosmos_per_second:
            return "¤-s";
    }
    return "unknown unit";
}
template <class T> struct ValueUnit {
    ValueUnit(NodeTree& tree, NodeHandle parent_handle, const T& value, Unit unit, SDL_Color text_color, FontSizes font_sizes);
    constexpr void SetValue(const T& value);
    constexpr void SetUnit(Unit unit);

private:
    NodeTree& tree;
    NodeHandleOptional node_handle { };
    NodeHandleOptional value_handle { };
    NodeHandleOptional unit_handle { };
};
struct BuildItem {
    BuildItem(NodeTree& tree, NodeHandle parent_handle, const Building& building) {
        build_item = B(tree, parent_handle, { fill, hug }).Padding(10U).Direction(vertical).Center().Fill(colors::gray_tint).Build();
        const NodeHandle upper = B(tree, build_item.GetHandle(), { fill, hug }).Center().GapAuto().Build();
        const NodeHandle lower = B(tree, build_item.GetHandle(), { fill, hug }).Center().GapAuto().Build();
        B(tree, upper, hug).Text("0000", FontSizes::title).Fill(colors::black).Build();
        ValueUnit(tree, upper, building.cost, Unit::cosmos, colors::black, FontSizes::h1);
        B(tree, lower, hug).Text(building.name, FontSizes::title).Fill(colors::black).Build();
        ValueUnit(tree, lower, building.income, Unit::cosmos_per_second, colors::black, FontSizes::h1);
    }

    [[nodiscard]] constexpr NodeHandle Handle() const { return build_item.GetHandle(); }

private:
    NodeHandleOptional build_item { };
};
struct GameFrame {
    NodeTree tree;
    GameFrame();
    [[nodiscard]] constexpr b8 InsidePlanet(uint2 screen_position);
    [[nodiscard]] constexpr ValueUnit<Money>& GetMoneyValueUnit();
    [[nodiscard]] constexpr ValueUnit<Income>& GetIncomeValueUnit();
    [[nodiscard]] constexpr std::optional<u32> GetBuildItemAtPosition(const uint2 screen_position);

private:
    NodeHandleOptional planet;
    std::unique_ptr<ValueUnit<Money>> money;
    std::unique_ptr<ValueUnit<Income>> income;
    List<BuildItem> shop;
};
enum class Scene { game, quit };
class CosmoClick {
    RenderSystem& render_system;
    TickSystem tick_system { };
    InputSystem input_system { };
    NodeRenderSystem node_render_system { render_system };
    DebugSystem debug_system { node_render_system };

    GameFrame game_frame { };

    GameData game_data { .money = Money { 100U }, .income = Income { 0U } };

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
    game_data.money += Money { game_data.income.Value() };

    if (input_system.LeftMouseDown() || input_system.LeftMouseUp()) {
        if (game_frame.InsidePlanet(input_system.MousePosition())) {
            constexpr Money click_money = Money { 5U };
            game_data.money += click_money;
        }
    }
    if (input_system.LeftMouseDown()) {
        std::optional<u32> building_index = game_frame.GetBuildItemAtPosition(input_system.MousePosition());
        if (building_index.has_value()) {
            const Building& building = buildings[building_index.value()];
            if (game_data.money >= building.cost) {
                game_data.money -= building.cost;
                game_data.income += building.income;
            }
        }
    }
    game_frame.tree.MarkDirty();
    game_frame.GetMoneyValueUnit().SetValue(game_data.money);
    game_frame.GetIncomeValueUnit().SetValue(game_data.income);
    return Scene::game;
}
// ui
template <class T> ValueUnit<T>::ValueUnit(NodeTree& tree, NodeHandle parent_handle, const T& value, const Unit unit, const SDL_Color text_color, const FontSizes font_sizes): tree { tree } {
    node_handle = B(tree, parent_handle, hug).Alignment(top_right).Build();
    value_handle = B(tree, node_handle.GetHandle(), hug).Text("", font_sizes).Fill(text_color).Build();
    unit_handle = B(tree, node_handle.GetHandle(), hug).Text(UnitToString(unit), static_cast<FontSizes>(static_cast<f32>(font_sizes) * 0.66F)).Fill(text_color).Build();
    SetValue(value);
}
template <class T> constexpr void ValueUnit<T>::SetValue(const T& value) { tree.GetProperties(value_handle.GetHandle()).text = std::format("{}", value); }
template <class T> constexpr void ValueUnit<T>::SetUnit(Unit unit) { tree.GetProperties(value_handle.GetHandle()).text = UnitToString(unit); }
constexpr b8 GameFrame::InsidePlanet(uint2 screen_position) { return tree.GetStyle(planet.GetHandle()).IsInside(screen_position); }
constexpr ValueUnit<Money>& GameFrame::GetMoneyValueUnit() { return *money.get(); }
constexpr ValueUnit<Income>& GameFrame::GetIncomeValueUnit() { return *income.get(); }
constexpr std::optional<u32> GameFrame::GetBuildItemAtPosition(const uint2 screen_position) {
    for (u32 i = 0U; i < shop.Size(); i++) { if (tree.GetStyle(shop[i].Handle()).IsInside(screen_position)) { return i; } }
    return std::nullopt;
}
GameFrame::GameFrame() {
    const NodeHandle frame = B(tree, fill, uint2 { 0U, 0U }).Build();
    const NodeHandle game = B(tree, frame, fill).Direction(vertical).Center().Build();
    const NodeHandle title = B(tree, game, hug).Fill(colors::white).Text("Cosmo Click", FontSizes::title).Build();
    money = std::make_unique<ValueUnit<Money>>(tree, game, Money { 0U }, Unit::cosmos, colors::white, FontSizes::h1);
    income = std::make_unique<ValueUnit<Income>>(tree, game, Income { 0U }, Unit::cosmos_per_second, colors::white, FontSizes::h4);
    const NodeHandle click = B(tree, game, fill).Padding2(uint2 { 0U, 100U }).Fill(colors::dark_grey).Alignment(top_center).Build();
    constexpr u32 planet_size = 200U;
    constexpr u32 planet_border_size = 10U;
    planet = B(tree, click, Layout { uint2 { planet_size + planet_border_size, planet_size + planet_border_size } }).Padding(5U).Fill(colors::white).Build();
    const NodeHandle planet_intra = B(tree, planet.GetHandle(), fill).Fill(colors::dark_navy_blue).Build();
    const NodeHandle build_menu = B(tree, frame, { 600U, hug }).Direction(vertical).Padding(10U).Gap(10U).Fill(colors::faded_green).Build();
    for (const Building& building : buildings) { shop.EmplaceBack(tree, build_menu, building); }
}
} // namespace pcg::cosmoclick

void pcg::arcade::RunCosmoClick(pce::RenderSystem& render_system) {
    cosmoclick::CosmoClick cosmo_click { render_system };
    while (cosmo_click.IsRunning()) { cosmo_click.Tick(); }
}
