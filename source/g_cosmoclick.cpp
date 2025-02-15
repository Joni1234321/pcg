#include "g_arcade.hpp"

#include "0_engine/r_window.hpp"
#include "0_engine/u_algorithm.hpp"
#include "0_engine/u_collections.hpp"
#include "0_engine/u_colors.hpp"
#include "0_engine/u_types.hpp"

#include "1_systems/orchestra.hpp"
#include "1_systems/r_render.hpp"
#include "1_systems/r_ui_node.hpp"
#include "1_systems/t_debug_system.hpp"
#include "1_systems/t_tick_system.hpp"
#include "1_systems/u_animation_system.hpp"

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
    ValueUnit(NodeReference parent_reference_handle, const T& value, Unit unit, SDL_Color text_color, FontSizes font_sizes);
    constexpr void SetValue(const T& value);
    constexpr void SetUnit(Unit unit) const;

private:
    Handle<NodeTree> tree_handle;
    HandleOptional<Node> node_handle { };
    HandleOptional<Node> value_handle { };
    HandleOptional<Node> unit_handle { };
};
struct BuildItem {
    BuildItem(NodeReference parent_reference_handle, const Building& building);
    [[nodiscard]] Handle<Node> RootHandle() const;
    [[nodiscard]] Handle<Node> CountHandle() const;

private:
    Handle<NodeTree> tree_handle;
    HandleOptional<Node> build_item { };
    HandleOptional<Node> count_handle { };
};
struct GameFrame {
    Handle<NodeTree> tree_handle { data.Create<NodeTree>() };
    GameFrame();
    [[nodiscard]] constexpr b8 InsidePlanet(uint2 screen_position) const;
    [[nodiscard]] constexpr Handle<Node> PlanetHandle() const;
    [[nodiscard]] constexpr ValueUnit<Money>& GetMoneyValueUnit() const;
    [[nodiscard]] constexpr ValueUnit<Income>& GetIncomeValueUnit() const;
    [[nodiscard]] constexpr std::optional<u32> GetBuildItemAtPosition(uint2 screen_position);
    constexpr void UpdateBuildItemsCount(const List<Count>& building_counts);
    void RestartClickAnimation() const;

private:
    HandleOptional<Node> planet_handle;
    std::unique_ptr<ValueUnit<Money>> money;
    std::unique_ptr<ValueUnit<Income>> income;
    List<BuildItem> shop;
    HandleOptional<Animation> planet_animation_handle;
    HandleOptional<Animation> click_animation_handle;
};
enum class Scene { game, quit };
struct CosmoClickConfig {
    GameFrame game_frame { };
    GameData game_data { .building_counts = List { buildings.Size(), Count { 0U } }, .start_time = TimeNow(), .seconds_since_start = 0U, .money = Money { 100U }, .income = Income { 0U } };
    Scene scene { Scene::game };
};
struct CosmoClickSystem {
    static CosmoClickConfig cosmo_click_config;
    void operator()();

private:
    Scene GameScene();
};
inline CosmoClickConfig CosmoClickSystem::cosmo_click_config;
class CosmoClick {
    Orchestra orchestra { };
    TickSystem tick_system { };

public:
    CosmoClick();
    void Tick();
};

