module;

#include <SDL3/SDL_keycode.h>
#include <SDL3/SDL_render.h>
#include <SDL3_ttf/SDL_ttf.h>

export module rail.editor;

import std;

import pce.std;
import pce.math;
import pce.sdl;
import pce.globals;
import pce.window_state;
import pce.collections;
import pce.colors;
import pce.assets;
import pce.font;

import pcs.input;
import pcs.camera;
import pcs.node;
import pcs.node_data;

import hex.hex;
import rail.types;
import rail.scenarios;

using namespace hex;
using namespace hex::ui;

export namespace rail {
constexpr const char* SCENARIOS_BASE_DIR = "rail/scenarios_base";
constexpr const char* SCENARIOS_DIR = "rail/scenarios";
constexpr const char* IMPORT_IMAGE = "rail/source/britain.jpg";
constexpr uint2 IMPORT_MAP_SIZE { 136U, 240U };
constexpr f32 EDITOR_CAMERA_SCALE = 8.0F;
constexpr u32 HISTORY_MAX = 64U;
constexpr i32 BRUSH_ELEVATION_STEP = 8;
constexpr u32 OVERLAY_YEAR_MIN = 1830U;
constexpr u32 OVERLAY_YEAR_MAX = 2020U;
constexpr u32 OVERLAY_YEAR_STEP = 10U;
constexpr u32 SLIDER_TRACK_WIDTH = 200U;
constexpr u32 SLIDER_KNOB_WIDTH = 14U;
constexpr u32 SLIDER_HEIGHT = 24U;
constexpr Color COLOR_SEA_SHALLOW { 140U, 190U, 230U };
constexpr Color COLOR_SEA_DEEP { 40U, 80U, 150U };
constexpr Color COLOR_LOWLAND { 110U, 160U, 90U };
constexpr Color COLOR_UPLAND { 190U, 190U, 120U };
constexpr Color COLOR_HIGHLAND { 140U, 100U, 60U };
constexpr Color COLOR_MOUNTAIN { 160U, 160U, 160U };
constexpr f32 UPLAND_T = 0.85F;
constexpr f32 HIGHLAND_T = 0.95F;
constexpr Color COLOR_CITY { 40U, 40U, 40U };
constexpr Color COLOR_CITY_SELECTED { 200U, 60U, 40U };
constexpr Color COLOR_BUTTON { colors::COLOR_LIGHT_GRAY };
constexpr Color COLOR_BUTTON_HOVER { colors::COLOR_WHITE };
constexpr Color COLOR_BUTTON_TEXT { colors::COLOR_BLACK };
constexpr Color COLOR_BUTTON_TEXT_INACTIVE { colors::COLOR_GRAY };

enum class EditorTool : u8 { TOOL_TERRAIN, TOOL_CITY };

[[nodiscard]] constexpr Color ElevationToColor(const i8 elevation) {
    if (elevation < 0) { return colors::ColorLerp(COLOR_SEA_SHALLOW, COLOR_SEA_DEEP, static_cast<f32>(-1 - elevation) / static_cast<f32>(-1 - ELEVATION_MIN)); }
    const f32 t = static_cast<f32>(elevation) / ELEVATION_MAX;
    if (t < UPLAND_T) { return colors::ColorLerp(COLOR_LOWLAND, COLOR_UPLAND, t / UPLAND_T); }
    if (t < HIGHLAND_T) { return colors::ColorLerp(COLOR_UPLAND, COLOR_HIGHLAND, (t - UPLAND_T) / (HIGHLAND_T - UPLAND_T)); }
    return colors::ColorLerp(COLOR_HIGHLAND, COLOR_MOUNTAIN, (t - HIGHLAND_T) / (1.0F - HIGHLAND_T));
}

[[nodiscard]] Handle<Node> Button(const NodeReference parent, const String& text) {
    const Handle<Node> button = NodeBuilder(parent, Layout { hug }).Padding(4U).Fill(COLOR_BUTTON).FillHover(COLOR_BUTTON_HOVER).Build();
    (void)NodeBuilder(NodeReference { parent.tree, button }, Layout { hug }).Text(text, COLOR_BUTTON_TEXT).Build();
    return button;
}
void SetButtonTextColor(const Handle<NodeTree> tree, const Handle<Node> button, const Color color) { globalData[tree].styles[globalData[tree].children[button][0]].background_color = color; }

struct RailEditorFrame : Frame {
    Handle<Node> root { B(frame).Node(hug).Gap(6U).Direction(vertical).Build() };
    Handle<Node> toolbar { B(root).Node(hug).Padding(8U).Gap(16U).Fill(colors::COLOR_BEIGE).Build() };
    Handle<Node> history_group { B(toolbar).Node(hug).Gap(4U).Build() };
    Handle<Node> undo_button { Button(B(history_group).parent, "↶") };
    Handle<Node> redo_button { Button(B(history_group).parent, "↷") };
    Handle<Node> tool_group { B(toolbar).Node(hug).Gap(4U).Build() };
    Handle<Node> terrain_tool_button { Button(B(tool_group).parent, "▲ Terrain") };
    Handle<Node> city_tool_button { Button(B(tool_group).parent, "● City") };
    Handle<Node> brush_group { B(toolbar).Node(hug).Gap(4U).Build() };
    Handle<Node> brush_smaller { Button(B(brush_group).parent, "−") };
    Handle<Node> brush_label { B(brush_group).Node(hug).Padding(4U).Text(FontSizes::body, colors::COLOR_BLACK).Build() };
    Handle<Node> brush_bigger { Button(B(brush_group).parent, "+") };
    Handle<Node> overlay_group { B(toolbar).Node(hug).Gap(4U).Build() };
    Handle<Node> year_previous { Button(B(overlay_group).parent, "◂") };
    Handle<Node> year_track { B(overlay_group).Node(SLIDER_TRACK_WIDTH, SLIDER_HEIGHT).Fill(colors::COLOR_GRAY).Build() };
    Handle<Node> year_knob { B(year_track).Node(SLIDER_KNOB_WIDTH, SLIDER_HEIGHT).Fill(colors::COLOR_BLACK).Build() };
    Handle<Node> year_label { B(overlay_group).Node(hug).Padding(4U).Text(FontSizes::body, colors::COLOR_BLACK).Build() };
    Handle<Node> year_next { Button(B(overlay_group).parent, "▸") };
    Handle<Node> file_group { B(toolbar).Node(hug).Gap(4U).Build() };
    Handle<Node> import_button { Button(B(file_group).parent, "▦ Import image") };
    Handle<Node> save_button { Button(B(file_group).parent, "⬇ Save") };
    Handle<Node> file_panel { B(root).Node(hug).Padding(8U).Gap(4U).Direction(vertical).Fill(colors::COLOR_BEIGE).Build() };
    Handle<Node> help_label { B(file_panel).Node(hug).Text(FontSizes::small, colors::COLOR_DARK_GRAY).Build() };
    Handle<Node> status_label { B(file_panel).Node(hug).Text(FontSizes::small, colors::COLOR_DARK_GRAY).Build() };
    Handle<Node> file_list { B(file_panel).Node(hug).Gap(2U).Direction(vertical).Build() };
    RailEditorFrame() {
        globalData[tree].styles[frame].alignment = top_right;
        globalData[tree].styles[root].alignment = top_right;
    }
};

struct EditorDocument {
    HexList<i8> elevation { };
    std::map<u32, std::vector<City>> city_overlays { };
};

struct RailEditorSystem {
    EditorDocument document { };
    u32 overlay_year { OVERLAY_YEAR_MIN };
    b8 slider_dragging { false };
    List<Vertex> verts { };
    List<Label> city_labels { };
    EditorTool tool { EditorTool::TOOL_TERRAIN };
    u32 brush_radius { 1U };
    Optional<int2> last_painted_axial { };
    Optional<u32> selected_city { };
    List<EditorDocument> undo_history { };
    List<EditorDocument> redo_history { };
    RailEditorFrame frame { };

