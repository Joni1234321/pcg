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
import rail.defines;

using namespace hex;
using namespace hex::ui;

export namespace rail {
constexpr const char* SCENARIOS_BASE_DIR = "rail/scenarios_base";
constexpr const char* SCENARIOS_DIR = "rail/scenarios";
constexpr const char* IMPORT_IMAGE = "rail/scenario_source/britain.jpg";
constexpr const char* RAIL_TEXTURE = "rail/textures/track.png";
constexpr uint2 IMPORT_MAP_SIZE { 136U, 240U };
constexpr f32 EDITOR_CAMERA_SCALE = 20.0F;
constexpr f32 EDITOR_ZOOM_MIN = 0.5F;
constexpr f32 EDITOR_ZOOM_MAX = 200.0F;
constexpr f32 TERRAIN_TEXTURE_HEX_RADIUS = 2.0F;
constexpr f32 TERRAIN_TEXTURE_MAX_CAMERA_SCALE = 6.0F;
constexpr u32 TERRAIN_TEXTURE_HEXES_PER_DRAW = 16384U;
constexpr float2 TERRAIN_TEXTURE_WORLD_MARGIN { HEX_SPACING.x * 0.5F, 1.0F };
constexpr f32 MINIMAP_WIDTH = 300.0F;
constexpr f32 CITY_LABEL_MIN_CAMERA_SCALE = 2.0F;
constexpr FontSizes CITY_LABEL_FONT_SIZE = FontSizes::h4;
constexpr f32 MINIMAP_SCREEN_MARGIN = 10.0F;
constexpr Color COLOR_MINIMAP_BORDER { 30U, 30U, 30U };
constexpr Color COLOR_MINIMAP_VIEW { 255U, 255U, 255U };
constexpr u32 HISTORY_MAX = 64U;
constexpr i32 BRUSH_ELEVATION_STEP = 8;
constexpr u32 OVERLAY_YEAR_MIN = 1830U;
constexpr u32 OVERLAY_YEAR_MAX = 2020U;
constexpr u32 OVERLAY_YEAR_STEP = 10U;
constexpr u32 SLIDER_TRACK_WIDTH = 200U;
constexpr u32 SLIDER_KNOB_WIDTH = 14U;
constexpr u32 SLIDER_HEIGHT = 24U;
constexpr u32 RIVER_SIZE_MAX = 12U;
constexpr Color COLOR_RIVER_SMALL { 110U, 160U, 220U };
constexpr Color COLOR_RIVER_LARGE { 40U, 80U, 170U };
constexpr Color COLOR_WATER_LABEL { 30U, 60U, 120U };
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
constexpr Color COLOR_CITY_STAR { 240U, 200U, 40U };
constexpr Color COLOR_CITY_STAR_EMPTY { 40U, 40U, 40U, 50U };
constexpr u32 CITY_STAR_POINTS = 5U;
constexpr f32 CITY_STAR_INNER_RADIUS_RATIO = 0.45F;
constexpr f32 CITY_STAR_SCREEN_RADIUS = 10.0F;
constexpr f32 CITY_LABEL_SCREEN_GAP = 4.0F;
constexpr Color COLOR_BUTTON { colors::COLOR_LIGHT_GRAY };
constexpr Color COLOR_BUTTON_HOVER { colors::COLOR_WHITE };
constexpr Color COLOR_BUTTON_TEXT { colors::COLOR_BLACK };
constexpr Color COLOR_BUTTON_TEXT_INACTIVE { colors::COLOR_GRAY };
constexpr f32 RAIL_WIDTH_WORLD = 0.4F;
constexpr f32 RAIL_SNAP_RADIUS_WORLD = 1.0F;
constexpr u32 RAIL_SAVE_ELEVATION_UNITS_DEFAULT = 10U;
constexpr u32 RAIL_SAVE_ELEVATION_UNITS_MAX = 100U;
constexpr u32 RAIL_SAVE_ELEVATION_UNITS_STEP = 10U;
constexpr f32 RAIL_BRIDGE_CLEARANCE_MIN_ELEVATION_UNITS = 1.5F;
constexpr f32 RAIL_TUNNEL_DEPTH_MIN_ELEVATION_UNITS = 1.5F;
constexpr f32 RAIL_BRIDGE_DECK_WIDTH_WORLD = 0.7F;
constexpr f32 RAIL_TUNNEL_BRIGHTNESS = 0.4F;
constexpr Color COLOR_BRIDGE_DECK { 210U, 200U, 180U };
constexpr Color COLOR_TUNNEL_ROCK { 55U, 50U, 48U };
constexpr f32 RAIL_GRADE_STEEP = 0.04F;
constexpr f32 RAIL_SPEED_FACTOR_MIN = 0.1F;
constexpr f32 RAIL_PATH_TURN_COST_UNITS = 0.5F;
constexpr f32 RAIL_PATH_WATER_COST_UNITS = 2.0F;
constexpr i32 RAIL_PATH_MARGIN_UNITS = 48;
constexpr u32 RAIL_PATH_STATES_MAX = 8'000'000U;
constexpr Color COLOR_RAIL_GRADE_FLAT { 50U, 160U, 60U };
constexpr Color COLOR_RAIL_GRADE_STEEP { 220U, 40U, 30U };
constexpr FontSizes RAIL_GRADE_LABEL_FONT_SIZE = FontSizes::h4;
constexpr u32 RAIL_GRADE_LEVEL_MAX = 10U;
constexpr f32 RAIL_GRADE_PERCENT_PER_LEVEL = 1.0F;
constexpr f32 RAIL_GRADE_LABEL_MIN_CAMERA_SCALE = 48.0F;
constexpr f32 RAIL_GRADE_LABEL_OFFSET_WORLD = 0.6F;
constexpr f32 INDUSTRY_RADIUS_WORLD = 0.35F;
constexpr Array<Color, 6U> COLOR_BUILDINGS {
    Color { 120U, 80U, 50U }, Color { 160U, 110U, 40U }, Color { 90U, 100U, 130U }, Color { 190U, 150U, 60U }, Color { 200U, 200U, 210U }, Color { 210U, 90U, 90U },
};
[[nodiscard]] constexpr Color BuildingColor(const BuildingDefineId id) { return COLOR_BUILDINGS[id.value % COLOR_BUILDINGS.size()]; }
constexpr Array<Color, 7U> COLOR_INDUSTRIES {
    Color { 30U, 30U, 30U }, Color { 150U, 80U, 60U }, Color { 120U, 120U, 140U }, Color { 210U, 180U, 60U }, Color { 230U, 140U, 60U }, Color { 100U, 70U, 30U }, Color { 120U, 60U, 140U },
};
[[nodiscard]] constexpr Color IndustryColor(const IndustryDefineId id) { return COLOR_INDUSTRIES[id.value % COLOR_INDUSTRIES.size()]; }

enum class EditorTool : u8 { TOOL_TERRAIN, TOOL_CITY, TOOL_RAIL, TOOL_RAIL_PATH };

struct RailGradeLabelDraw {
    float2 screen;
    u32 level;
};
struct RailUnits {
    float2 world_step;
    f32 length_meters;
    u32 count;
};
[[nodiscard]] RailUnits RailUnitsOf(const Rail& rail) {
    const float2 world_direction = rail.world_b - rail.world_a;
    const f32 length_world = std::sqrt(math::Dot(world_direction, world_direction));
    const u32 count = math::Max(1U, static_cast<u32>(math::Round(length_world / RAIL_UNIT_WORLD)));
    return RailUnits { .world_step = world_direction * float2 { 1.0F / static_cast<f32>(count) }, .length_meters = length_world / static_cast<f32>(count) * WORLD_TO_METERS, .count = count };
}
struct RailProfile {
    std::vector<f32> terrain;
    std::vector<f32> rail;
};
void FillValleysDeeperThan(std::vector<f32>& elevations, const f32 depth_min) {
    const u32 count = static_cast<u32>(elevations.size());
    std::vector<f32> level(count);
    f32 rim = elevations.front();
    for (u32 i = 0; i < count; i++) {
        rim = math::Max(rim, elevations[i]);
        level[i] = rim;
    }
    rim = elevations.back();
    for (u32 i = count; i > 0; i--) {
        rim = math::Max(rim, elevations[i - 1]);
        level[i - 1] = math::Min(level[i - 1], rim);
    }
    for (u32 start = 0; start < count;) {
        u32 end = start;
        f32 depth = 0.0F;
        while (end < count && level[end] > elevations[end]) {
            depth = math::Max(depth, level[end] - elevations[end]);
            end++;
        }
        if (depth >= depth_min) {
            for (u32 i = start; i < end; i++) { elevations[i] = level[i]; }
        }
        start = math::Max(end, start + 1U);
    }
}

[[nodiscard]] constexpr f32 RailUnitTravelTime(const f32 grade) { return 1.0F / math::Max(RAIL_SPEED_FACTOR_MIN, 1.0F - math::Abs(grade) / RAIL_GRADE_STEEP); }

[[nodiscard]] constexpr Color RailGradeToColor(const f32 grade) { return colors::ColorLerp(COLOR_RAIL_GRADE_FLAT, COLOR_RAIL_GRADE_STEEP, math::Clamp(math::Abs(grade) / RAIL_GRADE_STEEP, 0.0F, 1.0F)); }

[[nodiscard]] constexpr Color ElevationToColor(const i8 elevation) {
    if (elevation < 0) { return colors::ColorLerp(COLOR_SEA_SHALLOW, COLOR_SEA_DEEP, static_cast<f32>(-1 - elevation) / static_cast<f32>(-1 - ELEVATION_MIN)); }
    const f32 t = static_cast<f32>(elevation) / ELEVATION_MAX;
    if (t < UPLAND_T) { return colors::ColorLerp(COLOR_LOWLAND, COLOR_UPLAND, t / UPLAND_T); }
    if (t < HIGHLAND_T) { return colors::ColorLerp(COLOR_UPLAND, COLOR_HIGHLAND, (t - UPLAND_T) / (HIGHLAND_T - UPLAND_T)); }
    return colors::ColorLerp(COLOR_HIGHLAND, COLOR_MOUNTAIN, (t - HIGHLAND_T) / (1.0F - HIGHLAND_T));
}

void AppendHex(List<Vertex>& verts, const float2 screen_center, const f32 screen_radius, const Color color) {
    for (u32 corner = 0; corner < HEX_CORNERS; corner++) {
        verts.EmplaceBack(screen_center, color);
        verts.EmplaceBack(screen_center + HEX_ANGLE[corner] * float2 { screen_radius }, color);
        verts.EmplaceBack(screen_center + HEX_ANGLE[(corner + 1) % HEX_CORNERS] * float2 { screen_radius }, color);
    }
}

void AppendSquare(List<Vertex>& verts, const float2 screen_center, const f32 screen_half_size, const f32 rotation, const Color color) {
    const float2 axis_x = float2 { math::Cos(rotation), math::Sin(rotation) } * float2 { screen_half_size };
    const float2 axis_y { -axis_x.y, axis_x.x };
    verts.EmplaceBack(screen_center - axis_x - axis_y, color);
    verts.EmplaceBack(screen_center + axis_x - axis_y, color);
    verts.EmplaceBack(screen_center + axis_x + axis_y, color);
    verts.EmplaceBack(screen_center - axis_x - axis_y, color);
    verts.EmplaceBack(screen_center + axis_x + axis_y, color);
    verts.EmplaceBack(screen_center - axis_x + axis_y, color);
}

void AppendStar(List<Vertex>& verts, const float2 screen_center, const f32 screen_radius, const f32 fill, const Color color) {
    const auto star_corner = [screen_center, screen_radius](const u32 corner) {
        const f32 angle = -math::PI * 0.5F - static_cast<f32>(corner) * math::PI / static_cast<f32>(CITY_STAR_POINTS);
        const f32 radius = corner % 2 == 0 ? screen_radius : screen_radius * CITY_STAR_INNER_RADIUS_RATIO;
        return screen_center + float2 { math::Cos(angle), math::Sin(angle) } * float2 { radius };
    };
    for (u32 corner = 0; corner < CITY_STAR_POINTS * 2; corner++) {
        const f32 wedge_fill = std::clamp(fill * static_cast<f32>(CITY_STAR_POINTS * 2) - static_cast<f32>(corner), 0.0F, 1.0F);
        if (wedge_fill == 0.0F) { return; }
        const float2 corner_a = star_corner(corner);
        const float2 corner_b = star_corner((corner + 1) % (CITY_STAR_POINTS * 2));
        verts.EmplaceBack(screen_center, color);
        verts.EmplaceBack(corner_a, color);
        verts.EmplaceBack(corner_a + (corner_b - corner_a) * float2 { wedge_fill }, color);
    }
}

[[nodiscard]] Handle<Node> Button(const NodeReference parent, const String& text, const FontSizes font_size = FontSizes::h4, const u32 padding = 4U) {
    const Handle<Node> button = NodeBuilder(parent, Layout { hug }).Padding(padding).Fill(COLOR_BUTTON).FillHover(COLOR_BUTTON_HOVER).Build();
    (void)NodeBuilder(NodeReference { parent.tree, button }, Layout { hug }).Text(text, font_size, COLOR_BUTTON_TEXT).Build();
    return button;
}
void SetButtonTextColor(const Handle<NodeTree> tree, const Handle<Node> button, const Color color) { globalData[tree].styles[globalData[tree].children[button][0]].background_color = color; }

struct Slider {
    Handle<Node> track;
    Handle<Node> knob;
    Handle<Node> label;
    u32 min;
    u32 max;
    u32 step;
    u32 value;
    b8 dragging { false };

