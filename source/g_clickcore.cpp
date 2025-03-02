#include "g_arcade.hpp"

#include "0_engine/r_window.hpp"
#include "0_engine/u_collections.hpp"
#include "0_engine/u_colors.hpp"
#include "0_engine/u_types.hpp"
#include "0_engine/u_util.hpp"

#include "1_systems/i_input_system.hpp"
#include "1_systems/r_render.hpp"
#include "1_systems/r_ui_node.hpp"
#include "1_systems/u_orchestra.hpp"
#include "1_systems/t_debug_system.hpp"
#include "1_systems/t_tick_system.hpp"

namespace pcg::clickcore {
using namespace pce;
using namespace ui;

using Score = NamedType<u32, struct ScoreTag, Arithmetic, FormatLongNumber>;

enum class Scene { game, main_menu, game_over, quit };

struct HighScore {
    Score score;
    std::strong_ordering operator<=>(const HighScore& other) const noexcept { return score.Value() <=> other.score.Value(); }
};
struct RoundData {
    Score score { 0U };
    TimePoint start_time;
};
struct GameData {
    Multiset<HighScore> high_scores { };
};

struct HighScoreComponent : NodeComponentBase {
    using Property = std::tuple<long long, HighScore>;
    explicit HighScoreComponent(const NodeReference parent) : NodeComponentBase { parent.tree, B(parent).Node(150U, 100U).Center().Build() } { }
    void SetProperty(const Property& property) const;

    Handle<Node> score { B(root.node).Node(hug).Text(FontSizes::h2, colors::black).Build() };
};
struct HighScoreFrame : Frame {
    Handle<Node> root { B(frame).Node(hug).Direction(vertical).Center().Build()};
    Handle<Node> title { B(root).Node(hug).Text("High Scores", FontSizes::h1, colors::deep_purple).Build() };
    NodeComponentPool<HighScoreComponent> highscores { B(root).Pool<HighScoreComponent>() };
};
struct MainMenuFrame : Frame {
    [[nodiscard]] constexpr NodeStyle& StartButton() const { return data[tree].styles[start_button]; }
    constexpr void SetStartButtonText(String&& string) const { data[tree].node_properties[start_button].text = string; }
    [[nodiscard]] constexpr NodeStyle& SettingsButton() const { return data[tree].styles[settings_button]; }
    [[nodiscard]] constexpr NodeStyle& ExitButton() const { return data[tree].styles[exit_button]; }

    Handle<Node> root1 { B(frame).Node(fill).Padding2({ 100U, 30U }).Direction(vertical).Build() };
    Handle<Node> root { B(root1).Node(hug).Padding2({ 20U, 5U }).Fill(colors::deep_purple).Center().Direction(vertical).Build() };
    Handle<Node> title { B(root).Node(hug).Text("Hey Helene!", FontSizes::title, colors::light_sky_blue).Build() };
    Handle<Node> start_button { B(root).Node(hug).Text("Play", FontSizes::h1, colors::radiant_orange).Build() };
    Handle<Node> settings_button { B(root).Node(hug).Text("Settings", FontSizes::h1, colors::cool_teal).Build() };
    Handle<Node> exit_button {  B(root).Node(hug).Text("Exit" , FontSizes::h1, colors::ruby_red).Build() };
};
struct GameFrame : Frame {
    [[nodiscard]] constexpr NodeStyle& Frame() const { return data[tree].styles[root]; }
    [[nodiscard]] constexpr NodeStyle& GameArea() const { return data[tree].styles[game_area]; }
    [[nodiscard]] constexpr NodeStyle& Box() const { return data[tree].styles[box]; }
    [[nodiscard]] constexpr NodeStyle& ScoreBox() const { return data[tree].styles[score_box]; }
    void SetTime(const u32 time_ms) const { data[tree].node_properties[time_label].text = std::format("Time {:02}:{:02}.{:02}", time_ms / (1000U * 60U), time_ms / 1000U % 60U, time_ms % 100U); }
    void SetScore(const u32 score) const { data[tree].node_properties[score_label].text = std::format("Score {:4}", score); }

    Handle<Node> root { B(frame).Node(fill).Center().Direction(vertical).Texture(data.Create<Texture>(Asset("rainforest.jpg"))).Padding2(uint2 { 0U, 30U }).Build() };
    Handle<Node> title { B(root).Node(hug).Text("🎮 GAME TIME 🎮", FontSizes::h1, colors::navy_blue).Padding2(uint2 { 20U, 10U }).Center().Build() };
    Handle<Node> score_box { B(root).Node(hug).Padding2(uint2 { 10U, 5U }).Fill(colors::forest_green).Direction(vertical).Center().Build() };
    Handle<Node> time_label { B(score_box).Node(hug).Text("Time: 00:00.00", FontSizes::h2, colors::light_gray).Padding(5U).Build() };
    Handle<Node> score_label { B(score_box).Node(hug).Text("Score: 0000", FontSizes::h2, colors::gold).Padding(5U).Build() };
    Handle<Node> game_area { B(root).Node(fill).Padding2(uint2 { 300U, 100U }).Build() };
    Handle<Node> box { B(game_area).Node(100U).Fill(colors::ruby_red).Texture(data.Create<Texture>(Asset("parrot.jpg"))).Padding(5U).Build() };
};

class ClickCoreFrames {
    MainMenuFrame main_menu_frame { };
    GameFrame game_frame { };
    HighScoreFrame high_score_frame { };

public:
    [[nodiscard]] constexpr MainMenuFrame& MainMenuFrame();
    [[nodiscard]] constexpr GameFrame& GameFrame();
    [[nodiscard]] constexpr HighScoreFrame& HighScoreFrame();
};
class ClickCore {
    PresentSystem present_system { };
    TickSystem tick_system { };
    InputSystem input_system { };
    NodeInputSystem node_input_system { };
    NodeRenderSystem node_render_system { };
    DebugSystem debug_system { };

