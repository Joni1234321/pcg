#include "g_arcade.hpp"

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

using Count = NamedType<u32, struct CountTag, Arithmetic>;
using Money = NamedType<u32, struct MoneyTag, Arithmetic, FormatLongNumber>;
using Income = NamedType<u32, struct IncomeTag, Arithmetic, FormatLongNumber>;
enum class ValueUnitTextSize : u8 { small, normal, larger };
enum class Unit : u8 { cosmos, cosmos_per_second };
enum class Scene : u8 { start, game, quit };
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
// ui
struct UIFlags {
    enum Flag : u32 { buildings, money, planet, size };
    [[nodiscard]] constexpr b8 Any() const noexcept { return flags.any(); }
    constexpr void Set() noexcept { flags.set(); }
    constexpr void Reset() noexcept { flags.reset(); }
    [[nodiscard]] constexpr b8 Only(const Flag flag) const noexcept {
        std::bitset<static_cast<size_t>(size)> mask { };
        return flags == mask.set(flag);
    }
    std::bitset<static_cast<size_t>(size)> flags;
};
constexpr UIFlags& operator&(UIFlags& flags, const UIFlags::Flag flag) {
    flags.flags.set(flag);
    return flags;
}
constexpr b8 operator|(const UIFlags& flags, const UIFlags::Flag flag) noexcept { return flags.flags.test(flag); }

template <class T> struct ValueUnit : NodeComponentBase {
    struct Property {
        const T& value;
        Unit unit;
        FontSizes font_size;
        SDL_Color color;
    };
    explicit ValueUnit(const NodeReference parent) : NodeComponentBase { parent.tree, B(parent).Node(hug).Alignment(top_right).Build() } { }
    void SetProperty(const Property& property) const;
    void SetValue(const T& new_value) const { data[root.tree].node_properties[value].text = FormatValue(new_value); }

    Handle<Node> value { B(root).Node(hug).Fill(colors::white).Build() };
    Handle<Node> unit { B(root).Node(hug).Fill(colors::white).Build() };
};
static_assert(NodeComponent<ValueUnit<u32>>);
struct BuildingComponent : NodeComponentBase {
    using Property = u32;
    explicit BuildingComponent(const NodeReference parent) : NodeComponentBase { parent.tree, B(parent).Node(fill, hug).Gap(6U).Build() } { }
    void SetProperty(const Property& property) const;

private:
    Handle<Node> item { B(root).Node(128U, 64U).Build() };
    Handle<Node> info { B(root).Node(fill, 64U).Direction(vertical).Fill(colors::jet).Padding2({ 4U, 2U }).GapAuto().Build() };
    Handle<Node> name { B(info).Node(hug).Text(FontSizes::body, colors::light_gray).Build() };
    Handle<Node> purchasing_info { B(info).Node(hug).Gap(10U).Build() };
    ValueUnit<Money> money { B(purchasing_info).Component<ValueUnit<Money>>() };
    ValueUnit<Income> income { B(purchasing_info).Component<ValueUnit<Income>>() };
};
static_assert(NodeComponent<BuildingComponent>);

struct GameFrame : Frame {
    static constexpr u32 PLANET_SIZE = 400U;
    static constexpr u32 PLANET_BORDER_SIZE = 10U;

    Handle<ParticleEmitter> emitter = data.Create<ParticleEmitter>(ParticleEmitter { float2 { 0, -50.0F } });