    Slider(const NodeReference parent, const u32 min, const u32 max, const u32 step, const u32 value)
        : track { NodeBuilder(parent, Layout { uint2 { SLIDER_TRACK_WIDTH, SLIDER_HEIGHT } }).Fill(colors::COLOR_GRAY).Build() },
          knob { NodeBuilder(NodeReference { parent.tree, track }, Layout { uint2 { SLIDER_KNOB_WIDTH, SLIDER_HEIGHT } }).Fill(colors::COLOR_BLACK).Build() },
          label { NodeBuilder(parent, Layout { hug }).Padding(4U).Text(FontSizes::h4, colors::COLOR_BLACK).Build() },
          min { min }, max { max }, step { step }, value { value } { }

    void SetValue(NodeTree& tree, const u32 new_value, String&& text) {
        value = std::clamp(new_value / step * step, min, max);
        tree.node_properties[label].text = std::move(text);
        tree.styles[track].padding.x = (value - min) * (SLIDER_TRACK_WIDTH - SLIDER_KNOB_WIDTH) / (max - min);
        tree.MarkDirty();
    }

    [[nodiscard]] Optional<u32> Drag(const InputState& input, const HoveredType& hovered, const Handle<NodeTree> tree) {
        const b8 slider_hovered = hovered.has_value() && hovered->tree.id == tree.id && (hovered->node.id == track.id || hovered->node.id == knob.id);
        if (input.left_mouse_down && slider_hovered) { dragging = true; }
        if (!input.left_mouse) { dragging = false; }
        if (!dragging) { return std::nullopt; }
        const SDL_FRect& box = globalData[tree].styles[track].bounding_box;
        const f32 t = math::Clamp((input.mouse_position.x - box.x - SLIDER_KNOB_WIDTH * 0.5F) / (box.w - SLIDER_KNOB_WIDTH), 0.0F, 1.0F);
        return min + math::Round(t * (max - min) / step) * step;
    }
};

struct RailEditorFrame : Frame {
    Handle<Node> root { B(frame).Node(fill).Gap(6U).Direction(vertical).Build() };
    Handle<Node> file_panel { B(root).Node(hug, fill).Padding(8U).Gap(4U).Direction(vertical).Fill(colors::COLOR_BEIGE).Build() };
    Handle<Node> help_label { B(file_panel).Node(hug).Text(FontSizes::body, colors::COLOR_DARK_GRAY).Build() };
    Handle<Node> status_label { B(file_panel).Node(hug).Text(FontSizes::body, colors::COLOR_DARK_GRAY).Build() };
    Handle<Node> file_list { B(file_panel).Node(hug).Gap(1U).Direction(vertical).Build() };
    Handle<Node> rail_toolbar { B(root).Node(fill, hug).Padding(8U).Gap(16U).Fill(colors::COLOR_BEIGE).Build() };
    Handle<Node> tunnel_group { B(rail_toolbar).Node(hug).Gap(4U).Build() };
    Slider tunnel_slider { B(tunnel_group).parent, RAIL_SAVE_ELEVATION_UNITS_STEP, RAIL_SAVE_ELEVATION_UNITS_MAX, RAIL_SAVE_ELEVATION_UNITS_STEP, RAIL_SAVE_ELEVATION_UNITS_DEFAULT };
    Handle<Node> bridge_group { B(rail_toolbar).Node(hug).Gap(4U).Build() };
    Slider bridge_slider { B(bridge_group).parent, RAIL_SAVE_ELEVATION_UNITS_STEP, RAIL_SAVE_ELEVATION_UNITS_MAX, RAIL_SAVE_ELEVATION_UNITS_STEP, RAIL_SAVE_ELEVATION_UNITS_DEFAULT };
    Handle<Node> toolbar { B(root).Node(fill, hug).Padding(8U).Gap(16U).Fill(colors::COLOR_BEIGE).Build() };
    Handle<Node> history_group { B(toolbar).Node(hug).Gap(4U).Build() };
    Handle<Node> undo_button { Button(B(history_group).parent, "↶") };
    Handle<Node> redo_button { Button(B(history_group).parent, "↷") };
    Handle<Node> tool_group { B(toolbar).Node(hug).Gap(4U).Build() };
    Handle<Node> terrain_tool_button { Button(B(tool_group).parent, "▲ Terrain") };
    Handle<Node> city_tool_button { Button(B(tool_group).parent, "● City") };
    Handle<Node> rail_tool_button { Button(B(tool_group).parent, "━ Rail") };
    Handle<Node> rail_path_tool_button { Button(B(tool_group).parent, "⚡ Rail path") };
    Handle<Node> brush_group { B(toolbar).Node(hug).Gap(4U).Build() };
    Handle<Node> brush_smaller { Button(B(brush_group).parent, "−") };
    Handle<Node> brush_label { B(brush_group).Node(hug).Padding(4U).Text(FontSizes::h4, colors::COLOR_BLACK).Build() };
    Handle<Node> brush_bigger { Button(B(brush_group).parent, "+") };
    Handle<Node> overlay_group { B(toolbar).Node(hug).Gap(4U).Build() };
    Handle<Node> year_previous { Button(B(overlay_group).parent, "◂") };
    Slider year_slider { B(overlay_group).parent, OVERLAY_YEAR_MIN, OVERLAY_YEAR_MAX, OVERLAY_YEAR_STEP, OVERLAY_YEAR_MIN };
    Handle<Node> year_next { Button(B(overlay_group).parent, "▸") };
    Handle<Node> river_group { B(toolbar).Node(hug).Gap(4U).Build() };
    Slider river_slider { B(river_group).parent, 0U, RIVER_SIZE_MAX, 1U, 0U };
    Handle<Node> shading_button { Button(B(toolbar).parent, "⬡ Flat") };
    Handle<Node> file_group { B(toolbar).Node(hug).Gap(4U).Build() };
    Handle<Node> import_button { Button(B(file_group).parent, "▦ Import image") };
    Handle<Node> generate_button { Button(B(file_group).parent, "⚙ Generate") };
    Handle<Node> save_button { Button(B(file_group).parent, "⬇ Save") };
    RailEditorFrame() {
        globalData[tree].styles[frame].alignment = bottom_right;
        globalData[tree].styles[root].alignment = bottom_right;
    }
};

struct EditorDocument {
    HexList<i8> elevation { };
    std::vector<River> rivers { };
    std::vector<MapLabel> water_labels { };
    std::map<u32, std::vector<City>> city_overlays { };
    std::vector<Rail> rails { };
};

struct RailEditorSystem {
    EditorDocument document { };
    std::vector<IndustryDefine> industry_defines { RailIndustryDefines() };
    std::vector<BuildingDefine> building_defines { RailBuildingDefines() };
    Optional<Map> generated { };
    u32 overlay_year { OVERLAY_YEAR_MIN };
    u32 river_size_min { 0U };
    u32 rail_tunnel_save_units { RAIL_SAVE_ELEVATION_UNITS_DEFAULT };
    u32 rail_bridge_save_units { RAIL_SAVE_ELEVATION_UNITS_DEFAULT };
    List<Vertex> verts { };
    List<Label> city_labels { };
    List<Label> water_labels { };
    SDL_Texture* terrain_texture { nullptr };
    List<int2> terrain_texture_dirty_axials { };
    EditorTool tool { EditorTool::TOOL_RAIL };
    b8 terrain_smooth { false };
    u32 brush_radius { 1U };
    Optional<int2> last_painted_axial { };
    Optional<u32> selected_city { };
    Optional<float2> rail_drag_start_world { };
    Handle<Texture> rail_texture { globalData.Create<Texture>(Asset(RAIL_TEXTURE)) };
    List<Vertex> rail_verts { };
    List<Label> rail_grade_labels { };
    List<RailGradeLabelDraw> rail_grade_label_draws { };
    List<EditorDocument> undo_history { };
    List<EditorDocument> redo_history { };
    RailEditorFrame frame { };

