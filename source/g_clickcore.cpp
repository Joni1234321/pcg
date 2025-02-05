#include "g_clickcore.hpp"

#include <chrono>

#include "m_frame.hpp"
#include "r_engine.hpp"
#include "r_ui.hpp"
#include "r_ui_node.hpp"
#include "u_collections.hpp"
#include "u_util.hpp"

namespace pcg::clickcore {
using namespace pce;
using namespace pce::frame;
using namespace std::chrono;

using Score = NamedType<u32, struct ScoreTag, Arithmetic, FormatLongNumber>;
struct HighScore {
    Score score;
    std::strong_ordering operator<=>(const HighScore& other) const noexcept { return score.Value() <=> other.score.Value(); }
};
struct RoundData {
    Score score { 0U };
    time_point<high_resolution_clock> start_time;
};
struct GameData {
    Multiset<HighScore> high_scores { };
};

struct MainMenuFrame {
    ui::NodeTree tree;
    explicit MainMenuFrame() {
        const String title = "Hey Helene!";
        const ui::NodeStyle::NodeHandle frame = ui::B(tree, ui::fill, { 100U, 30U }).Direction(ui::vertical).Build();
        ui::B(tree, frame, ui::hug).Text(title, ui::FontSizes::title).Fill(colors::light_sky_blue).Build();
        const ui::NodeStyle::NodeHandle root = ui::B(tree, frame, ui::hug).Padding2({ 20U, 5U }).Fill(colors::deep_purple).Center().Direction(ui::vertical).Build();
        start_button = ui::B(tree, root, ui::hug).Fill(colors::radiant_orange).Text(String { "Play" }, ui::FontSizes::h1).Build();
        settings_button = ui::B(tree, root, ui::hug).Fill(colors::cool_teal).Text(String { "Settings" }, ui::FontSizes::h1).Build();
        exit_button = ui::B(tree, root, ui::hug).Fill(colors::ruby_red).Text(String { "Exit" }, ui::FontSizes::h1).Build();
    }
    [[nodiscard]] constexpr ui::NodeStyle& StartButton() { return tree.GetNode(start_button.GetHandle()); }
    constexpr void SetStartButtonText(String&& string) { tree.GetNodeProperties(start_button.GetHandle()).text = string; }
    [[nodiscard]] constexpr ui::NodeStyle& SettingsButton() { return tree.GetNode(settings_button.GetHandle()); }
    [[nodiscard]] constexpr ui::NodeStyle& ExitButton() { return tree.GetNode(exit_button.GetHandle()); }

private:
    ui::NodeStyle::NodeHandleOptional start_button { };
    ui::NodeStyle::NodeHandleOptional settings_button { };
    ui::NodeStyle::NodeHandleOptional exit_button { };
};
struct GameFrame {
    ui::NodeTree tree;
    explicit GameFrame() {
        frame = ui::B(tree, ui::fill, { 0U, 0U }).Center().Direction(ui::vertical).Padding2(uint2 { 0U, 30U }).Build();
        ui::B(tree, frame.GetHandle(), ui::hug).Text("🎮 GAME TIME 🎮", ui::FontSizes::h1).Padding2(uint2 { 20U, 10U }).Fill(colors::navy_blue).Center().Build();
        score_box = ui::B(tree, frame.GetHandle(), ui::hug).Padding2(uint2 { 10U, 5U }).Fill(colors::forest_green).Direction(ui::vertical).Center().Build();
        time_label = ui::B(tree, score_box.GetHandle(), ui::hug).Text("Time: 00:00.00", ui::FontSizes::h2).Padding(5U).Fill(colors::light_gray).Build();
        score_label = ui::B(tree, score_box.GetHandle(), ui::hug).Text("Score: 0000", ui::FontSizes::h2).Padding(5U).Fill(colors::gold).Build();
        game_area = ui::B(tree, frame.GetHandle(), ui::fill).Padding2(uint2 { 300U, 100U }).Build();
        constexpr u32 box_size = 100U;
        box = ui::B(tree, game_area.GetHandle(), uint2 { box_size, box_size }).Fill(colors::ruby_red).Padding(5U).Build();
    }
    [[nodiscard]] constexpr ui::NodeStyle& Frame() { return tree.GetNode(frame.GetHandle()); }
    [[nodiscard]] constexpr ui::NodeStyle& GameArea() { return tree.GetNode(game_area.GetHandle()); }
    [[nodiscard]] constexpr ui::NodeStyle& Box() { return tree.GetNode(box.GetHandle()); }
    [[nodiscard]] constexpr ui::NodeStyle& ScoreBox() { return tree.GetNode(score_box.GetHandle()); }
    void SetTime(const u32 time_ms) { tree.GetNodeProperties(time_label.GetHandle()).text = std::format("Time {:02}:{:02}.{:02}", time_ms / (1000U * 60U), time_ms / 1000U % 60U, time_ms % 100U); }
    void SetScore(const u32 score) { tree.GetNodeProperties(score_label.GetHandle()).text = std::format("Score {:4}", score); }

private:
    ui::NodeStyle::NodeHandleOptional time_label { };
    ui::NodeStyle::NodeHandleOptional score_label { };
    ui::NodeStyle::NodeHandleOptional score_box { };
    ui::NodeStyle::NodeHandleOptional box { };
    ui::NodeStyle::NodeHandleOptional game_area { };
    ui::NodeStyle::NodeHandleOptional frame { };
};
struct HighScoreFrame {
    ui::NodeTree tree;
    void SetHighScore(Multiset<HighScore> scores) {
        tree.Clear();
        ui::NodeStyle::NodeHandle frame = ui::B(tree, ui::fill, uint2 { 0U, 0U }).Direction(ui::vertical).Center().Build();
        ui::NodeStyle::NodeHandle root = ui::B(tree, frame, ui::hug ).Direction(ui::vertical).Center().Build();
        ui::NodeStyle::NodeHandle title = ui::B(tree, root, ui::hug).Text("High Scores", ui::FontSizes::h1).Fill(colors::deep_purple).Build();
        bool alternate = false;
        for (const HighScore& high_score : scores | std::views::reverse) {
            const SDL_Color primary = alternate ? colors::deep_purple : colors::radiant_orange;
            const SDL_Color secondary = !alternate ? colors::deep_purple : colors::radiant_orange;
            ui::NodeStyle::NodeHandle row = ui::B(tree, root, {150U, ui::hug}).Fill(primary).Center().Build();
            ui::B(tree, row, ui::hug).Text(std::format("{:05}", high_score.score), ui::FontSizes::h2).Fill(secondary).Build();
            alternate = !alternate;
        }
    }
};

enum class Scene { game, main_menu, game_over, quit };
class ClickCore {
    RenderSystem& render_system;
    TickSystem tick_system { };
    InputSystem input_system { };
    ui::NodeRenderSystem node_render_system { render_system };