    Handle<Node> game { B(frame).Node(fill).Direction(vertical).Center().Padding2({ 0U, 40U }).Build() };
    Handle<Node> info { B(game).Node(500U, hug).Direction(vertical).Center().Fill(colors::clear).Padding2({ 50U, 4U }).Build() };
    ValueUnit<Money> money { B(info).Component<ValueUnit<Money>>({ Money { 0U }, Unit::cosmos, FontSizes::h1, colors::deep_gold }) };
    ValueUnit<Income> income { B(info).Component<ValueUnit<Income>>({ Income { 0U }, Unit::cosmos_per_second, FontSizes::h4, colors::silver }) };
    Handle<Node> click { B(game).Node(fill).Padding2(uint2 { 0U, 100U }).Alignment(top_center).Build() };
    Handle<Node> planet {
        B(click).Node(PLANET_SIZE + PLANET_BORDER_SIZE).Padding(PLANET_BORDER_SIZE).Fill(colors::white).OnClick([this] (const NodeReference) -> void {
            AnimationSystem::StartAnimation(this->planet_animation);
            constexpr Money money_per_click { 5U };
            singleton.Get<GameState>().money += money_per_click;
            singleton.Get<UIFlags>() & UIFlags::money;
            const uint2 mouse_position = singleton.Get<InputState>().mouse_position;
            const String text = std::to_string(money_per_click.Value());
            data[emitter].particles.items.push_back(Particle {
                                                        .position = { static_cast<f32>(mouse_position.x), static_cast<f32>(mouse_position.y) },
                                                        .text = std::unique_ptr<TTF_Text, DestroyText> { TTF_CreateText(singleton.Get<WindowState>().text_engine, singleton.Get<FontCollection>().GetFont(static_cast<FontSizes>(Rand(static_cast<u32>(FontSizes::body), static_cast<u32>(FontSizes::title)))).ToSDL(), text.c_str(), text.size()) },
                                                        .duration = ms32 { Rand(1000U, 3001U) }, });
        }).Build() };
    Handle<Node> planet_intra { B(planet).Node(fill).Fill(colors::dark_navy_blue).Build() };
    Handle<Node> build_menu { B(frame).Node(700U, hug).Direction(vertical).Padding(10U).Gap(10U).Fill(colors::cerulean).Build() };
    NodeComponentPool<BuildingComponent> shop { B(build_menu).Pool<BuildingComponent>() };

    Handle<Animation> planet_animation = AnimationSystem::Register(AnimationDesc {
                                                                       .action = [this] (const f32 t) -> void {
                                                                           static constexpr u32 PLANET_PADDING_START = 10U;
                                                                           const u32 padding_value = PLANET_PADDING_START + static_cast<u32>((1 - t) * PLANET_BORDER_SIZE);
                                                                           data[tree].styles[planet].padding = uint4 { padding_value, padding_value, padding_value, padding_value };
                                                                           data[tree].styles[planet].background_color = colors::LightenColor(colors::cyan, t);
                                                                           singleton.Get<UIFlags>() & UIFlags::planet;
                                                                       },
                                                                       .duration = ms32 { 400U },
                                                                       .state = AnimationState::persistent_stopped });
};
struct CosmoClickSystem {
    Scene scene { Scene::start };
    void operator()();

private:
    Scene GameScene();
};
struct CosmoClickUISystem {
    void operator()() const;
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
        { .name = "Dyson Sphere Segment", .texture = data.Create<Texture>(Asset("dyson-sphere.png")), .cost = Money { 2000000U }, .income = Income { 250000U } }, };
    singleton.Get<GameState>() = GameState {
        .building_counts = List { singleton.Get<GameDefines>().buildings.size(), Count { 0U } },
        .start_time = TimeNow(),
        .seconds_since_start = 0U,
        .money = Money { 100U },
        .income = Income { 0U } };

    orchestra.Add<DebugSystem>();

    orchestra.Add<TickSystem>();
    orchestra.Add<InputSystem>();
    orchestra.Add<NodeInputSystem>();

    orchestra.Add<CosmoClickSystem>();
    orchestra.Add<CosmoClickUISystem>();

    orchestra.Add<AnimationSystem>();
    orchestra.Add<NodeRenderSystem>();
    orchestra.Add<ParticleSystem>();
    orchestra.Add<PresentSystem>();
}
void CosmoClick::Tick() {
    if (singleton.Get<InputState>().keys_down[SDLK_ESCAPE]) { singleton.Get<TickState>().running = false; }
    orchestra.RunSystems();
}
void CosmoClickSystem::operator()() {
    const Scene start = scene;
    switch (scene) {
        case Scene::start:
            scene = Scene::game;
            break;
        case Scene::game:
            scene = GameScene();
            break;
        case Scene::quit:
            Logger().Log("Quit requested");
            singleton.Get<TickState>().running = false;
            break;
    }
    if (scene != start) { singleton.Get<UIFlags>().Set(); }
}

