#include "g_arcade.hpp"

#include "r_engine.hpp"
#include "r_ui.hpp"
#include "r_ui_node.hpp"
#include "u_collections.hpp"
#include "u_util.hpp"

namespace pcg::clickcore {
using namespace pce;

using Score = NamedType<u32, struct ScoreTag, Arithmetic, FormatLongNumber>;
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
struct MainMenuFrame {
    ui::NodeTree& tree;
    explicit MainMenuFrame();
    [[nodiscard]] constexpr ui::NodeStyle& StartButton() { return tree.GetStyle(start_button.GetHandle()); }
    constexpr void SetStartButtonText(String&& string) { tree.GetProperties(start_button.GetHandle()).text = string; }
    [[nodiscard]] constexpr ui::NodeStyle& SettingsButton() { return tree.GetStyle(settings_button.GetHandle()); }
    [[nodiscard]] constexpr ui::NodeStyle& ExitButton() { return tree.GetStyle(exit_button.GetHandle()); }

private:
    ui::NodeHandleOptional start_button { };
    ui::NodeHandleOptional settings_button { };
    ui::NodeHandleOptional exit_button { };
};
struct GameFrame {
    ui::NodeTree& tree;
    explicit GameFrame();
    [[nodiscard]] constexpr ui::NodeStyle& Frame() { return tree.GetStyle(frame.GetHandle()); }
    [[nodiscard]] constexpr ui::NodeStyle& GameArea() { return tree.GetStyle(game_area.GetHandle()); }
    [[nodiscard]] constexpr ui::NodeStyle& Box() { return tree.GetStyle(box.GetHandle()); }
    [[nodiscard]] constexpr ui::NodeStyle& ScoreBox() { return tree.GetStyle(score_box.GetHandle()); }
    void SetTime(const u32 time_ms) { tree.GetProperties(time_label.GetHandle()).text = std::format("Time {:02}:{:02}.{:02}", time_ms / (1000U * 60U), time_ms / 1000U % 60U, time_ms % 100U); }
    void SetScore(const u32 score) { tree.GetProperties(score_label.GetHandle()).text = std::format("Score {:4}", score); }

private:
    ui::NodeHandleOptional time_label { };
    ui::NodeHandleOptional score_label { };
    ui::NodeHandleOptional score_box { };
    ui::NodeHandleOptional box { };
    ui::NodeHandleOptional game_area { };
    ui::NodeHandleOptional frame { };
};
struct HighScoreFrame {
    ui::NodeTree& tree;
    HighScoreFrame() : tree { ui::NodeRenderSystem::node_trees.EmplaceBack() } { }
    void SetHighScore(Multiset<HighScore> scores);
};
enum class Scene { game, main_menu, game_over, quit };
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
    RenderSystem render_system { };
    TickSystem tick_system { };
    InputSystem input_system { };
    ui::NodeRenderSystem node_render_system { };
    ui::DebugSystem debug_system { };

    ClickCoreFrames frames { };

