module;

export module pcg.g_clickcore;

import pcg.g_arcade;

import pce.collections;
import pce.util;

import pcs.input;
import pcs.render;
import pcs.node;
import pcs.debug;
import pcs.tick;
import pcs.orchestra;

import pce.colors;
import pce.std;

namespace pcg::clickcore {
using namespace pce;
using namespace ui;

using Score = StrongType<u32, struct ScoreTag, Arithmetic, FormatLongNumber>;

enum class Scene : u8 { game, main_menu, game_over, quit };

struct HighScore {
    Score score;
    std::strong_ordering operator<=>(const HighScore& other) const noexcept { return score.value <=> other.score.value; }
};
struct RoundData {
    Score score { 0U };
    miliseconds32 start_time { TimeNowMS() };
};
struct GameData {
    Multiset<HighScore> high_scores { };
};
struct HighScoreComponent : NodeComponentBase {
    using Property = std::tuple<i64, HighScore>;
    explicit HighScoreComponent(const NodeReference parent) : NodeComponentBase { parent.tree, B(parent).Node(200U, hug).Center().Build() } { }
    void SetProperty(const Property& property) const;

    Handle<Node> score { B(root.node).Node(hug).Text(FontSizes::h2, colors::COLOR_BLACK).Build() };
};
struct HighScoreFrame : Frame {
    Handle<Node> root { B(frame).Node(fill).Direction(vertical).Center().Build() };
    Handle<Node> title { B(root).Node(hug).Text("High Scores", FontSizes::h1, colors::COLOR_DEEP_PURPLE).Build() };
    NodeComponentPool<HighScoreComponent> highscores { B(root).Pool<HighScoreComponent>() };
};
struct MainMenuFrame : Frame {
    Handle<Node> root { B(frame).Node(hug).Padding2({ 100U, 30U }).Direction(vertical).Center().Build() };
    Handle<Node> title { B(root).Node(hug).Text("Hey Helene!", FontSizes::title, colors::COLOR_LIGHT_SKY_BLUE).Build() };
    Handle<Node> menu { B(root).Node(hug).Padding2({ 20U, 5U }).Fill(colors::COLOR_DEEP_PURPLE).Center().Direction(vertical).Build() };
    Handle<Node> start_button { B(menu).Node(hug).Text("Play", FontSizes::h1, colors::COLOR_RADIANT_ORANGE).Build() };
    Handle<Node> settings_button { B(menu).Node(hug).Text("Settings", FontSizes::h1, colors::COLOR_COOL_TEAL).Build() };
    Handle<Node> exit_button { B(menu).Node(hug).Text("Exit", FontSizes::h1, colors::COLOR_RUBY_RED).Build() };
};
struct GameFrame : Frame {
    void SetTime(const miliseconds32 time_ms) const { globalData[tree].node_properties[time_label].text = std::format("Time {:02}:{:02}.{:02}", time_ms.value / (1000U * 60U), time_ms.value / 1000U % 60U, time_ms.value % 100U); }

    Handle<Node> root { B(frame).Node(fill).Center().Direction(vertical).Texture(globalData.Create<Texture>(Asset("rainforest.jpg"))).Padding2(uint2 { 0U, 30U }).Build() };
    Handle<Node> title { B(root).Node(hug).Text("🎮 GAME TIME 🎮", FontSizes::h1, colors::COLOR_NAVY_BLUE).Padding2(uint2 { 20U, 10U }).Center().Build() };
    Handle<Node> score_box { B(root).Node(hug).Padding2(uint2 { 10U, 5U }).Fill(colors::COLOR_FOREST_GREEN).Direction(vertical).Center().Build() };
    Handle<Node> time_label { B(score_box).Node(hug).Text("Time: 00:00.00", FontSizes::h2, colors::COLOR_LIGHT_GRAY).Padding(5U).Build() };
    Handle<Node> score_label { B(score_box).Node(hug).Text("Score: 0000", FontSizes::h2, colors::COLOR_GOLD).Padding(5U).Build() };
    Handle<Node> game_area { B(root).Node(fill).Padding2(uint2 { 300U, 100U }).Build() };
    Handle<Node> box { B(game_area).Node(100U).Fill(colors::COLOR_RUBY_RED).Texture(globalData.Create<Texture>(Asset("parrot.jpg"))).Padding(5U).Build() };
};

class ClickCoreFrames {
    MainMenuFrame main_menu_frame { };
    HighScoreFrame high_score_frame { };
    GameFrame game_frame { };

public:
    [[nodiscard]] constexpr MainMenuFrame& MainMenuFrame();
    [[nodiscard]] constexpr GameFrame& GameFrame();
    [[nodiscard]] constexpr HighScoreFrame& HighScoreFrame();
};
class ClickCore {
    RenderWindowSystem present_system { };
    TickSystem tick_system { };
    InputSystem input_system { };
    InputNodeSystem node_input_system { };
    RenderNodeSystem node_render_system { };
    DebugSystem debug_system { };

    ClickCoreFrames frames { };

    RoundData game { };
    GameData game_data { };
    Scene scene { Scene::main_menu };

public:
    b8 running = true;