    ClickCoreFrames frames { };

    RoundData game { };
    GameData game_data { };
    Scene scene { Scene::main_menu };

public:
    b8 running = true;

    ClickCore() { singleton.Get<WindowState>().clear_color = colors::dark_slate; }
    void Tick();

private:
    void GameLoop();
    Scene MainMenuScene();
    Scene GameScene();
    Scene GameOverScene();
};
constexpr MainMenuFrame& ClickCoreFrames::MainMenuFrame() {
    data[main_menu_frame.tree].MarkDirty();
    return main_menu_frame;
}
constexpr GameFrame& ClickCoreFrames::GameFrame() {
    data[game_frame.tree].MarkDirty();
    return game_frame;
}
constexpr HighScoreFrame& ClickCoreFrames::HighScoreFrame() {
    data[high_score_frame.tree].MarkDirty();
    return high_score_frame;
}
void ClickCore::Tick() {
    tick_system();
    input_system();
    debug_system();
    node_input_system();
    if (singleton.Get<InputState>().keys_down[SDLK_ESCAPE]) { running = false; }

    GameLoop();

    node_render_system();
    present_system();
}
void ClickCore::GameLoop() {
    switch (scene) {
        case Scene::game:
            scene = GameScene();
            break;
        case Scene::main_menu:
            scene = MainMenuScene();
            break;
        case Scene::game_over:
            scene = GameOverScene();
            break;
        case Scene::quit:
            Logger().Log("Quit requested");
            running = false;
            break;
    }
}
Scene ClickCore::MainMenuScene() {
    InputState& input_state = singleton.Get<InputState>();
    if (input_state.left_mouse_down) {
        MainMenuFrame& main_menu_frame = frames.MainMenuFrame();
        if (main_menu_frame.StartButton().IsInside(input_state.mouse_position)) {
            data[main_menu_frame.tree].SetDisplay(false);
            game = RoundData { .score = Score { 0U }, .start_time = TimeNow() };
            return Scene::game;
        }
        if (main_menu_frame.SettingsButton().IsInside(input_state.mouse_position)) { return Scene::game_over; }
        if (main_menu_frame.ExitButton().IsInside(input_state.mouse_position)) { return Scene::quit; }
    }
    return Scene::main_menu;
}
Scene ClickCore::GameScene() {
    constexpr Seconds game_time = 2s;
    const Nanoseconds elapsed = TimeNow() - game.start_time;
    GameFrame& game_frame = frames.GameFrame();
    if (elapsed > game_time) {
        MainMenuFrame& main_menu_frame = frames.MainMenuFrame();
        HighScoreFrame& high_score_frame = frames.HighScoreFrame();

        game_data.high_scores.emplace(game.score);

        data[main_menu_frame.tree].SetDisplay(true);
        main_menu_frame.SetStartButtonText("Play Again");
        game_frame.SetTime(0);
        game_frame.Box().background_color.a = 0U;
        high_score_frame.highscores.Set(std::views::enumerate(game_data.high_scores | std::views::reverse));
        data[high_score_frame.tree].SetDisplay(true);
        data[high_score_frame.tree].MarkDirty();
        return Scene::game_over;
    }
    const u32 time_left_ms = static_cast<u32>(duration_cast<Milliseconds>(game_time - elapsed).count());
    game_frame.SetScore(game.score.Value());
    game_frame.SetTime(time_left_ms);

    const InputState& input_state = singleton.Get<InputState>();
    if (input_state.left_mouse_down || input_state.left_mouse_up) {
        if (game_frame.Box().IsInside(input_state.mouse_position)) {
            constexpr Score points = Score { 50U };
            game.score += points;
            const uint2 area = game_frame.GameArea().OuterBoxSize() - game_frame.Box().OuterBoxSize();
            const uint4 random_position = uint4 { Rand(area.x), Rand(area.y), 0U, 0U };
            game_frame.GameArea().padding = random_position;
        }
    }
    return Scene::game;
}
Scene ClickCore::GameOverScene() {
    constexpr f32 slow_down = 1000.0F;
    GameFrame& game_frame = frames.GameFrame();
    game_frame.Box().background_color = colors::AnimateDamp(static_cast<f32>(singleton.Get<TickState>().tick.Value()) / slow_down);
    game_frame.ScoreBox().background_color = colors::AnimateDamp(singleton.Get<TickState>().tick.Value() * 1.2F / slow_down);

    const Scene scene = MainMenuScene();
    if (scene == Scene::main_menu) { return Scene::game_over; }
    if (scene == Scene::game) {
        HighScoreFrame& high_score_frame = frames.HighScoreFrame();
        game_frame.Box().background_color = colors::ruby_red;
        game_frame.ScoreBox().background_color = colors::forest_green;
        data[high_score_frame.tree].SetDisplay(false);
        return Scene::game;
    }
    return scene;
}
void HighScoreComponent::SetProperty(const Property& property) const {
    const auto& [i, high_score] = property;
    const b8 alternate = i % 2 == 0;
    const SDL_Color primary = alternate ? colors::deep_purple : colors::radiant_orange;
    const SDL_Color secondary = !alternate ? colors::deep_purple : colors::radiant_orange;
    auto& tree = data[root.tree];
    data[root.tree].styles[root.node].background_color = primary;
    data[root.tree].styles[score].background_color = secondary;
    data[root.tree].node_properties[score].text = std::format("{:05}", high_score.score);
}
} // namespace pcg::clickcore

void pcg::arcade::RunClickCore() {
    clickcore::ClickCore click_core { };
    while (click_core.running) { click_core.Tick(); }
}