    RoundData game { };
    GameData game_data { };
    Scene scene { Scene::main_menu };

public:
    ClickCore() { clear_color = colors::dark_slate; }
    [[nodiscard]] constexpr b8 IsRunning() const { return tick_system.running; }
    void Tick();

private:
    void GameLoop();
    Scene MainMenuScene();
    Scene GameScene();
    Scene GameOverScene();
};
constexpr MainMenuFrame& ClickCoreFrames::MainMenuFrame() {
    main_menu_frame.tree.MarkDirty();
    return main_menu_frame;
}
constexpr GameFrame& ClickCoreFrames::GameFrame() {
    game_frame.tree.MarkDirty();
    return game_frame;
}
constexpr HighScoreFrame& ClickCoreFrames::HighScoreFrame() {
    high_score_frame.tree.MarkDirty();
    return high_score_frame;
}
void ClickCore::Tick() {
    tick_system();
    input_system();
    debug_system(input_system, tick_system, node_render_system);
    node_render_system.HoverClickEvents(input_system);
    if (input_system.keys[SDLK_ESCAPE]) { tick_system.running = false; }

    GameLoop();

    render_system();
    node_render_system.RenderTrees(Window::renderer);
    render_system.Present();
    tick_system.CaptureTime();
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
            tick_system.running = false;
            break;
    }
}
Scene ClickCore::MainMenuScene() {
    if (input_system.LeftMouseDown()) {
        MainMenuFrame& main_menu_frame = frames.MainMenuFrame();
        if (main_menu_frame.StartButton().IsInside(input_system.MousePosition())) {
            main_menu_frame.tree.SetDisplay(false);
            game = RoundData { .score = Score { 0U }, .start_time = TimeNow() };
            return Scene::game;
        }
        if (main_menu_frame.SettingsButton().IsInside(input_system.MousePosition())) { return Scene::game_over; }
        if (main_menu_frame.ExitButton().IsInside(input_system.MousePosition())) { return Scene::quit; }
    }
    return Scene::main_menu;
}
Scene ClickCore::GameScene() {
    constexpr Seconds game_time = 20s;
    const Nanoseconds elapsed = TimeNow() - game.start_time;
    GameFrame& game_frame = frames.GameFrame();
    if (elapsed > game_time) {
        MainMenuFrame& main_menu_frame = frames.MainMenuFrame();
        HighScoreFrame& high_score_frame = frames.HighScoreFrame();

        game_data.high_scores.emplace(game.score);

        main_menu_frame.tree.SetDisplay(true);
        main_menu_frame.SetStartButtonText("Play Again");
        game_frame.SetTime(0);
        game_frame.Box().background_color.a = 0U;
        high_score_frame.SetHighScore(game_data.high_scores);
        high_score_frame.tree.SetDisplay(true);
        return Scene::game_over;
    }
    const u32 time_left_ms = duration_cast<Milliseconds>(game_time - elapsed).count();
    game_frame.SetScore(game.score.Value());
    game_frame.SetTime(time_left_ms);

    if (input_system.LeftMouseDown() || input_system.LeftMouseUp()) {
        if (game_frame.Box().IsInside(input_system.MousePosition())) {
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
    game_frame.Box().background_color = colors::AnimateDamp(static_cast<f32>(tick_system.tick.Value()) / slow_down);
    game_frame.ScoreBox().background_color = colors::AnimateDamp(tick_system.tick.Value() * 1.2F / slow_down);

    const Scene scene = MainMenuScene();
    if (scene == Scene::main_menu) { return Scene::game_over; }
    if (scene == Scene::game) {
        HighScoreFrame& high_score_frame = frames.HighScoreFrame();
        game_frame.Box().background_color = colors::ruby_red;
        game_frame.ScoreBox().background_color = colors::forest_green;
        high_score_frame.tree.SetDisplay(false);
        return Scene::game;
    }
    return scene;
}
GameFrame::GameFrame() : tree { ui::NodeRenderSystem::node_trees.EmplaceBack() } {
    frame = ui::B(tree, ui::fill, { 0U, 0U }).Center().Direction(ui::vertical).Padding2(uint2 { 0U, 30U }).Build();
    ui::B(tree, frame.GetHandle(), ui::hug).Text("🎮 GAME TIME 🎮", ui::FontSizes::h1).Padding2(uint2 { 20U, 10U }).Fill(colors::navy_blue).Center().Build();
    score_box = ui::B(tree, frame.GetHandle(), ui::hug).Padding2(uint2 { 10U, 5U }).Fill(colors::forest_green).Direction(ui::vertical).Center().Build();
    time_label = ui::B(tree, score_box.GetHandle(), ui::hug).Text("Time: 00:00.00", ui::FontSizes::h2).Padding(5U).Fill(colors::light_gray).Build();
    score_label = ui::B(tree, score_box.GetHandle(), ui::hug).Text("Score: 0000", ui::FontSizes::h2).Padding(5U).Fill(colors::gold).Build();
    game_area = ui::B(tree, frame.GetHandle(), ui::fill).Padding2(uint2 { 300U, 100U }).Build();
    constexpr u32 box_size = 100U;
    box = ui::B(tree, game_area.GetHandle(), uint2 { box_size, box_size }).Fill(colors::ruby_red).Padding(5U).Build();
}
MainMenuFrame::MainMenuFrame() : tree { ui::NodeRenderSystem::node_trees.EmplaceBack() } {
    const String title = "Hey Helene!";
    const ui::NodeHandle frame = ui::B(tree, ui::fill, { 100U, 30U }).Direction(ui::vertical).Build();
    ui::B(tree, frame, ui::hug).Text(title, ui::FontSizes::title).Fill(colors::light_sky_blue).Build();
    const ui::NodeHandle root = ui::B(tree, frame, ui::hug).Padding2({ 20U, 5U }).Fill(colors::deep_purple).Center().Direction(ui::vertical).Build();
    start_button = ui::B(tree, root, ui::hug).Fill(colors::radiant_orange).Text(String { "Play" }, ui::FontSizes::h1).Build();
    settings_button = ui::B(tree, root, ui::hug).Fill(colors::cool_teal).Text(String { "Settings" }, ui::FontSizes::h1).Build();
    exit_button = ui::B(tree, root, ui::hug).Fill(colors::ruby_red).Text(String { "Exit" }, ui::FontSizes::h1).Build();
}
void HighScoreFrame::SetHighScore(Multiset<HighScore> scores) {
    tree.Clear();
    ui::NodeHandle frame = ui::B(tree, ui::fill, uint2 { 0U, 0U }).Direction(ui::vertical).Center().Build();
    ui::NodeHandle root = ui::B(tree, frame, ui::hug).Direction(ui::vertical).Center().Build();
    ui::NodeHandle title = ui::B(tree, root, ui::hug).Text("High Scores", ui::FontSizes::h1).Fill(colors::deep_purple).Build();
    bool alternate = false;
    for (const HighScore& high_score : scores | std::views::reverse) {
        const SDL_Color primary = alternate ? colors::deep_purple : colors::radiant_orange;
        const SDL_Color secondary = !alternate ? colors::deep_purple : colors::radiant_orange;
        ui::NodeHandle row = ui::B(tree, root, { 150U, ui::hug }).Fill(primary).Center().Build();
        ui::B(tree, row, ui::hug).Text(std::format("{:05}", high_score.score), ui::FontSizes::h2).Fill(secondary).Build();
        alternate = !alternate;
    }
}
} // namespace pcg::clickcore

void pcg::arcade::RunClickCore() {
    clickcore::ClickCore click_core { };
    while (click_core.IsRunning()) { click_core.Tick(); }
}