Scene CosmoClickSystem::GameScene() {
    UIFlags& ui_flags = singleton.Get<UIFlags>();
    GameState& game_data = singleton.Get<GameState>();
    const TimePoint update_time = TimeNow();
    const Duration time_since_start = update_time - game_data.start_time;
    const u32 seconds_since_start = static_cast<u32>(std::chrono::duration_cast<Seconds>(time_since_start).count());
    const u32 delta_seconds = seconds_since_start - game_data.seconds_since_start;
    if (delta_seconds > 0U) {
        game_data.money += Money { game_data.income.Value() * delta_seconds };
        game_data.seconds_since_start = seconds_since_start;
        ui_flags & UIFlags::money;
    }
    return Scene::game;
}
void CosmoClickUISystem::operator()() const {
    const GameState& game_state = singleton.Get<GameState>();
    UIFlags& ui_flags = singleton.Get<UIFlags>();
    GameFrame& game_frame = singleton.Get<GameFrame>();
    if (ui_flags | UIFlags::planet) { data[game_frame.tree].MarkDirty(game_frame.planet); }
    if (ui_flags | UIFlags::buildings) { game_frame.shop.Set(std::views::iota(0U, singleton.Get<GameDefines>().buildings.size())); }
    if (ui_flags | UIFlags::money) {
        game_frame.money.SetValue(game_state.money);
        game_frame.income.SetValue(game_state.income);
    }
    UIFlags planet_check { };
    planet_check & UIFlags::planet;
    if (ui_flags.Only(UIFlags::planet)) { data[game_frame.tree].MarkDirty(game_frame.planet); } else if (ui_flags.Any()) {
        data[game_frame.tree].MarkDirty();
        ui_flags.Reset();
    }
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
    data[root.tree].node_properties[value].text = FormatValue(property.value);
    data[root.tree].styles[value].background_color = property.color;
    data[root.tree].node_properties[unit].font_size = static_cast<FontSizes>(static_cast<f32>(property.font_size) * 0.66F);
    data[root.tree].node_properties[unit].text = UnitToString(property.unit);
    data[root.tree].styles[unit].background_color = property.color;
}
void BuildingComponent::SetProperty(const Property& property) const {
    const Count& count = singleton.Get<GameState>().building_counts[property];
    const Building& building = singleton.Get<GameDefines>().buildings[property];

    data[root.tree].node_properties[name].text = std::format("[{:04}] {:20}", count, building.name);
    data[root.tree].styles[item].texture = building.texture;
    money.SetProperty({ .value = building.cost, .unit = Unit::cosmos, .font_size = FontSizes::body, .color = colors::gold });
    income.SetProperty({ .value = building.income, .unit = Unit::cosmos_per_second, .font_size = FontSizes::body, .color = colors::silver });
    data[root.tree].node_properties[root.node].on_click = [&building, property] (const NodeReference) -> void {
        GameState& game_state = singleton.Get<GameState>();
        if (game_state.money >= building.cost) {
            ++game_state.building_counts[property];
            game_state.money -= building.cost;
            game_state.income += building.income;
            singleton.Get<UIFlags>() & UIFlags::buildings & UIFlags::money;
        }
    };
}
} // namespace pcg::cosmoclick

void pcg::arcade::RunCosmoClick() {
    cosmoclick::CosmoClick cosmo_click { };
    while (pce::singleton.Get<pce::TickState>().running) { cosmo_click.Tick(); }
}