    RoundData game { };
    GameData game_data { };
    Scene scene { Scene::main_menu };

public:
    TickFrame tick_frame { };
    InspectorFrame debug_frame { };
    MainMenuFrame main_menu_frame { };
    GameFrame game_frame { };
    HighScoreFrame high_score_frame { };

    explicit ClickCore(RenderSystem& render_system);
    void Tick();
    [[nodiscard]] constexpr b8 IsRunning() const { return tick_system.running; }

private:
    void UserInterface();
    Scene MainMenuScene();
    Scene GameScene();
    Scene GameOverScene();
};

ClickCore::ClickCore(RenderSystem& render_system) : render_system { render_system } {
    node_render_system.node_trees.EmplaceBack(tick_frame.tree);
    node_render_system.node_trees.EmplaceBack(debug_frame.tree);
    node_render_system.node_trees.EmplaceBack(main_menu_frame.tree);
    node_render_system.node_trees.EmplaceBack(game_frame.tree);
    node_render_system.node_trees.EmplaceBack(high_score_frame.tree);
}
void ClickCore::Tick() {
    tick_system();
    input_system();
    node_render_system.HoverClickEvents(input_system);

    UserInterface();

    render_system();
    node_render_system.RenderTrees(render_system.renderer);
    render_system.End();
    tick_system.End();
}
void ClickCore::UserInterface() {
    if (input_system.keys[SDLK_ESCAPE]) { tick_system.running = false; }

    if (input_system.LeftMouseDown()) {
        debug_frame.tree.MarkDirty();
        debug_frame.ShowElementStructure(node_render_system.hovered);
    }
    tick_frame.tree.MarkDirty();
    tick_frame.SetInfo(tick_system.tick.Value(), 1.0F / tick_system.tick_time, 1.0F / tick_system.delta_time);

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
            return;
    }
}

Scene ClickCore::MainMenuScene() {
    main_menu_frame.tree.MarkDirty();
    if (input_system.LeftMouseDown()) {
        if (main_menu_frame.StartButton().IsInside(input_system.MousePosition())) {
            main_menu_frame.tree.SetDisplay(false);
            game = RoundData { .score = Score { 0U }, .start_time = high_resolution_clock::now() };
            return Scene::game;
        }
        if (main_menu_frame.SettingsButton().IsInside(input_system.MousePosition())) { return Scene::game_over; }
        if (main_menu_frame.ExitButton().IsInside(input_system.MousePosition())) { return Scene::quit; }
    }
    return Scene::main_menu;
}
Scene ClickCore::GameScene() {
    constexpr seconds game_time = 2s;
    const nanoseconds elapsed = high_resolution_clock::now() - game.start_time;
    if (elapsed > game_time) {
        game_data.high_scores.emplace(game.score);

        main_menu_frame.tree.MarkDirty();
        main_menu_frame.tree.SetDisplay(true);
        main_menu_frame.SetStartButtonText("Play Again");
        game_frame.tree.MarkDirty();
        game_frame.SetTime(0);
        game_frame.Box().background_color.a = 0U;
        high_score_frame.tree.MarkDirty();
        high_score_frame.SetHighScore(game_data.high_scores);
        high_score_frame.tree.SetDisplay(true);
        return Scene::game_over;
    }
    const u32 time_left_ms = duration_cast<milliseconds>(game_time - elapsed).count();
    game_frame.tree.MarkDirty();
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
    game_frame.tree.MarkDirty();
    game_frame.Box().background_color = colors::AnimateDamp(static_cast<f32>(tick_system.tick.Value()) / slow_down);
    game_frame.ScoreBox().background_color = colors::AnimateDamp(tick_system.tick.Value() * 1.2F / slow_down);

    const Scene scene = MainMenuScene();
    if (scene == Scene::main_menu) { return Scene::game_over; }
    if (scene == Scene::game) {
        game_frame.Box().background_color = colors::ruby_red;
        game_frame.ScoreBox().background_color = colors::forest_green;
        high_score_frame.tree.MarkDirty();
        high_score_frame.tree.SetDisplay(false);
        return Scene::game;
    }
    return scene;
}
void RunClickCore(RenderSystem& render_system) {
    ClickCore click_core { render_system };
    while (click_core.IsRunning()) { click_core.Tick(); }
}
}
