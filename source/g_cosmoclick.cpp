#include "g_arcade.hpp"

#include "0_engine/r_window.hpp"
#include "0_engine/u_algorithm.hpp"
#include "0_engine/u_collections.hpp"
#include "0_engine/u_colors.hpp"
#include "0_engine/u_texture.hpp"
#include "0_engine/u_types.hpp"

#include "1_systems/i_input_system.hpp"
#include "1_systems/r_render.hpp"
#include "1_systems/r_ui_node.hpp"
#include "1_systems/t_debug_system.hpp"
#include "1_systems/t_tick_system.hpp"
#include "1_systems/u_animation_system.hpp"
#include "1_systems/u_orchestra.hpp"

namespace pcg::cosmoclick {
using namespace pce;
using namespace pce::ui;

using Count = NamedType<u32, struct CountTag, Arithmetic, FormatLongNumber>;
using Money = NamedType<u32, struct MoneyTag, Arithmetic, FormatLongNumber>;
using Income = NamedType<u32, struct IncomeTag, Arithmetic, FormatLongNumber>;
enum class ValueUnitTextSize : u8 { small, normal, larger };
enum class Unit : u8 { cosmos, cosmos_per_second };
enum class Scene { game, quit };
constexpr u32 planet_size = 400U;
constexpr u32 planet_border_size = 40U;
struct Building {
    String name;
    Handle<Texture> texture;
    Money cost;
    Income income;
};
struct GameDefines {
    List<Building> buildings;
};
struct GameState {
    List<Count> building_counts { };
    TimePoint start_time { };
    u32 seconds_since_start { 0U };
    Money money { 0U };
    Income income { 0U };
};
constexpr String UnitToString(const Unit unit) {
    switch (unit) {
        case Unit::cosmos:
            return "¤";
        case Unit::cosmos_per_second:
            return "¤-s";
    }
    return "unknown unit";
}
struct NodeBase {
    NodeReference root;
    explicit NodeBase(NodeReference root) : root(root) {}
};
template <class T> struct ValueUnit {
    NodeReference root;
    struct Property {
        const T& value;
        Unit unit;
        FontSizes font_size;
    };
    explicit ValueUnit(NodeReference parent);
    void SetProperty(const Property& property) const;
    void SetValue(const T& new_value) const;

private:
    Handle<Node> value;
    Handle<Node> unit;
};
static_assert(NodeComponent<ValueUnit<u32>>);

struct BuildingComponent {
    NodeReference root;
    using Property = std::tuple<const Building&, const Count>;
    explicit BuildingComponent(NodeReference parent);
    void SetProperty(const Property& property) const;

private:
    Handle<Node> upper;
    Handle<Node> lower;
    Handle<Node> count;
    ValueUnit<Money> money;
    ValueUnit<Income> income;
};
static_assert(NodeComponent<BuildingComponent>);
struct GameFrame : LogLifetimeWithCount<GameFrame> {
    Handle<NodeTree> tree { data.Create<NodeTree>() };
    [[nodiscard]] constexpr b8 InsidePlanet(uint2 screen_position) const;
    [[nodiscard]] constexpr Handle<Node> PlanetHandle() const;
    [[nodiscard]] constexpr ValueUnit<Money>& GetMoneyValueUnit();
    [[nodiscard]] constexpr ValueUnit<Income>& GetIncomeValueUnit();
    [[nodiscard]] std::optional<u32> GetBuildItemAtPosition(uint2 screen_position) const;
    void UpdateBuildItemsCount(const List<Count>& building_counts);
    void RestartClickAnimation() const;

private:
    void PlanetAnimation(const f32 t) const noexcept {
        static constexpr u32 PLANET_PADDING_START = 10U;
        const u32 padding_value = PLANET_PADDING_START + static_cast<u32>(t * planet_border_size);
        data[tree].styles[planet].padding = uint4 { padding_value, padding_value, padding_value, padding_value };
    }
    void ClickAnimation(const f32 t) const noexcept { data[tree].styles[planet].background_color = colors::LightenColor(colors::blue, t); }
    Handle<Node> frame { B(tree, fill, uint2 { 0U, 0U }).Build() };
    Handle<Node> game { B(tree, frame, fill).Direction(vertical).Center().Build() };