    RailEditorSystem() {
        NodeTree& tree = globalData[frame.tree];
        tree.node_properties[frame.undo_button].on_click = [this](NodeReference) { Undo(); };
        tree.node_properties[frame.redo_button].on_click = [this](NodeReference) { Redo(); };
        tree.node_properties[frame.terrain_tool_button].on_click = [this](NodeReference) { SetTool(EditorTool::TOOL_TERRAIN); };
        tree.node_properties[frame.city_tool_button].on_click = [this](NodeReference) { SetTool(EditorTool::TOOL_CITY); };
        tree.node_properties[frame.brush_smaller].on_click = [this](NodeReference) { SetBrushRadius(brush_radius - 1U); };
        tree.node_properties[frame.brush_bigger].on_click = [this](NodeReference) { SetBrushRadius(brush_radius + 1U); };
        tree.node_properties[frame.year_previous].on_click = [this](NodeReference) { SetOverlayYear(overlay_year - OVERLAY_YEAR_STEP); };
        tree.node_properties[frame.year_next].on_click = [this](NodeReference) { SetOverlayYear(overlay_year + OVERLAY_YEAR_STEP); };
        tree.node_properties[frame.import_button].on_click = [this](NodeReference) {
            PushHistory();
            SetDocument(EditorDocument { .elevation = ElevationFromImage(IMPORT_IMAGE, IMPORT_MAP_SIZE) });
        };
        tree.node_properties[frame.save_button].on_click = [this](NodeReference) {
            for (u32 number = 1U;; number++) {
                const AssetPath asset_path = std::format("{}/map_{:03}.txt", SCENARIOS_DIR, number);
                if (std::filesystem::exists(Asset(asset_path))) { continue; }
                std::filesystem::create_directories(Asset(SCENARIOS_DIR));
                TerrainSave(document.elevation, asset_path);
                for (const auto& [year, cities] : document.city_overlays) {
                    if (!cities.empty()) { CitiesSave(cities, std::format("{}/map_{:03}_cities_{}.txt", SCENARIOS_DIR, number, year)); }
                }
                AddFileButton(asset_path);
                SetStatus(std::format("Saved {}", asset_path.string()));
                return;
            }
        };
        for (const char* dir : { SCENARIOS_BASE_DIR, SCENARIOS_DIR }) {
            if (!std::filesystem::exists(Asset(dir))) { continue; }
            std::vector<AssetPath> asset_paths;
            for (const std::filesystem::directory_entry& entry : std::filesystem::directory_iterator(Asset(dir))) {
                if (!entry.path().filename().string().contains("_cities_")) { asset_paths.push_back(AssetPath { dir } / entry.path().filename()); }
            }
            std::ranges::sort(asset_paths);
            for (const AssetPath& asset_path : asset_paths) { AddFileButton(asset_path); }
        }
        SetBrushRadius(brush_radius);
        SetTool(tool);
        SetDocument(LoadDocument(AssetPath { SCENARIOS_BASE_DIR } / "britain.txt"));
        SetOverlayYear(overlay_year);
        UpdateHistoryButtons();
    }

