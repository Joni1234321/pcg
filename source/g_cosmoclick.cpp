#include "g_arcade.hpp"

#include "r_engine.hpp"
#include "r_ui.hpp"
#include "r_ui_node.hpp"
#include "u_collections.hpp"

namespace pcg::cosmoclick {
using namespace pce;
using namespace pce::ui;

// struct EngineStructure {
//     List<std::function<void()>> systems;
//
//     void RunSystems() {
//         for (std::function<void()>& system : systems) {
//             auto start = std::chrono::high_resolution_clock::now();
//             system();  // Run the system
//             auto end = std::chrono::high_resolution_clock::now();
//             std::chrono::duration<double, std::milli> elapsed = end - start;
//             Logger().Log("System took {}ms", elapsed.count());
//         }
//     }
//     template<typename T> T& Get () { return systems | std::views::filter([](std::function<void()> f) {  return true; }); }
//
// };
//
// struct InputDirectory {
//     InputSystem input_system;
//     void operator()() {
//         input_system();
//     }
// };
// struct UpdateDirectory {
//     void operator()() {
//         input_system();
//     }
// };
// struct RenderDirectory {
//     RenderSystem render_system;
//     AnimationSystem animation_system;
//     NodeRenderSystem node_render_system;
//
//     void operator()() {
//         render_system();
//         animation_system();
//         node_render_system.RenderTrees(render_system.renderer);
//     }
// };
// void CreateEngine () {
//     EngineStructure engine;
//
//     engine.systems.PushBack(InputSystem {  });
//     engine.systems.PushBack(RenderSystem { uint2(100, 100) });
//
// }
//
// void CosmoClick::Tick() {
//     tick_system();
//     input_system();
//     debug_system(input_system, tick_system);
//     node_render_system.HoverClickEvents(input_system);
//     if (input_system.keys[SDLK_ESCAPE]) { tick_system.running = false; }
//
//     GameLoop();
//
//     animation_system();
//     render_system();
//     node_render_system.RenderTrees(render_system.renderer);
//     render_system.End();
//     tick_system.End();
// }
using Count = NamedType<u32, struct CountTag, Arithmetic, FormatLongNumber>;
using Money = NamedType<u32, struct MoneyTag, Arithmetic, FormatLongNumber>;
using Income = NamedType<u32, struct IncomeTag, Arithmetic, FormatLongNumber>;
struct Building {
    String name;
    Money cost;
    Income income;
};
struct GameData {
    List<Count> building_counts;
    TimePoint start_time;
    u32 seconds_since_start;
    Money money;
    Income income;
};
List<Building> buildings = {
    { "Mine", Money { 100U }, Income { 1U } }, { "Factory", Money { 500U }, Income { 10U } }, { "Spaceport", Money { 2000U }, Income { 60U } }, { "Off World Colony", Money { 10000U }, Income { 500U } } };

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
        count_handle = B(tree, upper, hug).Text("0000", FontSizes::title).Fill(colors::black).Build();
        ValueUnit(tree, upper, building.cost, Unit::cosmos, colors::black, FontSizes::h1);
        B(tree, lower, hug).Text(building.name, FontSizes::title).Fill(colors::black).Build();
        ValueUnit(tree, lower, building.income, Unit::cosmos_per_second, colors::black, FontSizes::h1);
    }

    [[nodiscard]] constexpr NodeHandle Handle() const;
    [[nodiscard]] constexpr NodeHandle CountHandle() const;

private:
    NodeHandleOptional build_item { };
    NodeHandleOptional count_handle { };
};
struct GameFrame {
    NodeTree& tree;
    AnimationSystem& animation_system;
    GameFrame(AnimationSystem& animation_system);
    [[nodiscard]] constexpr b8 InsidePlanet(uint2 screen_position);
    [[nodiscard]] constexpr NodeHandle PlanetHandle();
    [[nodiscard]] constexpr ValueUnit<Money>& GetMoneyValueUnit();
    [[nodiscard]] constexpr ValueUnit<Income>& GetIncomeValueUnit();
    [[nodiscard]] constexpr std::optional<u32> GetBuildItemAtPosition(uint2 screen_position);
    constexpr void UpdateBuildItemsCount(const List<Count>& building_counts);
    void RestartClickAnimation() const;

private:
    NodeHandleOptional planet_handle;
    std::unique_ptr<ValueUnit<Money>> money;
    std::unique_ptr<ValueUnit<Income>> income;
    List<BuildItem> shop;
    AnimationHandle planet_animation_handle;
    AnimationHandle click_animation_handle;
};
enum class Scene { game, quit };
class CosmoClick {
    RenderSystem render_system { };
    TickSystem tick_system { };
    InputSystem input_system { };
    NodeRenderSystem node_render_system { };
    DebugSystem debug_system { };
    AnimationSystem animation_system { };

    GameFrame game_frame { animation_system };

    GameData game_data { .building_counts = List { buildings.Size(), Count { 0U } }, .start_time = TimeNow(), .seconds_since_start = 0U, .money = Money { 100U }, .income = Income { 0U } };

    Scene scene { Scene::game };

public:
    CosmoClick();
    void Tick();
    [[nodiscard]] constexpr b8 IsRunning() const;

private:
    void GameLoop();
    Scene GameScene();
};

