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
struct MainMenuFrame {
    Handle<NodeTree> tree_handle { data.Create<NodeTree>() };
    MainMenuFrame();
    [[nodiscard]] constexpr NodeStyle& StartButton() const { return data[tree_handle].node_styles[start_button.GetHandle()]; }
    constexpr void SetStartButtonText(String&& string) const { data[tree_handle].node_properties[start_button.GetHandle()].text = string; }
    [[nodiscard]] constexpr NodeStyle& SettingsButton() const { return data[tree_handle].node_styles[settings_button.GetHandle()]; }
    [[nodiscard]] constexpr NodeStyle& ExitButton() const { return data[tree_handle].node_styles[exit_button.GetHandle()]; }

private:
    HandleOptional<Node> start_button { };
    HandleOptional<Node> settings_button { };
    HandleOptional<Node> exit_button { };
};
struct GameFrame {
    Handle<NodeTree> tree_handle { data.Create<NodeTree>() };
    GameFrame();
    [[nodiscard]] constexpr NodeStyle& Frame() const { return data[tree_handle].node_styles[frame.GetHandle()]; }
    [[nodiscard]] constexpr NodeStyle& GameArea() const { return data[tree_handle].node_styles[game_area.GetHandle()]; }
    [[nodiscard]] constexpr NodeStyle& Box() const { return data[tree_handle].node_styles[box.GetHandle()]; }
    [[nodiscard]] constexpr NodeStyle& ScoreBox() const { return data[tree_handle].node_styles[score_box.GetHandle()]; }
    void SetTime(const u32 time_ms) const { data[tree_handle].node_properties[time_label.GetHandle()].text = std::format("Time {:02}:{:02}.{:02}", time_ms / (1000U * 60U), time_ms / 1000U % 60U, time_ms % 100U); }
    void SetScore(const u32 score) const { data[tree_handle].node_properties[score_label.GetHandle()].text = std::format("Score {:4}", score); }

private:
    HandleOptional<Node> time_label { };
    HandleOptional<Node> score_label { };
    HandleOptional<Node> score_box { };
    HandleOptional<Node> box { };
    HandleOptional<Node> game_area { };
    HandleOptional<Node> frame { };
};
struct HighScoreFrame {
    Handle<NodeTree> tree_handle { data.Create<NodeTree>() };
    HighScoreFrame() { }
    void SetHighScore(Multiset<HighScore> scores) const;
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
    data[main_menu_frame.tree_handle].MarkDirty();
    return main_menu_frame;
}
constexpr GameFrame& ClickCoreFrames::GameFrame() {
    data[game_frame.tree_handle].MarkDirty();
    return game_frame;
}
constexpr HighScoreFrame& ClickCoreFrames::HighScoreFrame() {
    data[high_score_frame.tree_handle].MarkDirty();
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
            data[main_menu_frame.tree_handle].SetDisplay(false);
            game = RoundData { .score = Score { 0U }, .start_time = TimeNow() };
            return Scene::game;
        }
        if (main_menu_frame.SettingsButton().IsInside(input_state.mouse_position)) { return Scene::game_over; }
        if (main_menu_frame.ExitButton().IsInside(input_state.mouse_position)) { return Scene::quit; }
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

        data[main_menu_frame.tree_handle].SetDisplay(true);
        main_menu_frame.SetStartButtonText("Play Again");
        game_frame.SetTime(0);
        game_frame.Box().background_color.a = 0U;
        high_score_frame.SetHighScore(game_data.high_scores);
        data[high_score_frame.tree_handle].SetDisplay(true);
        return Scene::game_over;
    }
    const u32 time_left_ms = duration_cast<Milliseconds>(game_time - elapsed).count();
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
        data[high_score_frame.tree_handle].SetDisplay(false);
        return Scene::game;
    }
    return scene;
}
GameFrame::GameFrame() {
    frame = B(tree_handle, fill, { 0U, 0U }).Center().Direction(vertical).Texture(data.Create<Texture>(Asset("rainforest.jpg"))).Padding2(uint2 { 0U, 30U }).Build();
    (void)B(tree_handle, frame.GetHandle(), hug).Text("🎮 GAME TIME 🎮", FontSizes::h1).Padding2(uint2 { 20U, 10U }).Fill(colors::navy_blue).Center().Build();
    score_box = B(tree_handle, frame.GetHandle(), hug).Padding2(uint2 { 10U, 5U }).Fill(colors::forest_green).Direction(vertical).Center().Build();
    time_label = B(tree_handle, score_box.GetHandle(), hug).Text("Time: 00:00.00", FontSizes::h2).Padding(5U).Fill(colors::light_gray).Build();
    score_label = B(tree_handle, score_box.GetHandle(), hug).Text("Score: 0000", FontSizes::h2).Padding(5U).Fill(colors::gold).Build();
    game_area = B(tree_handle, frame.GetHandle(), fill).Padding2(uint2 { 300U, 100U }).Build();
    constexpr u32 box_size = 100U;
    box = B(tree_handle, game_area.GetHandle(), uint2 { box_size, box_size }).Fill(colors::ruby_red).Texture(data.Create<Texture>(Asset("parrot.jpg"))).Padding(5U).Build();
}
MainMenuFrame::MainMenuFrame() {
    const String title = "Hey Helene!";
    const Handle<Node> frame = B(tree_handle, fill, { 100U, 30U }).Direction(vertical).Build();
    B(tree_handle, frame, hug).Text(title, FontSizes::title).Fill(colors::light_sky_blue).Build();
    const Handle<Node> root = B(tree_handle, frame, hug).Padding2({ 20U, 5U }).Fill(colors::deep_purple).Center().Direction(vertical).Build();
    start_button = B(tree_handle, root, hug).Fill(colors::radiant_orange).Text(String { "Play" }, FontSizes::h1).Build();
    settings_button = B(tree_handle, root, hug).Fill(colors::cool_teal).Text(String { "Settings" }, FontSizes::h1).Build();
    exit_button = B(tree_handle, root, hug).Fill(colors::ruby_red).Text(String { "Exit" }, FontSizes::h1).Build();
}
void HighScoreFrame::SetHighScore(Multiset<HighScore> scores) const {
    data[tree_handle].Clear();
    Handle<Node> frame = B(tree_handle, fill, uint2 { 0U, 0U }).Direction(vertical).Center().Build();
    Handle<Node> root = B(tree_handle, frame, hug).Direction(vertical).Center().Build();
    Handle<Node> title = B(tree_handle, root, hug).Text("High Scores", FontSizes::h1).Fill(colors::deep_purple).Build();
    bool alternate = false;
    for (const HighScore& high_score : scores | std::views::reverse) {
        const SDL_Color primary = alternate ? colors::deep_purple : colors::radiant_orange;
        const SDL_Color secondary = !alternate ? colors::deep_purple : colors::radiant_orange;
        Handle<Node> row = B(tree_handle, root, { 150U, hug }).Fill(primary).Center().Build();
        B(tree_handle, row, hug).Text(std::format("{:05}", high_score.score), FontSizes::h2).Fill(secondary).Build();
        alternate = !alternate;
    }
}
} // namespace pcg::clickcore

void pcg::arcade::RunClickCore() {
    clickcore::ClickCore click_core { };
    while (click_core.running) { click_core.Tick(); }
}