    RailEditorSystem() {
        NodeTree& tree = globalData[frame.tree];
        tree.node_properties[frame.undo_button].on_click = [this](NodeReference) { Undo(); };
        tree.node_properties[frame.redo_button].on_click = [this](NodeReference) { Redo(); };
        tree.node_properties[frame.terrain_tool_button].on_click = [this](NodeReference) { SetTool(EditorTool::TOOL_TERRAIN); };
        tree.node_properties[frame.city_tool_button].on_click = [this](NodeReference) { SetTool(EditorTool::TOOL_CITY); };
        tree.node_properties[frame.rail_tool_button].on_click = [this](NodeReference) { SetTool(EditorTool::TOOL_RAIL); };
        tree.node_properties[frame.rail_path_tool_button].on_click = [this](NodeReference) { SetTool(EditorTool::TOOL_RAIL_PATH); };
        tree.node_properties[frame.brush_smaller].on_click = [this](NodeReference) { SetBrushRadius(brush_radius - 1U); };
        tree.node_properties[frame.brush_bigger].on_click = [this](NodeReference) { SetBrushRadius(brush_radius + 1U); };
        tree.node_properties[frame.year_previous].on_click = [this](NodeReference) { SetOverlayYear(overlay_year - OVERLAY_YEAR_STEP); };
        tree.node_properties[frame.year_next].on_click = [this](NodeReference) { SetOverlayYear(overlay_year + OVERLAY_YEAR_STEP); };
        tree.node_properties[frame.shading_button].on_click = [this](NodeReference) { SetTerrainSmooth(!terrain_smooth); };
        tree.node_properties[frame.import_button].on_click = [this](NodeReference) {
            PushHistory();
            SetDocument(EditorDocument { .elevation = ElevationFromImage(IMPORT_IMAGE, IMPORT_MAP_SIZE) });
        };
        tree.node_properties[frame.generate_button].on_click = [this](NodeReference) {
            generated = MapGenerate(MapDefine { .elevation = document.elevation, .water_labels = document.water_labels, .rivers = document.rivers, .cities = Cities() }, industry_defines, building_defines, RailCityGrowthSequence());
            SetStatus(std::format("Generated {} buildings, {} industries", generated->buildings.size(), generated->industries.size()));
        };
        tree.node_properties[frame.save_button].on_click = [this](NodeReference) {
            for (u32 number = 1U;; number++) {
                const AssetPath asset_path = std::format("{}/map_{:03}.txt", SCENARIOS_DIR, number);
                if (std::filesystem::exists(Asset(asset_path))) { continue; }
                std::filesystem::create_directories(Asset(SCENARIOS_DIR));
                TerrainSave(document.elevation, asset_path);
                if (!document.rivers.empty()) { RiversSave(document.rivers, std::format("{}/map_{:03}_rivers.txt", SCENARIOS_DIR, number)); }
                if (!document.water_labels.empty()) { WaterLabelsSave(document.water_labels, std::format("{}/map_{:03}_water_labels.txt", SCENARIOS_DIR, number)); }
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
                const std::string filename = entry.path().filename().string();
                if (!filename.contains("_cities_") && !filename.ends_with("_rivers.txt") && !filename.ends_with("_water_labels.txt")) { asset_paths.push_back(AssetPath { dir } / entry.path().filename()); }
            }
            std::ranges::sort(asset_paths);
            for (const AssetPath& asset_path : asset_paths) { AddFileButton(asset_path); }
        }
        TTF_Font* rail_grade_font = Singleton::Get<FontCollection>().GetFontBoldCourier(RAIL_GRADE_LABEL_FONT_SIZE);
        for (u32 level = 0; level <= RAIL_GRADE_LEVEL_MAX; level++) {
            rail_grade_labels.EmplaceBack(rail_grade_font, String { std::format("{}", level).c_str() });
        }
        SetBrushRadius(brush_radius);
        SetTool(tool);
        SetDocument(LoadDocument(AssetPath { SCENARIOS_BASE_DIR } / "britain_beginner.txt"));
        SetOverlayYear(overlay_year);
        SetRiverSizeMin(river_size_min);
        SetRailTunnelSave(rail_tunnel_save_units);
        SetRailBridgeSave(rail_bridge_save_units);
        UpdateHistoryButtons();
    }

