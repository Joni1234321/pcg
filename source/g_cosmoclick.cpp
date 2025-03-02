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
template <class T> struct ValueUnit : NodeComponentBase {
    struct Property {
        const T& value;
        Unit unit;
        FontSizes font_size;
        SDL_Color color;
    };
    explicit ValueUnit(const NodeReference parent) : NodeComponentBase { parent.tree, B(parent).Node(hug).Alignment(top_right).Build() } { }
    void SetProperty(const Property& property) const;
    void SetValue(const T& new_value) const { data[root.tree].node_properties[value].text = std::format("{}", new_value); }

    Handle<Node> value { B(root).Node(hug).Fill(colors::white).Build() };
    Handle<Node> unit { B(root).Node(hug).Fill(colors::white).Build() };
};
static_assert(NodeComponent<ValueUnit<u32>>);
struct BuildingComponent : NodeComponentBase {
    using Property = std::tuple<const Building&, const Count>;
    explicit BuildingComponent(const NodeReference parent) : NodeComponentBase { parent.tree, B(parent).Node(fill, 100U).Direction(vertical).Build() } { }
    void SetProperty(const Property& property) const;

private:
    Handle<Node> info { B(root).Node(fill, hug).GapAuto().Build() };
    Handle<Node> name { B(info).Node(hug).Text(FontSizes::body, colors::black).Build() };
    Handle<Node> purchasing_info { B(info).Node(hug).Gap(10U).Build() };
    ValueUnit<Money> money { B(purchasing_info).Component<ValueUnit<Money>>() };
    ValueUnit<Income> income { B(purchasing_info).Component<ValueUnit<Income>>() };
    Handle<Node> item { B(root).Node(fill).Build() };
};
static_assert(NodeComponent<BuildingComponent>);

struct GameFrame : Frame {
    static constexpr u32 PLANET_SIZE = 400U;
    static constexpr u32 PLANET_BORDER_SIZE = 40U;

    Handle<Node> game { B(frame).Node(fill).Direction(vertical).Center().Build() };
    Handle<Node> title { B(game).Node(hug).Text("Cosmo Click", FontSizes::title, colors::white).Build() };
    ValueUnit<Money> money { B(game).Component<ValueUnit<Money>>({ Money { 0U }, Unit::cosmos, FontSizes::h1, colors::white }) };
    ValueUnit<Income> income { B(game).Component<ValueUnit<Income>>({ Income { 0U }, Unit::cosmos_per_second, FontSizes::h4, colors::white }) };
    Handle<Node> click { B(game).Node(fill).Padding2(uint2 { 0U, 100U }).Alignment(top_center).Build() };
    Handle<Node> planet { B(click).Node(PLANET_SIZE + PLANET_BORDER_SIZE).Padding(PLANET_BORDER_SIZE).Fill(colors::white).Build() };
    Handle<Node> planet_intra { B(planet).Node(fill).Fill(colors::dark_navy_blue).Build() };
    Handle<Node> build_menu { B(frame).Node(400U, hug).Direction(vertical).Padding(10U).Gap(2U).Fill(colors::faded_green).Build() };
    NodeComponentPool<BuildingComponent> shop { B(build_menu).Pool<BuildingComponent>() };