    Handle<Node> title { B(tree, game, hug).Fill(colors::white).Text("Cosmo Click", FontSizes::title).Build() };
    ValueUnit<Money> money = SingleComponent<ValueUnit<Money>>(NodeReference { tree, game }, ValueUnit<Money>::Property { Money { 0U }, Unit::cosmos, FontSizes::h1 });
    ValueUnit<Income> income = SingleComponent<ValueUnit<Income>>(NodeReference { tree, game }, ValueUnit<Income>::Property { Income { 0U }, Unit::cosmos_per_second, FontSizes::h4 });
    Handle<Node> click { B(tree, game, fill).Padding2(uint2 { 0U, 100U }).Alignment(top_center).Build() };
    Handle<Node> planet { B(tree, click, Layout { uint2 { planet_size + planet_border_size, planet_size + planet_border_size } }).Padding(planet_border_size).Fill(colors::white).Build() };
    Handle<Node> planet_intra { B(tree, planet, fill).Fill(colors::dark_navy_blue).Build() };

    Handle<Node> build_menu { B(tree, frame, { 600U, hug }).Direction(vertical).Padding(10U).Gap(10U).Fill(colors::faded_green).Build() };
    NodeComponentPool<BuildingComponent> shop { NodeReference { .tree = tree, .node = build_menu } };