    void SetRailTunnelSave(const u32 elevation_units) {
        frame.tunnel_slider.SetValue(globalData[frame.tree], elevation_units, std::format("Tunnel if it saves ≥ {}", elevation_units));
        rail_tunnel_save_units = frame.tunnel_slider.value;
    }

    void SetRailBridgeSave(const u32 elevation_units) {
        frame.bridge_slider.SetValue(globalData[frame.tree], elevation_units, std::format("Bridge if it saves ≥ {}", elevation_units));
        rail_bridge_save_units = frame.bridge_slider.value;
    }

    void SetRiverSizeMin(const u32 size) {
        frame.river_slider.SetValue(globalData[frame.tree], size, std::format("Rivers ≥ {}", size));
        river_size_min = frame.river_slider.value;
    }

    ~RailEditorSystem() {
        if (terrain_texture) { SDL_DestroyTexture(terrain_texture); }
    }

    [[nodiscard]] static float2 TerrainTexturePixel(const float2 world) { return (world + TERRAIN_TEXTURE_WORLD_MARGIN) * float2 { TERRAIN_TEXTURE_HEX_RADIUS }; }
    [[nodiscard]] float2 TerrainTextureSize() const { return TerrainTexturePixel(HexAxialToWorld(HexOffsetToAxial(static_cast<int2>(document.elevation.map_size - uint2 { 1U, 1U }))) + TERRAIN_TEXTURE_WORLD_MARGIN); }

    void RebuildTerrainTexture() {
        if (terrain_texture) { SDL_DestroyTexture(terrain_texture); }
        const float2 size = TerrainTextureSize();
        terrain_texture = SDL_CreateTexture(Singleton::Get<WindowState>().renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_TARGET, static_cast<i32>(std::ceil(size.x)), static_cast<i32>(std::ceil(size.y)));
        terrain_texture_dirty_axials.clear();
        for (u32 i = 0; i < document.elevation.Size(); i++) { terrain_texture_dirty_axials.EmplaceBack(document.elevation.IndexToAxial(i)); }
        FlushTerrainTexture();
    }

    void SetTerrainSmooth(const b8 smooth) {
        terrain_smooth = smooth;
        NodeTree& tree = globalData[frame.tree];
        tree.node_properties[tree.children[frame.shading_button][0]].text = smooth ? "◈ Smooth" : "⬡ Flat";
        tree.MarkDirty();
        RebuildTerrainTexture();
    }

    void AppendTerrainHex(const float2 screen_center, const f32 screen_radius, const int2 axial, const f32 brightness = 1.0F) {
        const Color center_color = ElevationToColor(document.elevation[axial]).Mul(brightness);
        if (!terrain_smooth) {
            AppendHex(verts, screen_center, screen_radius, center_color);
            return;
        }
        Array<Color, HEX_CORNERS> corner_colors;
        for (u32 corner = 0; corner < HEX_CORNERS; corner++) { corner_colors[corner] = ElevationToColor(static_cast<i8>(CornerElevation(axial, corner))).Mul(brightness); }
        for (u32 corner = 0; corner < HEX_CORNERS; corner++) {
            verts.EmplaceBack(screen_center, center_color);
            verts.EmplaceBack(screen_center + HEX_ANGLE[corner] * float2 { screen_radius }, corner_colors[corner]);
            verts.EmplaceBack(screen_center + HEX_ANGLE[(corner + 1) % HEX_CORNERS] * float2 { screen_radius }, corner_colors[(corner + 1) % HEX_CORNERS]);
        }
    }

    [[nodiscard]] f32 CornerElevation(const int2 axial, const u32 corner) const {
        i32 elevation_sum = document.elevation[axial];
        i32 hex_count = 1;
        for (const int2 neighbour : { axial + HEX_AXIAL_NEIGHBOURS[corner], axial + HEX_AXIAL_NEIGHBOURS[(corner + HEX_CORNERS - 1U) % HEX_CORNERS] }) {
            if (!document.elevation.Contains(neighbour)) { continue; }
            elevation_sum += document.elevation[neighbour];
            hex_count++;
        }
        return static_cast<f32>(elevation_sum) / static_cast<f32>(hex_count);
    }

    [[nodiscard]] f32 ElevationAtWorld(const float2 world) const {
        const int2 axial = HexWorldToAxial(world);
        if (!document.elevation.Contains(axial)) { return 0.0F; }
        const float2 local = world - HexAxialToWorld(axial);
        const f32 angle_degrees = std::atan2(local.y, local.x) / math::DEG_2_RAD;
        const u32 corner = static_cast<u32>(((static_cast<i32>(std::floor((30.0F - angle_degrees) / 60.0F)) % static_cast<i32>(HEX_CORNERS)) + HEX_CORNERS) % HEX_CORNERS);
        const float2 corner_a = HEX_ANGLE[corner];
        const float2 corner_b = HEX_ANGLE[(corner + 1) % HEX_CORNERS];
        const f32 weight_a = math::Cross(local, corner_b) / math::Cross(corner_a, corner_b);
        const f32 weight_b = math::Cross(corner_a, local) / math::Cross(corner_a, corner_b);
        return (1.0F - weight_a - weight_b) * static_cast<f32>(document.elevation[axial]) + weight_a * CornerElevation(axial, corner) + weight_b * CornerElevation(axial, (corner + 1) % HEX_CORNERS);
    }

    [[nodiscard]] float2 SnapToRailEnd(const float2 world) const {
        float2 world_snapped = world;
        f32 distance_best = RAIL_SNAP_RADIUS_WORLD;
        for (const Rail& rail : document.rails) {
            for (const float2 world_end : { rail.world_a, rail.world_b }) {
                const float2 world_offset = world_end - world;
                const f32 distance = std::sqrt(math::Dot(world_offset, world_offset));
                if (distance >= distance_best) { continue; }
                distance_best = distance;
                world_snapped = world_end;
            }
        }
        return world_snapped;
    }