    Handle<Animation> planet_animation_handle = AnimationSystem::Register(AnimationDesc {
                                                                              .action = [this] (const f32 t) -> void {
                                                                                  static constexpr u32 PLANET_PADDING_START = 10U;
                                                                                  const u32 padding_value = PLANET_PADDING_START + static_cast<u32>(t * PLANET_BORDER_SIZE);
                                                                                  data[tree].styles[planet].padding = uint4 { padding_value, padding_value, padding_value, padding_value };
                                                                              },
                                                                              .duration_ms = 500U,
                                                                              .state = AnimationState::repeat });
    Handle<Animation> click_animation = AnimationSystem::Register(AnimationDesc {
                                                                      .action = [this] (const f32 t) -> void { data[tree].styles[planet].background_color = colors::LightenColor(colors::blue, t); },
                                                                      .duration_ms = 300U,
                                                                      .state = AnimationState::keep_alive_stopped });
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
        { .name = "Off World Colony", .texture = data.Create<Texture>(Asset("off-world-colony-small.png")), .cost = Money { 10000U }, .income = Income { 500U } },
        { .name = "Asteroid Mining Station", .texture = data.Create<Texture>(Asset("asteroid-mine.jpg")), .cost = Money { 25000U }, .income = Income { 1500U } },
        { .name = "Lunar Research Base", .texture = data.Create<Texture>(Asset("lunar-base.png")), .cost = Money { 75000U }, .income = Income { 5000U } },
        { .name = "Orbital Shipyard", .texture = data.Create<Texture>(Asset("orbital-shipyard.png")), .cost = Money { 200000U }, .income = Income { 15000U } },
        { .name = "Mars Colony", .texture = data.Create<Texture>(Asset("mars-colony.png")), .cost = Money { 500000U }, .income = Income { 40000U } },
        { .name = "Dyson Sphere Segment", .texture = data.Create<Texture>(Asset("dyson-sphere.png")), .cost = Money { 2000000U }, .income = Income { 250000U } },
    };
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
        if (data[game_frame.tree].styles[game_frame.planet].IsInside(singleton.Get<InputState>().mouse_position)) {
            constexpr Money click_money = Money { 5U };
            game_data.money += click_money;
            AnimationSystem::StartAnimation(game_frame.click_animation);
        }
    }
    if (singleton.Get<InputState>().left_mouse_down) {
        const std::optional<u32> building_index = game_frame.shop.GetComponentAtPosition(singleton.Get<InputState>().mouse_position);
        if (building_index.has_value()) {
            const Building& building = singleton.Get<GameDefines>().buildings[building_index.value()];
            if (game_data.money >= building.cost) {
                ++game_data.building_counts[building_index.value()];
                game_data.money -= building.cost;
                game_data.income += building.income;
            }
        }
    }
    game_frame.shop.Set(std::views::zip(singleton.Get<GameDefines>().buildings, game_data.building_counts));
    game_frame.money.SetValue(game_data.money);
    game_frame.income.SetValue(game_data.income);
    data[game_frame.tree].MarkDirty();
    return Scene::game;
}
// ui
constexpr String UnitToString(const Unit unit) {
    switch (unit) {
        case Unit::cosmos:
            return "¤";
        case Unit::cosmos_per_second:
            return "¤-s";
    }
    return "unknown unit";
}
template <class T> void ValueUnit<T>::SetProperty(const Property& property) const {
    data[root.tree].node_properties[value].font_size = property.font_size;
    data[root.tree].node_properties[value].text = std::format("{}", property.value);
    data[root.tree].styles[value].background_color = property.color;
    data[root.tree].node_properties[unit].font_size = static_cast<FontSizes>(static_cast<f32>(property.font_size) * 0.66F);
    data[root.tree].node_properties[unit].text = UnitToString(property.unit);
    data[root.tree].styles[unit].background_color = property.color;
}
void BuildingComponent::SetProperty(const Property& property) const {
    const auto& [building, building_count] = property;
    data[root.tree].node_properties[name].text = std::format("[{:04}] {:20}", building_count, building.name);
    data[root.tree].styles[item].texture = building.texture;
    money.SetProperty({ .value = building.cost, .unit = Unit::cosmos, .font_size = FontSizes::body, .color = colors::black });
    income.SetProperty({ .value = building.income, .unit = Unit::cosmos_per_second, .font_size = FontSizes::body, .color = colors::black });
}
} // namespace pcg::cosmoclick

void pcg::arcade::RunCosmoClick() {
    cosmoclick::CosmoClick cosmo_click { };
    while (pce::singleton.Get<pce::TickState>().running) { cosmo_click.Tick(); }
}