constexpr u32 planet_size = 400U;
constexpr u32 planet_border_size = 40U;
CosmoClick::CosmoClick() {
    Window::window_config.clear_color = colors::dark_grey;
    orchestra.Add<DebugSystem>();

    orchestra.Add<TickSystem>();
    orchestra.Add<InputSystem>();
    orchestra.Add<NodeInputSystem>();

    orchestra.Add<CosmoClickSystem>();

    orchestra.Add<AnimationSystem>();
    orchestra.Add<RenderClearSystem>();
    orchestra.Add<NodeRenderSystem>();
    orchestra.Add<PresentSystem>();
}
void CosmoClick::Tick() {
    if (InputSystem::input_config.keys_down[SDLK_ESCAPE]) { TickSystem::tick_config.running = false; }
    orchestra.RunSystems();
}
void CosmoClickSystem::operator()() {
    switch (cosmo_click_config.scene) {
        case Scene::game:
            cosmo_click_config.scene = GameScene();
            break;
        case Scene::quit:
            Logger().Log("Quit requested");
            TickSystem::tick_config.running = false;
            break;
    }
}
Scene CosmoClickSystem::GameScene() {
    GameData& game_data = cosmo_click_config.game_data;
    GameFrame& game_frame = cosmo_click_config.game_frame;
    const TimePoint update_time = TimeNow();
    const Duration time_since_start = update_time - game_data.start_time;
    const u32 seconds_since_start = std::chrono::duration_cast<Seconds>(time_since_start).count();
    const u32 delta_seconds = seconds_since_start - game_data.seconds_since_start;
    if (delta_seconds > 0U) {
        game_data.money += Money { game_data.income.Value() * delta_seconds };
        game_data.seconds_since_start = seconds_since_start;
    }
    if (InputSystem::input_config.left_mouse_down || InputSystem::input_config.left_mouse_up) {
        if (game_frame.InsidePlanet(InputSystem::input_config.mouse_position)) {
            constexpr Money click_money = Money { 5U };
            game_data.money += click_money;
            game_frame.RestartClickAnimation();
        }
    }
    if (InputSystem::input_config.left_mouse_down) {
        const std::optional<u32> building_index = game_frame.GetBuildItemAtPosition(InputSystem::input_config.mouse_position);
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
    data[game_frame.tree_handle].MarkDirty();
    game_frame.GetMoneyValueUnit().SetValue(game_data.money);
    game_frame.GetIncomeValueUnit().SetValue(game_data.income);
    return Scene::game;
}
// ui
template <class T> ValueUnit<T>::ValueUnit(const NodeReference parent_reference_handle, const T& value, const Unit unit, const SDL_Color text_color, const FontSizes font_sizes): tree_handle {
    parent_reference_handle.tree_handle } {
    node_handle = B(tree_handle, parent_reference_handle.node_handle, hug).Alignment(top_right).Build();
    value_handle = B(tree_handle, node_handle.GetHandle(), hug).Text("", font_sizes).Fill(text_color).Build();
    unit_handle = B(tree_handle, node_handle.GetHandle(), hug).Text(UnitToString(unit), static_cast<FontSizes>(static_cast<f32>(font_sizes) * 0.66F)).Fill(text_color).Build();
    SetValue(value);
}
template <class T> constexpr void ValueUnit<T>::SetValue(const T& value) { data[tree_handle].node_properties[value_handle.GetHandle()].text = std::format("{}", value); }
template <class T> constexpr void ValueUnit<T>::SetUnit(const Unit unit) const { data[tree_handle].node_properties[value_handle.GetHandle()].text = UnitToString(unit); }
BuildItem::BuildItem(const NodeReference parent_reference_handle, const Building& building) : tree_handle { parent_reference_handle.tree_handle } {
    build_item = B(tree_handle, parent_reference_handle.node_handle, { fill, hug }).Padding(10U).Direction(vertical).Center().Fill(colors::gray_tint).Build();
    const Handle<Node> upper = B(tree_handle, build_item.GetHandle(), { fill, hug }).Center().GapAuto().Build();
    const Handle<Node> lower = B(tree_handle, build_item.GetHandle(), { fill, hug }).Center().GapAuto().Build();
    count_handle = B(tree_handle, upper, hug).Text("0000", FontSizes::title).Fill(colors::black).Build();
    ValueUnit(NodeReference { tree_handle, upper }, building.cost, Unit::cosmos, colors::black, FontSizes::h1);
    B(tree_handle, lower, hug).Text(building.name, FontSizes::title).Fill(colors::black).Build();
    ValueUnit(NodeReference { tree_handle, lower }, building.income, Unit::cosmos_per_second, colors::black, FontSizes::h1);
}
Handle<Node> BuildItem::RootHandle() const { return build_item.GetHandle(); }
Handle<Node> BuildItem::CountHandle() const { return count_handle.GetHandle(); }
constexpr b8 GameFrame::InsidePlanet(const uint2 screen_position) const { return data[tree_handle].node_styles[planet_handle.GetHandle()].IsInside(screen_position); }
constexpr Handle<Node> GameFrame::PlanetHandle() const { return planet_handle.GetHandle(); }
constexpr ValueUnit<Money>& GameFrame::GetMoneyValueUnit() const { return *money.get(); }
constexpr ValueUnit<Income>& GameFrame::GetIncomeValueUnit() const { return *income.get(); }
constexpr std::optional<u32> GameFrame::GetBuildItemAtPosition(const uint2 screen_position) {
    return find_index_of(shop, true, [this, screen_position] (const BuildItem& build_item) -> b8 { return data[tree_handle].node_styles[build_item.RootHandle()].IsInside(screen_position); });
}
constexpr void GameFrame::UpdateBuildItemsCount(const List<Count>& building_counts) {
    for (const auto [build_item, building_count] : std::views::zip(shop, building_counts)) { data[tree_handle].node_properties[build_item.CountHandle()].text = std::format("{:04}", building_count); }
}
void GameFrame::RestartClickAnimation() const { AnimationSystem::StartAnimation(click_animation_handle.GetHandle()); }
GameFrame::GameFrame() {
    const Handle<Node> frame = B(tree_handle, fill, uint2 { 0U, 0U }).Build();
    const Handle<Node> game = B(tree_handle, frame, fill).Direction(vertical).Center().Build();
    const Handle<Node> title = B(tree_handle, game, hug).Fill(colors::white).Text("Cosmo Click", FontSizes::title).Build();
    money = std::make_unique<ValueUnit<Money>>(NodeReference { tree_handle, game }, Money { 0U }, Unit::cosmos, colors::white, FontSizes::h1);
    income = std::make_unique<ValueUnit<Income>>(NodeReference { tree_handle, game }, Income { 0U }, Unit::cosmos_per_second, colors::white, FontSizes::h4);
    const Handle<Node> click = B(tree_handle, game, fill).Padding2(uint2 { 0U, 100U }).Alignment(top_center).Build();
    planet_handle = B(tree_handle, click, Layout { uint2 { planet_size + planet_border_size, planet_size + planet_border_size } }).Padding(planet_border_size).Fill(colors::white).Build();
    const Handle<Node> planet_intra = B(tree_handle, planet_handle.GetHandle(), fill).Fill(colors::dark_navy_blue).Build();
    const Handle<Node> build_menu = B(tree_handle, frame, { 600U, hug }).Direction(vertical).Padding(10U).Gap(10U).Fill(colors::faded_green).Build();
    for (const Building& building : buildings) { shop.EmplaceBack(NodeReference { tree_handle, build_menu }, building); }

    // animation
    constexpr u32 planet_padding_start = 10U;
    const AnimationDesc planet_animation_desc {
        .action = [this] (const f32 t) {
            const u32 padding_value = planet_padding_start + t * planet_border_size;
            data[tree_handle].node_styles[planet_handle.GetHandle()].padding = uint4 { padding_value, padding_value, padding_value, padding_value };
        },
        .duration_ms = 500U, .state = AnimationState::repeat };
    const AnimationDesc click_animation_desc {
        .action = [this] (const f32 t) { data[tree_handle].node_styles[planet_handle.GetHandle()].background_color = colors::LightenColor(colors::blue, t); }, .duration_ms = 300U,
        .state = AnimationState::keep_alive_stopped };
    planet_animation_handle = AnimationSystem::Register(planet_animation_desc);
    click_animation_handle = AnimationSystem::Register(click_animation_desc);
}
} // namespace pcg::cosmoclick

void pcg::arcade::RunCosmoClick() {
    cosmoclick::CosmoClick cosmo_click { };
    while (pce::TickSystem::tick_config.running) { cosmo_click.Tick(); }
}