    [[nodiscard]] Optional<Rail> DragRail(const float2 world_hover) const {
        if (!rail_drag_start_world.has_value()) { return std::nullopt; }
        const float2 world_end = SnapToRailEnd(world_hover);
        const float2 world_direction = world_end - *rail_drag_start_world;
        const f32 length_world = std::sqrt(math::Dot(world_direction, world_direction));
        if (length_world == 0.0F) { return std::nullopt; }
        const f32 unit_count = math::Max(1.0F, static_cast<f32>(math::Round(length_world / RAIL_UNIT_WORLD)));
        const Rail rail { .world_a = *rail_drag_start_world, .world_b = world_end == world_hover ? *rail_drag_start_world + world_direction * float2 { unit_count * RAIL_UNIT_WORLD / length_world } : world_end };
        if (!document.elevation.Contains(HexWorldToAxial(rail.world_b))) { return std::nullopt; }
        return rail;
    }

    [[nodiscard]] static f32 RailDistanceWorld(const Rail& rail, const float2 world) {
        const float2 world_direction = rail.world_b - rail.world_a;
        const f32 t = math::Clamp(math::Dot(world - rail.world_a, world_direction) / math::Dot(world_direction, world_direction), 0.0F, 1.0F);
        const float2 world_offset = world - (rail.world_a + world_direction * float2 { t });
        return std::sqrt(math::Dot(world_offset, world_offset));
    }

    [[nodiscard]] std::vector<Rail> RailPath(const float2 world_start, const float2 world_goal) const {
        const auto fine_to_world = [](const int2 fine) { return HexAxialToWorld(fine) * float2 { 0.5F }; };
        const int2 fine_start = HexWorldToAxial(world_start * float2 { 2.0F });
        const int2 fine_goal = HexWorldToAxial(world_goal * float2 { 2.0F });
        const int2 box_min { std::min(fine_start.x, fine_goal.x) - RAIL_PATH_MARGIN_UNITS, std::min(fine_start.y, fine_goal.y) - RAIL_PATH_MARGIN_UNITS };
        const int2 box_max { std::max(fine_start.x, fine_goal.x) + RAIL_PATH_MARGIN_UNITS, std::max(fine_start.y, fine_goal.y) + RAIL_PATH_MARGIN_UNITS };
        const uint2 box_size { static_cast<u32>(box_max.x - box_min.x + 1), static_cast<u32>(box_max.y - box_min.y + 1) };
        const u32 node_count = box_size.x * box_size.y;
        if (node_count * HEX_CORNERS > RAIL_PATH_STATES_MAX) { return {}; }
        const auto node_of = [box_min, box_max, box_size](const int2 fine) -> Optional<u32> {
            if (fine.x < box_min.x || fine.y < box_min.y || fine.x > box_max.x || fine.y > box_max.y) { return std::nullopt; }
            return static_cast<u32>(fine.y - box_min.y) * box_size.x + static_cast<u32>(fine.x - box_min.x);
        };
        const auto fine_of = [box_min, box_size](const u32 node) { return int2 { box_min.x + static_cast<i32>(node % box_size.x), box_min.y + static_cast<i32>(node / box_size.x) }; };
        std::vector<f32> node_elevation(node_count, std::numeric_limits<f32>::quiet_NaN());
        const auto elevation_of = [this, &node_elevation, fine_to_world](const u32 node, const int2 fine) {
            if (std::isnan(node_elevation[node])) { node_elevation[node] = ElevationAtWorld(fine_to_world(fine)); }
            return node_elevation[node];
        };
        std::vector<f32> state_cost(node_count * HEX_CORNERS, std::numeric_limits<f32>::infinity());
        std::vector<u32> state_parent(node_count * HEX_CORNERS, std::numeric_limits<u32>::max());
        std::priority_queue<std::pair<f32, u32>, std::vector<std::pair<f32, u32>>, std::greater<>> queue;
        const u32 start_node = *node_of(fine_start);
        for (u32 direction = 0; direction < HEX_CORNERS; direction++) {
            state_cost[start_node * HEX_CORNERS + direction] = 0.0F;
            queue.emplace(static_cast<f32>(HexAxialDistance(fine_start, fine_goal)), start_node * HEX_CORNERS + direction);
        }
        while (!queue.empty()) {
            const auto [estimate, state] = queue.top();
            queue.pop();
            const u32 node = state / HEX_CORNERS;
            const u32 direction = state % HEX_CORNERS;
            const int2 fine = fine_of(node);
            if (estimate > state_cost[state] + static_cast<f32>(HexAxialDistance(fine, fine_goal))) { continue; }
            if (fine == fine_goal) {
                std::vector<int2> fines;
                for (u32 back = state; back != std::numeric_limits<u32>::max(); back = state_parent[back]) { fines.push_back(fine_of(back / HEX_CORNERS)); }
                std::ranges::reverse(fines);
                std::vector<Rail> rails;
                u32 run_start = 0;
                for (u32 i = 2; i <= fines.size(); i++) {
                    if (i < fines.size() && fines[i] - fines[i - 1] == fines[i - 1] - fines[i - 2]) { continue; }
                    rails.push_back(Rail { .world_a = fine_to_world(fines[run_start]), .world_b = fine_to_world(fines[i - 1]) });
                    run_start = i - 1;
                }
                if (rails.empty()) { return rails; }
                rails.front().world_a = world_start;
                rails.back().world_b = world_goal;
                return rails;
            }
            for (u32 next_direction = 0; next_direction < HEX_CORNERS; next_direction++) {
                if ((next_direction + HEX_CORNERS / 2) % HEX_CORNERS == direction) { continue; }
                const int2 next_fine = fine + HEX_AXIAL_NEIGHBOURS[next_direction];
                const Optional<u32> next_node = node_of(next_fine);
                const int2 next_axial = HexWorldToAxial(fine_to_world(next_fine));
                if (!next_node.has_value() || !document.elevation.Contains(next_axial)) { continue; }
                const f32 next_elevation = elevation_of(*next_node, next_fine);
                const f32 grade = (math::Max(next_elevation, 0.0F) - math::Max(elevation_of(node, fine), 0.0F)) * ELEVATION_UNIT_METERS / (RAIL_UNIT_WORLD * WORLD_TO_METERS);
                const f32 cost = state_cost[state] + RailUnitTravelTime(grade) + (next_elevation < 0.0F ? RAIL_PATH_WATER_COST_UNITS : 0.0F) + (next_direction == direction ? 0.0F : RAIL_PATH_TURN_COST_UNITS);
                const u32 next_state = *next_node * HEX_CORNERS + next_direction;
                if (cost >= state_cost[next_state]) { continue; }
                state_cost[next_state] = cost;
                state_parent[next_state] = state;
                queue.emplace(cost + static_cast<f32>(HexAxialDistance(next_fine, fine_goal)), next_state);
            }
        }
        return {};
    }

    [[nodiscard]] RailProfile RailProfileOf(const Rail& rail, const RailUnits& units) const {
        RailProfile profile { .terrain = std::vector<f32>(units.count + 1), .rail = std::vector<f32>(units.count + 1) };
        for (u32 i = 0; i <= units.count; i++) {
            profile.terrain[i] = ElevationAtWorld(rail.world_a + units.world_step * float2 { static_cast<f32>(i) });
            profile.rail[i] = -math::Max(profile.terrain[i], 0.0F);
        }
        FillValleysDeeperThan(profile.rail, static_cast<f32>(rail_tunnel_save_units));
        for (u32 i = 0; i <= units.count; i++) { profile.rail[i] = profile.terrain[i] < 0.0F ? profile.terrain[i] - static_cast<f32>(RAIL_SAVE_ELEVATION_UNITS_MAX) : -profile.rail[i]; }
        u32 bank_first = 0;
        while (bank_first < units.count && profile.terrain[bank_first] < 0.0F) { bank_first++; }
        u32 bank_last = units.count;
        while (bank_last > 0 && profile.terrain[bank_last] < 0.0F) { bank_last--; }
        if (profile.terrain[0] < 0.0F) { profile.rail[0] = math::Max(profile.rail[bank_first], 0.0F); }
        if (profile.terrain[units.count] < 0.0F) { profile.rail[units.count] = math::Max(profile.rail[bank_last], 0.0F); }
        FillValleysDeeperThan(profile.rail, static_cast<f32>(rail_bridge_save_units));
        for (f32& elevation : profile.rail) { elevation = math::Max(elevation, 0.0F); }
        return profile;
    }