    AnimationDesc planet_animation_desc { .action = [this] (const f32 t) -> void { this->PlanetAnimation(t); }, .duration_ms = 500U, .state = AnimationState::repeat };
    AnimationDesc click_animation_desc { .action = [this] (const f32 t) -> void { this->ClickAnimation(t); }, .duration_ms = 300U, .state = AnimationState::keep_alive_stopped };
    Handle<Animation> planet_animation_handle = AnimationSystem::Register(planet_animation_desc);
    Handle<Animation> click_animation = AnimationSystem::Register(click_animation_desc);
};
struct CosmoClickConfig {
    GameFrame game_frame { };
    Scene scene { Scene::game };
};
struct CosmoClickSystem {
    CosmoClickConfig cosmo_click_config;
    void operator()();

private:
    Scene GameScene();
};
class CosmoClick : LogLifetimeWithCount<CosmoClick> {
    Orchestra orchestra { };

public:
    CosmoClick();
    void Tick();
};
CosmoClick::CosmoClick() {
    singleton.Get<WindowState>().clear_color = colors::dark_grey;
    singleton.Get<GameDefines>().buildings = List<Building> {
        { .name = "Mine", .texture = data.Create<Texture>(Asset("mine-small.png")), .cost = Money { 100U }, .income = Income { 1U } },
        { .name = "Factory", .texture = data.Create<Texture>(Asset("factory-small.png")), .cost = Money { 500U }, .income = Income { 10U } },
        { .name = "Spaceport", .texture = data.Create<Texture>(Asset("spaceport-small.png")), .cost = Money { 2000U }, .income = Income { 60U } },
        { .name = "Off World Colony", .texture = data.Create<Texture>(Asset("off-world-colony-small.png")), .cost = Money { 10000U }, .income = Income { 500U } } };
    singleton.Get<GameState>() = GameState {
        .building_counts = List { singleton.Get<GameDefines>().buildings.Size(), Count { 0U } },
        .start_time = TimeNow(),
        .seconds_since_start = 0U,
        .money = Money { 100U },
        .income = Income { 0U } };

    orchestra.Add<DebugSystem>();

    orchestra.Add<TickSystem>();
    orchestra.Add<InputSystem>();
    orchestra.Add<NodeInputSystem>();

    orchestra.Add<CosmoClickSystem>();

    orchestra.Add<AnimationSystem>();
    orchestra.Add<NodeRenderSystem>();
    orchestra.Add<PresentSystem>();
}
void CosmoClick::Tick() {
    if (singleton.Get<InputState>().keys_down[SDLK_ESCAPE]) { singleton.Get<TickState>().running = false; }
    orchestra.RunSystems();
}
void CosmoClickSystem::operator()() {
    switch (cosmo_click_config.scene) {
        case Scene::game:
            cosmo_click_config.scene = GameScene();
            break;
        case Scene::quit:
            Logger().Log("Quit requested");
            singleton.Get<TickState>().running = false;
            break;
    }
}
Scene CosmoClickSystem::GameScene() {
    GameState& game_data = singleton.Get<GameState>();
    GameFrame& game_frame = cosmo_click_config.game_frame;
    const TimePoint update_time = TimeNow();
    const Duration time_since_start = update_time - game_data.start_time;
    const u32 seconds_since_start = static_cast<u32>(std::chrono::duration_cast<Seconds>(time_since_start).count());
    const u32 delta_seconds = seconds_since_start - game_data.seconds_since_start;
    if (delta_seconds > 0U) {
        game_data.money += Money { game_data.income.Value() * delta_seconds };
        game_data.seconds_since_start = seconds_since_start;
    }
    if (singleton.Get<InputState>().left_mouse_down || singleton.Get<InputState>().left_mouse_up) {
        if (game_frame.InsidePlanet(singleton.Get<InputState>().mouse_position)) {
            constexpr Money click_money = Money { 5U };
            game_data.money += click_money;
            game_frame.RestartClickAnimation();
        }
    }
    if (singleton.Get<InputState>().left_mouse_down) {
        const std::optional<u32> building_index = game_frame.GetBuildItemAtPosition(singleton.Get<InputState>().mouse_position);
        Logger().Log("Position {} | {}", singleton.Get<InputState>().mouse_position.x, singleton.Get<InputState>().mouse_position.y);
        if (building_index.has_value()) {
            const Building& building = singleton.Get<GameDefines>().buildings[building_index.value()];
            if (game_data.money >= building.cost) {
                ++game_data.building_counts[building_index.value()];
                game_data.money -= building.cost;
                game_data.income += building.income;
            }
        }
    }
    game_frame.UpdateBuildItemsCount(game_data.building_counts);

    data[game_frame.tree].MarkDirty();
    game_frame.GetMoneyValueUnit().SetValue(game_data.money);
    game_frame.GetIncomeValueUnit().SetValue(game_data.income);
    return Scene::game;
}
// ui
template <class T> ValueUnit<T>::ValueUnit(const NodeReference parent) : root { .tree = parent.tree, .node = B(parent.tree, parent.node, hug).Alignment(top_right).Build() },
                                                                         value { B(root.tree, root.node, hug).Fill(colors::white).Build() },
                                                                         unit { B(root.tree, root.node, hug).Fill(colors::white).Build() } { }
template <class T> void ValueUnit<T>::SetProperty(const Property& property) const {
    data[root.tree].node_properties[value].font_size = property.font_size;
    data[root.tree].node_properties[value].text = std::format("{}", property.value);
    data[root.tree].node_properties[unit].font_size = static_cast<FontSizes>(static_cast<f32>(property.font_size) * 0.66F);
    data[root.tree].node_properties[unit].text = UnitToString(property.unit);
}
template <class T> void ValueUnit<T>::SetValue(const T& new_value) const { data[root.tree].node_properties[value].text = std::format("{}", new_value); }
BuildingComponent::BuildingComponent(const NodeReference parent) : root {
                                                                       .tree = parent.tree,
                                                                       .node = B(parent.tree, parent.node, { fill, hug }).Padding(10U).Direction(vertical).Center().Fill(colors::gray_tint).Build() },
                                                                   upper { B(root.tree, root.node, { fill, hug }).Center().GapAuto().Build() },
                                                                   lower { B(root.tree, root.node, { fill, hug }).Center().GapAuto().Build() },
                                                                   count { B(root.tree, upper, hug).FontSize(FontSizes::title).Fill(colors::black).Build() },
                                                                   money { NodeReference { root.tree, upper } },
                                                                   income { NodeReference { root.tree, upper } } { }
void BuildingComponent::SetProperty(const Property& property) const {
    const auto& [building, building_count] = property;
    data[root.tree].styles[root.node].texture = building.texture;
    money.SetProperty({ .value = building.cost, .unit = Unit::cosmos, .font_size = FontSizes::h1 });
    income.SetProperty({ .value = building.income, .unit = Unit::cosmos_per_second, .font_size = FontSizes::h1 });
    data[root.tree].node_properties[count].text = std::format("{:04}", building_count);
}
constexpr b8 GameFrame::InsidePlanet(const uint2 screen_position) const { return data[tree].styles[planet].IsInside(screen_position); }
constexpr Handle<Node> GameFrame::PlanetHandle() const { return planet; }
constexpr ValueUnit<Money>& GameFrame::GetMoneyValueUnit() { return money; }
constexpr ValueUnit<Income>& GameFrame::GetIncomeValueUnit() { return income; }
std::optional<u32> GameFrame::GetBuildItemAtPosition(const uint2 screen_position) const { return shop.GetComponentAtPosition(screen_position);}
void GameFrame::UpdateBuildItemsCount(const List<Count>& building_counts) { shop.Set(std::views::zip(singleton.Get<GameDefines>().buildings, building_counts)); }
void GameFrame::RestartClickAnimation() const { AnimationSystem::StartAnimation(click_animation); }
} // namespace pcg::cosmoclick

void pcg::arcade::RunCosmoClick() {
    cosmoclick::CosmoClick cosmo_click { };
    while (pce::singleton.Get<pce::TickState>().running) { cosmo_click.Tick(); }
}