    [[nodiscard]] static EditorDocument LoadDocument(const AssetPath& terrain_path) {
        EditorDocument loaded { .elevation = TerrainLoad(terrain_path) };
        const std::string overlay_prefix = terrain_path.stem().string() + "_cities_";
        for (const std::filesystem::directory_entry& entry : std::filesystem::directory_iterator(Asset(terrain_path).parent_path())) {
            const std::string filename = entry.path().stem().string();
            if (!filename.starts_with(overlay_prefix)) { continue; }
            const u32 year = static_cast<u32>(std::stoul(filename.substr(overlay_prefix.size())));
            loaded.city_overlays[year] = CitiesLoad(terrain_path.parent_path() / entry.path().filename());
        }
        return loaded;
    }

    [[nodiscard]] std::vector<City>& Cities() { return document.city_overlays[overlay_year]; }

    void SetOverlayYear(const u32 year) {
        overlay_year = std::clamp(year / OVERLAY_YEAR_STEP * OVERLAY_YEAR_STEP, OVERLAY_YEAR_MIN, OVERLAY_YEAR_MAX);
        NodeTree& tree = globalData[frame.tree];
        tree.node_properties[frame.year_label].text = std::format("{} ({} cities)", overlay_year, Cities().size());
        tree.styles[frame.year_track].padding.x = (overlay_year - OVERLAY_YEAR_MIN) * (SLIDER_TRACK_WIDTH - SLIDER_KNOB_WIDTH) / (OVERLAY_YEAR_MAX - OVERLAY_YEAR_MIN);
        tree.MarkDirty();
        SelectCity(std::nullopt);
        RebuildCityLabels();
    }