    template <typename GradeToColor> void AppendRail(const Rail& rail, const CameraState& camera, const GradeToColor& grade_to_color) {
        const RailUnits units = RailUnitsOf(rail);
        const RailProfile profile = RailProfileOf(rail, units);
        const float2 world_half_width = float2 { -units.world_step.y, units.world_step.x } * float2 { RAIL_WIDTH_WORLD * 0.5F / std::sqrt(math::Dot(units.world_step, units.world_step)) };
        const float2 world_label_offset = world_half_width * float2 { RAIL_GRADE_LABEL_OFFSET_WORLD / (RAIL_WIDTH_WORLD * 0.5F) };
        const float2 world_deck_half_width = world_half_width * float2 { RAIL_BRIDGE_DECK_WIDTH_WORLD / RAIL_WIDTH_WORLD };
        for (u32 unit = 0; unit < units.count; unit++) {
            const float2 world_a = rail.world_a + units.world_step * float2 { static_cast<f32>(unit) };
            const float2 world_b = world_a + units.world_step;
            const f32 grade = (profile.rail[unit + 1] - profile.rail[unit]) * ELEVATION_UNIT_METERS / units.length_meters;
            const f32 clearance = (profile.rail[unit] + profile.rail[unit + 1] - profile.terrain[unit] - profile.terrain[unit + 1]) * 0.5F;
            if (camera.scale >= RAIL_GRADE_LABEL_MIN_CAMERA_SCALE) {
                const u32 level = std::min(static_cast<u32>(math::Round(math::Abs(grade) * 100.0F / RAIL_GRADE_PERCENT_PER_LEVEL)), RAIL_GRADE_LEVEL_MAX);
                rail_grade_label_draws.EmplaceBack(camera.WorldToScreen((world_a + world_b) * float2 { 0.5F } + world_label_offset), level);
            }
            const b8 bridge = clearance >= RAIL_BRIDGE_CLEARANCE_MIN_ELEVATION_UNITS || profile.terrain[unit] + profile.terrain[unit + 1] < 0.0F;
            const b8 tunnel = !bridge && clearance <= -RAIL_TUNNEL_DEPTH_MIN_ELEVATION_UNITS;
            if (bridge || tunnel) {
                const Color deck_color = bridge ? COLOR_BRIDGE_DECK : COLOR_TUNNEL_ROCK;
                verts.EmplaceBack(camera.WorldToScreen(world_a - world_deck_half_width), deck_color);
                verts.EmplaceBack(camera.WorldToScreen(world_a + world_deck_half_width), deck_color);
                verts.EmplaceBack(camera.WorldToScreen(world_b + world_deck_half_width), deck_color);
                verts.EmplaceBack(camera.WorldToScreen(world_a - world_deck_half_width), deck_color);
                verts.EmplaceBack(camera.WorldToScreen(world_b + world_deck_half_width), deck_color);
                verts.EmplaceBack(camera.WorldToScreen(world_b - world_deck_half_width), deck_color);
            }
            const Color color = tunnel ? grade_to_color(grade).Mul(RAIL_TUNNEL_BRIGHTNESS) : grade_to_color(grade);
            const float2 screen_a_left = camera.WorldToScreen(world_a - world_half_width);
            const float2 screen_a_right = camera.WorldToScreen(world_a + world_half_width);
            const float2 screen_b_left = camera.WorldToScreen(world_b - world_half_width);
            const float2 screen_b_right = camera.WorldToScreen(world_b + world_half_width);
            rail_verts.EmplaceBack(screen_a_left, color, float2 { 0.0F, 0.0F });
            rail_verts.EmplaceBack(screen_a_right, color, float2 { 1.0F, 0.0F });
            rail_verts.EmplaceBack(screen_b_right, color, float2 { 1.0F, 1.0F });
            rail_verts.EmplaceBack(screen_a_left, color, float2 { 0.0F, 0.0F });
            rail_verts.EmplaceBack(screen_b_right, color, float2 { 1.0F, 1.0F });
            rail_verts.EmplaceBack(screen_b_left, color, float2 { 0.0F, 1.0F });
        }
    }

    void FlushTerrainTexture() {
        if (terrain_texture_dirty_axials.empty()) { return; }
        WindowState& window_state = Singleton::Get<WindowState>();
        (void)SDL_SetRenderTarget(window_state.renderer, terrain_texture);
        (void)SDL_SetRenderScale(window_state.renderer, 1.0F, 1.0F);
        for (u32 start = 0; start < terrain_texture_dirty_axials.size(); start += TERRAIN_TEXTURE_HEXES_PER_DRAW) {
            verts.clear();
            for (u32 i = start; i < std::min(start + TERRAIN_TEXTURE_HEXES_PER_DRAW, terrain_texture_dirty_axials.size()); i++) {
                const int2 axial = terrain_texture_dirty_axials[i];
                AppendTerrainHex(TerrainTexturePixel(HexAxialToWorld(axial)), TERRAIN_TEXTURE_HEX_RADIUS * 1.1F, axial);
            }
            (void)SDL_RenderGeometry(window_state.renderer, nullptr, verts);
        }
        (void)SDL_SetRenderTarget(window_state.renderer, nullptr);
        (void)SDL_SetRenderScale(window_state.renderer, window_state.ui_scale, window_state.ui_scale);
        terrain_texture_dirty_axials.clear();
    }

    [[nodiscard]] static EditorDocument LoadDocument(const AssetPath& terrain_path) {
        EditorDocument loaded { .elevation = TerrainLoad(terrain_path) };
        const std::string stem = terrain_path.stem().string();
        if (std::filesystem::exists(Asset(terrain_path.parent_path() / (stem + "_rivers.txt")))) { loaded.rivers = RiversLoad(terrain_path.parent_path() / (stem + "_rivers.txt")); }
        if (std::filesystem::exists(Asset(terrain_path.parent_path() / (stem + "_water_labels.txt")))) { loaded.water_labels = WaterLabelsLoad(terrain_path.parent_path() / (stem + "_water_labels.txt")); }
        const std::string overlay_prefix = stem + "_cities_";
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
        frame.year_slider.SetValue(globalData[frame.tree], overlay_year, std::format("{} ({} cities)", overlay_year, Cities().size()));
        generated.reset();
        SelectCity(std::nullopt);
        RebuildCityLabels();
    }