    ClickCore() { Singleton::Get<WindowState>().clear_color = colors::COLOR_DARK_SLATE; }
    void Tick();

private:
    void GameLoop();
    Scene MainMenuScene();
    Scene GameScene();
    Scene GameOverScene();
};
constexpr MainMenuFrame& ClickCoreFrames::MainMenuFrame() {
    globalData[main_menu_frame.tree].MarkDirty();
    return main_menu_frame;
}
constexpr GameFrame& ClickCoreFrames::GameFrame() {
    globalData[game_frame.tree].MarkDirty();
    return game_frame;
}
constexpr HighScoreFrame& ClickCoreFrames::HighScoreFrame() {
    globalData[high_score_frame.tree].MarkDirty();
    return high_score_frame;
}
void ClickCore::Tick() {
    tick_system();
    input_system();
    debug_system();
    node_input_system();
    if (Singleton::Get<InputState>().keys_down[SDLK_ESCAPE]) { running = false; }

    GameLoop();

    node_render_system();
    present_system();
}
void ClickCore::GameLoop() {
    switch (scene) {
        case Scene::game: scene = GameScene(); break;
        case Scene::main_menu: scene = MainMenuScene(); break;
        case Scene::game_over: scene = GameOverScene(); break;
        case Scene::quit:
            Logger().Log("Quit requested");
            running = false;
            break;
    }
}
Scene ClickCore::MainMenuScene() {
    const InputState& input_state = Singleton::Get<InputState>();
    if (input_state.left_mouse_down) {
        const MainMenuFrame& main_menu = frames.MainMenuFrame();
        if (globalData[main_menu.tree].styles[main_menu.start_button].IsInside(input_state.mouse_position)) {
            globalData[main_menu.tree].SetDisplay(false);
            game = RoundData { .score = Score { 0U }, .start_time = TimeNowMS() };
            return Scene::game;
        }
        if (globalData[main_menu.tree].styles[main_menu.settings_button].IsInside(input_state.mouse_position)) { return Scene::game_over; }
        if (globalData[main_menu.tree].styles[main_menu.exit_button].IsInside(input_state.mouse_position)) { return Scene::quit; }
    }
    return Scene::main_menu;
}
Scene ClickCore::GameScene() {
    static constexpr seconds32 GAME_TIME_SECS { 20U };
    static constexpr u32 SEC_TO_MS { 1000U };
    static constexpr miliseconds32 GAME_TIME { GAME_TIME_SECS.value * SEC_TO_MS };
    const miliseconds32 elapsed = TimeNowMS() - game.start_time;
    const GameFrame& game_frame = frames.GameFrame();
    if (elapsed > GAME_TIME) {
        game_data.high_scores.emplace(game.score);

        const MainMenuFrame& main_menu_frame = frames.MainMenuFrame();
        HighScoreFrame& high_score_frame = frames.HighScoreFrame();
        globalData[main_menu_frame.tree].SetDisplay(true);
        globalData[high_score_frame.tree].SetDisplay(true);

        globalData[main_menu_frame.tree].node_properties[main_menu_frame.start_button].text = "Play Again";
        game_frame.SetTime(miliseconds32 { 0U });
        globalData[game_frame.tree].styles[game_frame.box].background_color.a = 0U;
        high_score_frame.highscores.Set(std::views::zip(std::views::iota(0u, game_data.high_scores.size()), game_data.high_scores | std::views::reverse));
        return Scene::game_over;
    }
    const miliseconds32 time_left_ms = GAME_TIME - elapsed;
    globalData[game_frame.tree].node_properties[game_frame.score_label].text = std::format("Score {:4}", game.score);
    game_frame.SetTime(time_left_ms);

    const InputState& input_state = Singleton::Get<InputState>();
    if (input_state.left_mouse_down || input_state.left_mouse_up) {
        if (globalData[game_frame.tree].styles[game_frame.box].IsInside(input_state.mouse_position)) {
            constexpr Score points = Score { 50U };
            game.score += points;
            const uint2 area = globalData[game_frame.tree].styles[game_frame.game_area].OuterBoxSize() - globalData[game_frame.tree].styles[game_frame.box].OuterBoxSize();
            const uint4 random_position = uint4 { Rand(area.x), Rand(area.y), 0U, 0U };
            globalData[game_frame.tree].styles[game_frame.game_area].padding = random_position;
        }
    }
    return Scene::game;
}
Scene ClickCore::GameOverScene() {
    constexpr f32 slow_down = 1000.0F;
    const GameFrame& game_frame = frames.GameFrame();
    globalData[game_frame.tree].styles[game_frame.box].background_color = colors::AnimateDamp(static_cast<f32>(Singleton::Get<TickState>().tick.value) / slow_down);
    globalData[game_frame.tree].styles[game_frame.score_box].background_color = colors::AnimateDamp(Singleton::Get<TickState>().tick.value * 1.2F / slow_down);

    const Scene scene = MainMenuScene();
    if (scene == Scene::main_menu) { return Scene::game_over; }
    if (scene == Scene::game) {
        const HighScoreFrame& high_score_frame = frames.HighScoreFrame();
        globalData[game_frame.tree].styles[game_frame.box].background_color = colors::COLOR_RUBY_RED;
        globalData[game_frame.tree].styles[game_frame.score_box].background_color = colors::COLOR_FOREST_GREEN;
        globalData[high_score_frame.tree].SetDisplay(false);
        return Scene::game;
    }
    return scene;
}
void HighScoreComponent::SetProperty(const Property& property) const {
    const auto& [i, high_score] = property;
    const b8 alternate = i % 2 == 0;
    const Color primary = alternate ? colors::COLOR_DEEP_PURPLE : colors::COLOR_RADIANT_ORANGE;
    const Color secondary = !alternate ? colors::COLOR_DEEP_PURPLE : colors::COLOR_RADIANT_ORANGE;
    globalData[root.tree].styles[root.node].background_color = primary;
    globalData[root.tree].styles[score].background_color = secondary;
    globalData[root.tree].node_properties[score].text = std::format("{:05}", high_score.score);
}
} // namespace pcg::clickcore

void pcg::arcade::RunClickCore() {
    pce::Logger().Log("Running click core");
    clickcore::ClickCore click_core { };
    while (click_core.running) { click_core.Tick(); }
}