    void AddFileButton(const AssetPath& asset_path) {
        const Handle<Node> button = Button(NodeReference { frame.tree, frame.file_list }, std::format("▸ {}/{}", asset_path.parent_path().filename().string(), asset_path.filename().string()));
        globalData[frame.tree].node_properties[button].on_click = [this, asset_path](NodeReference) {
            PushHistory();
            SetDocument(LoadDocument(asset_path));
            SetStatus(std::format("Loaded {}", asset_path.string()));
        };
    }

    void SetStatus(String&& status) {
        globalData[frame.tree].node_properties[frame.status_label].text = std::move(status);
        globalData[frame.tree].MarkDirty();
    }

    void SetTool(const EditorTool new_tool) {
        tool = new_tool;
        SelectCity(std::nullopt);
        SetButtonTextColor(frame.tree, frame.terrain_tool_button, tool == EditorTool::TOOL_TERRAIN ? COLOR_BUTTON_TEXT : COLOR_BUTTON_TEXT_INACTIVE);
        SetButtonTextColor(frame.tree, frame.city_tool_button, tool == EditorTool::TOOL_CITY ? COLOR_BUTTON_TEXT : COLOR_BUTTON_TEXT_INACTIVE);
        globalData[frame.tree].node_properties[frame.help_label].text = tool == EditorTool::TOOL_TERRAIN ? "Left: raise   Right: lower   Ctrl+drag: pan" : "Left: add / select city   Right: remove city";
        globalData[frame.tree].MarkDirty();
    }

    void SetBrushRadius(const u32 new_brush_radius) {
        brush_radius = std::clamp(new_brush_radius, 1U, 8U);
        globalData[frame.tree].node_properties[frame.brush_label].text = std::format("Brush {}", brush_radius);
        globalData[frame.tree].MarkDirty();
    }

    void UpdateHistoryButtons() {
        SetButtonTextColor(frame.tree, frame.undo_button, undo_history.empty() ? COLOR_BUTTON_TEXT_INACTIVE : COLOR_BUTTON_TEXT);
        SetButtonTextColor(frame.tree, frame.redo_button, redo_history.empty() ? COLOR_BUTTON_TEXT_INACTIVE : COLOR_BUTTON_TEXT);
        globalData[frame.tree].MarkDirty();
    }

    void PushHistory() {
        if (undo_history.size() >= HISTORY_MAX) { undo_history.erase_at(0U); }
        undo_history.EmplaceBack(document);
        redo_history.clear();
        UpdateHistoryButtons();
    }

    void Undo() {
        if (undo_history.empty()) { return; }
        redo_history.EmplaceBack(std::move(document));
        SetDocument(std::move(undo_history.back()));
        undo_history.pop_back();
        UpdateHistoryButtons();
    }

    void Redo() {
        if (redo_history.empty()) { return; }
        undo_history.EmplaceBack(std::move(document));
        SetDocument(std::move(redo_history.back()));
        redo_history.pop_back();
        UpdateHistoryButtons();
    }