    void AddFileButton(const AssetPath& asset_path) {
        const Handle<Node> button = Button(NodeReference { frame.tree, frame.file_list }, std::format("▸ {}/{}", asset_path.parent_path().filename().string(), asset_path.stem().string()), FontSizes::body, 2U);
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
        rail_drag_start_world.reset();
        SetButtonTextColor(frame.tree, frame.terrain_tool_button, tool == EditorTool::TOOL_TERRAIN ? COLOR_BUTTON_TEXT : COLOR_BUTTON_TEXT_INACTIVE);
        SetButtonTextColor(frame.tree, frame.city_tool_button, tool == EditorTool::TOOL_CITY ? COLOR_BUTTON_TEXT : COLOR_BUTTON_TEXT_INACTIVE);
        SetButtonTextColor(frame.tree, frame.rail_tool_button, tool == EditorTool::TOOL_RAIL ? COLOR_BUTTON_TEXT : COLOR_BUTTON_TEXT_INACTIVE);
        SetButtonTextColor(frame.tree, frame.rail_path_tool_button, tool == EditorTool::TOOL_RAIL_PATH ? COLOR_BUTTON_TEXT : COLOR_BUTTON_TEXT_INACTIVE);
        switch (tool) {
            case EditorTool::TOOL_TERRAIN: globalData[frame.tree].node_properties[frame.help_label].text = "Left: raise   Right: lower   Ctrl+drag: pan"; break;
            case EditorTool::TOOL_CITY: globalData[frame.tree].node_properties[frame.help_label].text = "Left: add / select city   Right: remove city"; break;
            case EditorTool::TOOL_RAIL: globalData[frame.tree].node_properties[frame.help_label].text = "Left: drag rail   Right: remove rail"; break;
            case EditorTool::TOOL_RAIL_PATH: globalData[frame.tree].node_properties[frame.help_label].text = "Left: drag fastest rail path   Right: remove rail"; break;
        }
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
        water_labels.clear();
        TTF_Font* font = Singleton::Get<FontCollection>().GetFontNormalCourier(FontSizes::body);
        for (const MapLabel& label : document.water_labels) { water_labels.EmplaceBack(font, String { label.name.c_str() }); }
        SetOverlayYear(overlay_year);
        RebuildTerrainTexture();
        CameraState& camera = Singleton::Get<CameraState>();
        const float2 map_world_max = HexAxialToWorld(HexOffsetToAxial(static_cast<int2>(document.elevation.map_size - uint2 { 1U, 1U })));
        if (camera.map_world_max == map_world_max) { return; }
        camera.map_world_min = { 0.0F, 0.0F };
        camera.map_world_max = map_world_max;
        const float2 screen_size { Singleton::Get<WindowState>().screen_size };
        camera.world_position = (camera.map_world_min + camera.map_world_max) * float2 { 0.5F * camera.scale } - screen_size * float2 { 0.5F };
    }

    void SelectCity(const Optional<u32> city_index) { selected_city = city_index; }

    void RebuildCityLabels() {
        city_labels.clear();
        TTF_Font* font = Singleton::Get<FontCollection>().GetFontBoldCourier(CITY_LABEL_FONT_SIZE);
        for (const City& city : Cities()) { city_labels.EmplaceBack(font, String { city.name.c_str() }); }
    }