constexpr u32 planet_size = 400U;
constexpr u32 planet_border_size = 40U;
CosmoClick::CosmoClick() {
    clear_color = colors::yellow;
}
void CosmoClick::Tick() {
    tick_system();
    input_system();
    node_render_system.HoverClickEvents(input_system);
    if (input_system.keys[SDLK_ESCAPE]) { tick_system.running = false; }

    GameLoop();

    debug_system(input_system, tick_system, node_render_system);
    animation_system();
    render_system();
    node_render_system.RenderTrees(Window::renderer);
    render_system.Present();
    tick_system.CaptureTime();
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
    const TimePoint update_time = TimeNow();
    const Duration time_since_start = update_time - game_data.start_time;
    const u32 seconds_since_start = std::chrono::duration_cast<Seconds>(time_since_start).count();
    const u32 delta_seconds = seconds_since_start - game_data.seconds_since_start;
    if (delta_seconds > 0U) {
        game_data.money += Money { game_data.income.Value() * delta_seconds };
        game_data.seconds_since_start = seconds_since_start;
    }
    if (input_system.LeftMouseDown() || input_system.LeftMouseUp()) {
        if (game_frame.InsidePlanet(input_system.MousePosition())) {
            constexpr Money click_money = Money { 5U };
            game_data.money += click_money;
            game_frame.RestartClickAnimation();
        }
    }
    if (input_system.LeftMouseDown()) {
        const std::optional<u32> building_index = game_frame.GetBuildItemAtPosition(input_system.MousePosition());
        if (building_index.has_value()) {
            const Building& building = buildings[building_index.value()];
            if (game_data.money >= building.cost) {
                ++game_data.building_counts[building_index.value()];
                game_data.money -= building.cost;
                game_data.income += building.income;
                game_frame.UpdateBuildItemsCount(game_data.building_counts);
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
constexpr NodeHandle BuildItem::Handle() const { return build_item.GetHandle(); }
constexpr NodeHandle BuildItem::CountHandle() const { return count_handle.GetHandle(); }
constexpr b8 GameFrame::InsidePlanet(uint2 screen_position) { return tree.GetStyle(planet_handle.GetHandle()).IsInside(screen_position); }
constexpr NodeHandle GameFrame::PlanetHandle() { return planet_handle.GetHandle(); }
constexpr ValueUnit<Money>& GameFrame::GetMoneyValueUnit() { return *money.get(); }
constexpr ValueUnit<Income>& GameFrame::GetIncomeValueUnit() { return *income.get(); }
constexpr std::optional<u32> GameFrame::GetBuildItemAtPosition(const uint2 screen_position) {
    for (u32 i = 0U; i < shop.Size(); ++i) { if (tree.GetStyle(shop[i].Handle()).IsInside(screen_position)) { return i; } }
    return std::nullopt;
}
constexpr void GameFrame::UpdateBuildItemsCount(const List<Count>& building_counts) {
    for (u32 i = 0U; i < building_counts.Size(); ++i) { tree.GetProperties(shop[i].CountHandle()).text = std::format("{:04}", building_counts[i]); }
}
void GameFrame::RestartClickAnimation() const { animation_system.StartAnimation(click_animation_handle); }
constexpr b8 CosmoClick::IsRunning() const { return tick_system.running; }
GameFrame::GameFrame(AnimationSystem& animation_system) : tree { NodeRenderSystem::node_trees.EmplaceBack() }, animation_system { animation_system } {
    const NodeHandle frame = B(tree, fill, uint2 { 0U, 0U }).Build();
    const NodeHandle game = B(tree, frame, fill).Direction(vertical).Center().Build();
    const NodeHandle title = B(tree, game, hug).Fill(colors::white).Text("Cosmo Click", FontSizes::title).Build();
    money = std::make_unique<ValueUnit<Money>>(tree, game, Money { 0U }, Unit::cosmos, colors::white, FontSizes::h1);
    income = std::make_unique<ValueUnit<Income>>(tree, game, Income { 0U }, Unit::cosmos_per_second, colors::white, FontSizes::h4);
    const NodeHandle click = B(tree, game, fill).Padding2(uint2 { 0U, 100U }).Fill(colors::dark_grey).Alignment(top_center).Build();
    planet_handle = B(tree, click, Layout { uint2 { planet_size + planet_border_size, planet_size + planet_border_size } }).Padding(planet_border_size).Fill(colors::white).Build();
    const NodeHandle planet_intra = B(tree, planet_handle.GetHandle(), fill).Fill(colors::dark_navy_blue).Build();
    const NodeHandle build_menu = B(tree, frame, { 600U, hug }).Direction(vertical).Padding(10U).Gap(10U).Fill(colors::faded_green).Build();
    for (const Building& building : buildings) { shop.EmplaceBack(tree, build_menu, building); }

    // animation
    constexpr u32 planet_padding_start = 10U;
    const AnimationDesc planet_animation_desc {
        .action = [this] (const f32 t) {
            const u32 padding_value = planet_padding_start + t * planet_border_size;
            tree.GetStyle(planet_handle.GetHandle()).padding = uint4 { padding_value, padding_value, padding_value, padding_value };
        },
        .duration_ms = 500U, .state = AnimationState::repeat };
    const AnimationDesc click_animation_desc {
        .action = [this] (const f32 t) { tree.GetStyle(planet_handle.GetHandle()).background_color = LightenColor(colors::blue, t); }, .duration_ms = 300U, .state = AnimationState::keep_alive_stopped };
    planet_animation_handle = animation_system.Register(planet_animation_desc);
    click_animation_handle = animation_system.Register(click_animation_desc);
}
} // namespace pcg::cosmoclick

void pcg::arcade::RunCosmoClick() {
    cosmoclick::CosmoClick cosmo_click { };
    while (cosmo_click.IsRunning()) { cosmo_click.Tick(); }
}