    void SetDocument(EditorDocument&& new_document) {
        document = std::move(new_document);
        SetOverlayYear(overlay_year);
        CameraState& camera = Singleton::Get<CameraState>();
        camera.map_world_min = { 0.0F, 0.0F };
        camera.map_world_max = HexAxialToWorld(HexOffsetToAxial(static_cast<int2>(document.elevation.map_size - uint2 { 1U, 1U })));
        const float2 screen_size { Singleton::Get<WindowState>().screen_size };
        camera.world_position = (camera.map_world_min + camera.map_world_max) * float2 { 0.5F * camera.scale } - screen_size * float2 { 0.5F };
    }

    void SelectCity(const Optional<u32> city_index) { selected_city = city_index; }

    void RebuildCityLabels() {
        city_labels.clear();
        TTF_Font* font = Singleton::Get<FontCollection>().GetFontNormalCourier(FontSizes::body);
        for (const City& city : Cities()) { city_labels.EmplaceBack(font, String { city.name.c_str() }); }
    }

    void operator()() {
        InputState& input = Singleton::Get<InputState>();
        CameraState& camera = Singleton::Get<CameraState>();
        const b8 ctrl = input.keys[SDLK_LCTRL];
        if (ctrl && input.keys_down[SDLK_Z]) { Undo(); }
        if (ctrl && input.keys_down[SDLK_U]) { Redo(); }

        const HoveredType& hovered = Singleton::Get<HoveredType>();
        const b8 slider_hovered = hovered.has_value() && hovered->tree.id == frame.tree.id && (hovered->node.id == frame.year_track.id || hovered->node.id == frame.year_knob.id);
        if (input.left_mouse_down && slider_hovered) { slider_dragging = true; }
        if (!input.left_mouse) { slider_dragging = false; }
        if (slider_dragging) {
            const SDL_FRect& track = globalData[frame.tree].styles[frame.year_track].bounding_box;
            const f32 t = math::Clamp((input.mouse_position.x - track.x - SLIDER_KNOB_WIDTH * 0.5F) / (track.w - SLIDER_KNOB_WIDTH), 0.0F, 1.0F);
            SetOverlayYear(OVERLAY_YEAR_MIN + math::Round(t * (OVERLAY_YEAR_MAX - OVERLAY_YEAR_MIN) / OVERLAY_YEAR_STEP) * OVERLAY_YEAR_STEP);
        }

        const int2 axial_hover = HexWorldToAxial(camera.ScreenToWorld(input.mouse_position));
        const b8 over_ui = hovered.has_value();
        const b8 left_click = !over_ui && !ctrl && input.left_mouse_down;
        const b8 right_click = !over_ui && !ctrl && input.right_mouse_down;
        std::vector<City>& cities = Cities();
        const auto city_at = [&cities](const int2 axial) -> Optional<u32> {
            for (u32 i = 0; i < cities.size(); i++) {
                if (cities[i].axial == axial) { return i; }
            }
            return std::nullopt;
        };

        switch (tool) {
            case EditorTool::TOOL_TERRAIN: {
                const b8 paint_raise = input.left_mouse && !ctrl;
                const b8 paint_lower = input.right_mouse && !ctrl;
                if (!over_ui && (paint_raise || paint_lower) && last_painted_axial != axial_hover) {
                    if (!last_painted_axial.has_value()) { PushHistory(); }
                    last_painted_axial = axial_hover;
                    const i32 radius = static_cast<i32>(brush_radius) - 1;
                    for (i32 dy = -radius; dy <= radius; dy++) {
                        for (i32 dx = -radius; dx <= radius; dx++) {
                            const int2 axial = axial_hover + int2 { dx, dy };
                            if (HexAxialDistance(axial, axial_hover) > static_cast<u32>(radius) || !document.elevation.Contains(axial)) { continue; }
                            i8& elevation_value = document.elevation[axial];
                            elevation_value = static_cast<i8>(std::clamp<i32>(elevation_value + (paint_raise ? BRUSH_ELEVATION_STEP : -BRUSH_ELEVATION_STEP), ELEVATION_MIN, ELEVATION_MAX));
                        }
                    }
                }
                if (!input.left_mouse && !input.right_mouse) { last_painted_axial.reset(); }
                break;
            }
            case EditorTool::TOOL_CITY: {
                if (left_click && document.elevation.Contains(axial_hover)) {
                    const Optional<u32> existing = city_at(axial_hover);
                    if (!existing.has_value()) {
                        PushHistory();
                        cities.push_back(City { .axial = axial_hover, .name = std::format("City {}", cities.size() + 1U) });
                        RebuildCityLabels();
                    }
                    SelectCity(existing.value_or(static_cast<u32>(cities.size() - 1U)));
                }
                if (right_click) {
                    if (const Optional<u32> existing = city_at(axial_hover); existing.has_value()) {
                        PushHistory();
                        cities.erase(cities.begin() + existing.value());
                        SelectCity(std::nullopt);
                        RebuildCityLabels();
                    }
                }
                break;
            }
        }

        const float2 screen_size { Singleton::Get<WindowState>().screen_size };
        const f32 hex_screen_radius = camera.scale * 0.95F;
        verts.clear();
        for (u32 i = 0; i < document.elevation.Size(); i++) {
            const int2 axial = document.elevation.IndexToAxial(i);
            const float2 screen = camera.WorldToScreen(HexAxialToWorld(axial));
            if (screen.x < -camera.scale || screen.y < -camera.scale || screen.x > screen_size.x + camera.scale || screen.y > screen_size.y + camera.scale) { continue; }
            const u32 brush_hover_radius = tool == EditorTool::TOOL_TERRAIN && !over_ui ? brush_radius - 1U : 0U;
            const Color color = HexAxialDistance(axial, axial_hover) <= brush_hover_radius ? ElevationToColor(document.elevation.data[i]).Mul(1.2F) : ElevationToColor(document.elevation.data[i]);
            for (u32 corner = 0; corner < HEX_CORNERS; corner++) {
                verts.EmplaceBack(screen, color);
                verts.EmplaceBack(screen + HEX_ANGLE[corner] * float2 { hex_screen_radius }, color);
                verts.EmplaceBack(screen + HEX_ANGLE[(corner + 1) % HEX_CORNERS] * float2 { hex_screen_radius }, color);
            }
        }
        for (u32 i = 0; i < cities.size(); i++) {
            const float2 screen = camera.WorldToScreen(HexAxialToWorld(cities[i].axial));
            const Color color = selected_city == i ? COLOR_CITY_SELECTED : COLOR_CITY;
            const f32 outer = camera.scale * 0.6F;
            const f32 inner = camera.scale * 0.4F;
            for (u32 corner = 0; corner < HEX_CORNERS; corner++) {
                const float2 outer_a = screen + HEX_ANGLE[corner] * float2 { outer };
                const float2 outer_b = screen + HEX_ANGLE[(corner + 1) % HEX_CORNERS] * float2 { outer };
                const float2 inner_a = screen + HEX_ANGLE[corner] * float2 { inner };
                const float2 inner_b = screen + HEX_ANGLE[(corner + 1) % HEX_CORNERS] * float2 { inner };
                verts.EmplaceBack(outer_a, color);
                verts.EmplaceBack(outer_b, color);
                verts.EmplaceBack(inner_b, color);
                verts.EmplaceBack(outer_a, color);
                verts.EmplaceBack(inner_b, color);
                verts.EmplaceBack(inner_a, color);
            }
        }
        (void)SDL_RenderGeometry(Singleton::Get<WindowState>().renderer, nullptr, verts);
        for (u32 i = 0; i < cities.size(); i++) {
            const float2 screen = camera.WorldToScreen(HexAxialToWorld(cities[i].axial));
            city_labels[i].SetColor(selected_city == i ? COLOR_CITY_SELECTED : COLOR_CITY);
            city_labels[i].Draw(screen + float2 { camera.scale * 0.7F, -static_cast<f32>(FontSizes::body) * 0.5F });
        }
    }
};
} // namespace rail