    void operator()() {
        InputState& input = Singleton::Get<InputState>();
        CameraState& camera = Singleton::Get<CameraState>();
        const b8 ctrl = input.keys[SDLK_LCTRL];
        if (ctrl && input.keys_down[SDLK_Z]) { Undo(); }
        if (ctrl && input.keys_down[SDLK_U]) { Redo(); }

        const HoveredType& hovered = Singleton::Get<HoveredType>();
        if (const Optional<u32> year = frame.year_slider.Drag(input, hovered, frame.tree); year.has_value() && year != overlay_year) { SetOverlayYear(year.value()); }
        if (const Optional<u32> size = frame.river_slider.Drag(input, hovered, frame.tree); size.has_value() && size != river_size_min) { SetRiverSizeMin(size.value()); }
        if (const Optional<u32> units = frame.tunnel_slider.Drag(input, hovered, frame.tree); units.has_value() && units != rail_tunnel_save_units) { SetRailTunnelSave(units.value()); }
        if (const Optional<u32> units = frame.bridge_slider.Drag(input, hovered, frame.tree); units.has_value() && units != rail_bridge_save_units) { SetRailBridgeSave(units.value()); }

        const float2 screen_size { Singleton::Get<WindowState>().screen_size };
        const float2 texture_size = TerrainTextureSize();
        const f32 minimap_scale = MINIMAP_WIDTH / texture_size.x;
        const SDL_FRect minimap_rect { MINIMAP_SCREEN_MARGIN, MINIMAP_SCREEN_MARGIN, MINIMAP_WIDTH, texture_size.y * minimap_scale };
        const b8 over_minimap = input.mouse_position.x >= minimap_rect.x && input.mouse_position.y >= minimap_rect.y && input.mouse_position.x <= minimap_rect.x + minimap_rect.w && input.mouse_position.y <= minimap_rect.y + minimap_rect.h;
        if (over_minimap && input.left_mouse && !ctrl) {
            const float2 world = (input.mouse_position - float2 { minimap_rect.x, minimap_rect.y }) / float2 { minimap_scale * TERRAIN_TEXTURE_HEX_RADIUS } - TERRAIN_TEXTURE_WORLD_MARGIN;
            camera.world_position = world * float2 { camera.scale } - screen_size * float2 { 0.5F };
        }

        const float2 world_hover = camera.ScreenToWorld(input.mouse_position);
        const int2 axial_hover = HexWorldToAxial(world_hover);
        const b8 over_ui = hovered.has_value() || over_minimap;
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
                    for (i32 dy = -radius - 1; dy <= radius + 1; dy++) {
                        for (i32 dx = -radius - 1; dx <= radius + 1; dx++) {
                            const int2 axial = axial_hover + int2 { dx, dy };
                            const u32 distance = HexAxialDistance(axial, axial_hover);
                            if (distance > static_cast<u32>(radius) + 1U || !document.elevation.Contains(axial)) { continue; }
                            if (distance <= static_cast<u32>(radius)) {
                                i8& elevation_value = document.elevation[axial];
                                elevation_value = static_cast<i8>(std::clamp<i32>(elevation_value + (paint_raise ? BRUSH_ELEVATION_STEP : -BRUSH_ELEVATION_STEP), ELEVATION_MIN, ELEVATION_MAX));
                            }
                            terrain_texture_dirty_axials.EmplaceBack(axial);
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
                        cities.push_back(City { .axial = axial_hover, .level = 1.0F, .name = std::format("City {}", cities.size() + 1U) });
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
            case EditorTool::TOOL_RAIL:
            case EditorTool::TOOL_RAIL_PATH: {
                if (left_click && document.elevation.Contains(axial_hover)) { rail_drag_start_world = SnapToRailEnd(world_hover); }
                if (!input.left_mouse && rail_drag_start_world.has_value()) {
                    if (tool == EditorTool::TOOL_RAIL_PATH && document.elevation.Contains(axial_hover)) {
                        const std::vector<Rail> rails = RailPath(*rail_drag_start_world, SnapToRailEnd(world_hover));
                        if (!rails.empty()) {
                            PushHistory();
                            document.rails.append_range(rails);
                        }
                        SetStatus(rails.empty() ? String { "Rail path: no route" } : String { std::format("Rail path: {} rails", rails.size()).c_str() });
                    } else if (const Optional<Rail> rail = DragRail(world_hover); tool == EditorTool::TOOL_RAIL && rail.has_value()) {
                        PushHistory();
                        document.rails.push_back(*rail);
                    }
                    rail_drag_start_world.reset();
                }
                const auto rail_hovered = [world_hover](const Rail& rail) { return RailDistanceWorld(rail, world_hover) <= RAIL_WIDTH_WORLD; };
                if (right_click && std::ranges::any_of(document.rails, rail_hovered)) {
                    PushHistory();
                    std::erase_if(document.rails, rail_hovered);
                }
                break;
            }
        }

        FlushTerrainTexture();
        SDL_Renderer* renderer = Singleton::Get<WindowState>().renderer;
        const f32 hex_screen_radius = camera.scale;
        const u32 brush_hover_radius = tool == EditorTool::TOOL_TERRAIN && !over_ui ? brush_radius - 1U : 0U;
        verts.clear();
        if (camera.scale < TERRAIN_TEXTURE_MAX_CAMERA_SCALE) {
            const float2 screen_origin = camera.WorldToScreen(float2 { 0.0F, 0.0F } - TERRAIN_TEXTURE_WORLD_MARGIN);
            const float2 screen_texture_size = texture_size * float2 { camera.scale / TERRAIN_TEXTURE_HEX_RADIUS };
            const SDL_FRect destination { screen_origin.x, screen_origin.y, screen_texture_size.x, screen_texture_size.y };
            (void)SDL_RenderTexture(renderer, terrain_texture, nullptr, &destination);
            const i32 radius = static_cast<i32>(brush_hover_radius);
            for (i32 dy = -radius; dy <= radius; dy++) {
                for (i32 dx = -radius; dx <= radius; dx++) {
                    const int2 axial = axial_hover + int2 { dx, dy };
                    if (HexAxialDistance(axial, axial_hover) > brush_hover_radius || !document.elevation.Contains(axial)) { continue; }
                    AppendTerrainHex(camera.WorldToScreen(HexAxialToWorld(axial)), hex_screen_radius, axial, 1.2F);
                }
            }
        } else {
            const uint2 map_size = document.elevation.map_size;
            const float2 world_min = camera.ScreenToWorld({ 0.0F, 0.0F }) - float2 { 1.0F };
            const float2 world_max = camera.ScreenToWorld(screen_size) + float2 { 1.0F };
            const i32 row_min = std::clamp(static_cast<i32>(std::floor(world_min.y / HEX_SPACING.y)), 0, static_cast<i32>(map_size.y) - 1);
            const i32 row_max = std::clamp(static_cast<i32>(std::ceil(world_max.y / HEX_SPACING.y)), 0, static_cast<i32>(map_size.y) - 1);
            for (i32 row = row_min; row <= row_max; row++) {
                const f32 row_shift = row & 1 ? 0.5F : 0.0F;
                const i32 column_min = std::clamp(static_cast<i32>(std::floor(world_min.x / HEX_SPACING.x - row_shift)), 0, static_cast<i32>(map_size.x) - 1);
                const i32 column_max = std::clamp(static_cast<i32>(std::ceil(world_max.x / HEX_SPACING.x - row_shift)), 0, static_cast<i32>(map_size.x) - 1);
                for (i32 column = column_min; column <= column_max; column++) {
                    const int2 axial = HexOffsetToAxial(int2 { column, row });
                    AppendTerrainHex(camera.WorldToScreen(HexAxialToWorld(axial)), hex_screen_radius, axial, HexAxialDistance(axial, axial_hover) <= brush_hover_radius ? 1.2F : 1.0F);
                }
            }
        }
        for (const River& river : document.rivers) {
            if (river.size < river_size_min) { continue; }
            const Color color = colors::ColorLerp(COLOR_RIVER_SMALL, COLOR_RIVER_LARGE, static_cast<f32>(river.size) / RIVER_SIZE_MAX);
            for (const int2 axial : river.axials) {
                const float2 screen = camera.WorldToScreen(HexAxialToWorld(axial));
                if (screen.x < -camera.scale || screen.y < -camera.scale || screen.x > screen_size.x + camera.scale || screen.y > screen_size.y + camera.scale) { continue; }
                AppendHex(verts, screen, hex_screen_radius, color);
            }
        }
        rail_grade_label_draws.clear();
        rail_verts.clear();
        for (const Rail& rail : document.rails) { AppendRail(rail, camera, [](f32) { return colors::COLOR_WHITE; }); }
        if (const Optional<Rail> rail_drag = DragRail(world_hover)) { AppendRail(*rail_drag, camera, RailGradeToColor); }
        if (generated.has_value()) {
            for (const Building& building : generated->buildings) { AppendSquare(verts, camera.WorldToScreen(building.pos), camera.scale * building_defines[building.id.value].size_hex_widths * HEX_SPACING.x * 0.5F, building.rotation, BuildingColor(building.id)); }
            for (const Industry& industry : generated->industries) { AppendHex(verts, camera.WorldToScreen(industry.pos), camera.scale * INDUSTRY_RADIUS_WORLD, IndustryColor(industry.id)); }
        }
        (void)SDL_RenderGeometry(renderer, nullptr, verts);
        (void)SDL_RenderGeometry(renderer, globalData[rail_texture], rail_verts);
        (void)SDL_RenderTexture(renderer, terrain_texture, nullptr, &minimap_rect);
        const float2 minimap_view_min = float2 { minimap_rect.x, minimap_rect.y } + TerrainTexturePixel(camera.ScreenToWorld({ 0.0F, 0.0F })) * float2 { minimap_scale };
        const float2 minimap_view_max = float2 { minimap_rect.x, minimap_rect.y } + TerrainTexturePixel(camera.ScreenToWorld(screen_size)) * float2 { minimap_scale };
        const SDL_FRect minimap_view_rect { minimap_view_min.x, minimap_view_min.y, minimap_view_max.x - minimap_view_min.x, minimap_view_max.y - minimap_view_min.y };
        (void)SDL_SetRenderDrawColor(renderer, COLOR_MINIMAP_VIEW.r, COLOR_MINIMAP_VIEW.g, COLOR_MINIMAP_VIEW.b, COLOR_MINIMAP_VIEW.a);
        (void)SDL_RenderRect(renderer, &minimap_view_rect);
        (void)SDL_SetRenderDrawColor(renderer, COLOR_MINIMAP_BORDER.r, COLOR_MINIMAP_BORDER.g, COLOR_MINIMAP_BORDER.b, COLOR_MINIMAP_BORDER.a);
        (void)SDL_RenderRect(renderer, &minimap_rect);
        const auto on_screen = [screen_size](const float2 screen) { return screen.x > -screen_size.x * 0.1F && screen.y > -screen_size.y * 0.1F && screen.x < screen_size.x * 1.1F && screen.y < screen_size.y * 1.1F; };
        for (u32 i = 0; i < document.water_labels.size(); i++) {
            const float2 screen = camera.WorldToScreen(HexAxialToWorld(document.water_labels[i].axial));
            if (!on_screen(screen)) { continue; }
            water_labels[i].SetColor(COLOR_WATER_LABEL);
            water_labels[i].Draw(screen - float2 { 0.0F, static_cast<f32>(FontSizes::body) * 0.5F });
        }
        for (const RailGradeLabelDraw& draw : rail_grade_label_draws) {
            if (!on_screen(draw.screen)) { continue; }
            int2 text_size { 0, 0 };
            (void)TTF_GetTextSize(rail_grade_labels[draw.level], &text_size.x, &text_size.y);
            rail_grade_labels[draw.level].SetColor(RailGradeToColor(static_cast<f32>(draw.level) * RAIL_GRADE_PERCENT_PER_LEVEL * 0.01F));
            rail_grade_labels[draw.level].Draw(draw.screen - float2 { static_cast<f32>(text_size.x), static_cast<f32>(text_size.y) } * float2 { 0.5F });
        }
        verts.clear();
        for (u32 i = 0; i < cities.size() && camera.scale >= CITY_LABEL_MIN_CAMERA_SCALE; i++) {
            const float2 screen = camera.WorldToScreen(HexAxialToWorld(cities[i].axial));
            if (!on_screen(screen)) { continue; }
            const Color color = selected_city == i ? COLOR_CITY_SELECTED : COLOR_CITY;
            city_labels[i].SetColor(color);
            int2 text_size { 0, 0 };
            (void)TTF_GetTextSize(city_labels[i], &text_size.x, &text_size.y);
            const u32 star_count = static_cast<u32>(math::Ceil(cities[i].level));
            city_labels[i].Draw(screen - float2 { static_cast<f32>(text_size.x) * 0.5F, static_cast<f32>(text_size.y) + CITY_STAR_SCREEN_RADIUS * 2.0F + CITY_LABEL_SCREEN_GAP * 2.0F });
            const float2 screen_star_row = screen - float2 { static_cast<f32>(star_count - 1U) * CITY_STAR_SCREEN_RADIUS * 1.1F, CITY_STAR_SCREEN_RADIUS + CITY_LABEL_SCREEN_GAP };
            for (u32 star = 0; static_cast<f32>(star) < cities[i].level; star++) {
                const float2 screen_star = screen_star_row + float2 { static_cast<f32>(star) * CITY_STAR_SCREEN_RADIUS * 2.2F, 0.0F };
                AppendStar(verts, screen_star, CITY_STAR_SCREEN_RADIUS, 1.0F, COLOR_CITY_STAR_EMPTY);
                AppendStar(verts, screen_star, CITY_STAR_SCREEN_RADIUS, std::min(cities[i].level - static_cast<f32>(star), 1.0F), COLOR_CITY_STAR);
            }
        }
        (void)SDL_RenderGeometry(renderer, nullptr, verts);
    }
};
} // namespace rail
