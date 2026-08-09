module;

export module hex.counter;

import std;

import pce.std;
import pce.math;
import pce.globals;
import pce.window_state;
import pce.assets;
import pce.font;
import pce.sdl;
import pcs.camera;
import pce.ui;
import pce.collections;
import pce.colors;

import hex.hex;
import hex.enums;
import hex.types;
import hex.terrain;
import hex.render;

export namespace hex {
[[nodiscard]] constexpr String EchelonToString(const Echelon echelon) {
    switch (echelon) {
        case Echelon::ECHELON_SQUAD: return "..";
        case Echelon::ECHELON_PLATOON: return "...";
        case Echelon::ECHELON_COMPANY: return "I";
        case Echelon::ECHELON_BATTALION: return "II";
        case Echelon::ECHELON_REGIMENT: return "III";
        case Echelon::ECHELON_BRIGADE: return "x";
        case Echelon::ECHELON_DIVISION: return "xx";
        case Echelon::ECHELON_CORPS: return "xxx";
        case Echelon::ECHELON_ARMY: return "xxxx";
    }
    std::unreachable();
}

[[nodiscard]] constexpr String UnitIconToString(const UnitIcon icon) {
    switch (icon) {
        case UnitIcon::ICON_INF: return "inf";
        case UnitIcon::ICON_ART: return "art";
        case UnitIcon::ICON_HQ: return "hq";
        case UnitIcon::ICON_TANK: return "tnk";
        case UnitIcon::ICON_ENGINEER: return "eng";
    }
    std::unreachable();
}
[[nodiscard]] constexpr String MoveTypeToString(const MoveType move_type) {
    switch (move_type) {
        case MoveType::MOVE_LEG: return "/\\";
        case MoveType::MOVE_TAC: return "ooo";
        case MoveType::MOVE_TRUCK: return "O O";
    }
    std::unreachable();
}

constexpr f32 TEXT_SHADOW_STRENGTH = 1.0F;
constexpr f32 OFFSET_STACK = 1.0F / 16.0F;
constexpr f32 OFFSET_SHADOW = OFFSET_STACK / 3;
constexpr f32 BORDER_THICKNESS = OFFSET_STACK / 6;

[[nodiscard]] constexpr ui::ColorBox MoveTypeToColorBox(const MoveType move_type) {
    switch (move_type) {
        case MoveType::MOVE_LEG: return ui::ColorBox { .color_text = colors::COLOR_WHITE, .color_text_shadow = colors::COLOR_BLACK.WithAlpha(TEXT_SHADOW_STRENGTH) };
        case MoveType::MOVE_TAC: return ui::ColorBox { .color_text = colors::COLOR_RED, .color_text_shadow = colors::COLOR_WHITE.WithAlpha(TEXT_SHADOW_STRENGTH) };
        case MoveType::MOVE_TRUCK: return ui::ColorBox { .color_text = colors::COLOR_BLACK, .color_text_shadow = colors::COLOR_WHITE.WithAlpha(TEXT_SHADOW_STRENGTH) };
    }
    std::unreachable();
}

[[nodiscard]] constexpr ui::ColorBox RangedTypeToColorBox(const RangedType ranged_type) {
    switch (ranged_type) {
        case RangedType::RANGED_NONE: return ui::ColorBox { .color_text = colors::COLOR_WHITE, .color_text_shadow = colors::COLOR_BLACK.WithAlpha(TEXT_SHADOW_STRENGTH) };
        case RangedType::RANGED_DEFENSE: return ui::ColorBox { .color_text = colors::COLOR_BLACK, .color_text_shadow = colors::COLOR_WHITE.WithAlpha(TEXT_SHADOW_STRENGTH) };
        case RangedType::RANGED_ATTACK: return ui::ColorBox { .color_text = colors::COLOR_RED, .color_text_shadow = colors::COLOR_WHITE.WithAlpha(TEXT_SHADOW_STRENGTH) };
    }
    std::unreachable();
}

struct CounterTextures {
    HandleOptional<Texture> infantry;
    HandleOptional<Texture> artillery;
    HandleOptional<Texture> armor;
    HandleOptional<Texture> headquarters;
    HandleOptional<Texture> engineer;

    explicit CounterTextures(const RelativePath& dir) {
        infantry = globalData.Create<Texture>(Asset(dir / "counter-inf.jpg"));
        artillery = globalData.Create<Texture>(Asset(dir / "counter-art.jpg"));
        armor = globalData.Create<Texture>(Asset(dir / "counter-armor.jpg"));
        headquarters = globalData.Create<Texture>(Asset(dir / "counter-hq.jpg"));
        engineer = globalData.Create<Texture>(Asset(dir / "counter-engineer.jpg"));
    }
    [[nodiscard]] HandleOptional<Texture> ForIcon(const UnitIcon icon) const {
        switch (icon) {
            case UnitIcon::ICON_INF: return infantry;
            case UnitIcon::ICON_ART: return artillery;
            case UnitIcon::ICON_TANK: return armor;
            case UnitIcon::ICON_HQ: return headquarters;
            case UnitIcon::ICON_ENGINEER: return engineer;
        }
        std::unreachable();
    }
};
struct CounterTextureStack {
    CounterTextures counter_textures_niehorster { "counters/counter-niehorster" };
    CounterTextures counter_textures_niehorster_big { "counters/counter-niehorster-big" };
    CounterTextures counter_textures_real { "counters/counter-real" };
};

[[nodiscard]] inline const CounterTextures& CounterStyleToTextures(const CounterStyle style) {
    const CounterTextureStack& stack = Singleton::Get<CounterTextureStack>();
    switch (style) {
        case CounterStyle::COUNTER_STYLE_NIEHORSTER: return stack.counter_textures_niehorster;
        case CounterStyle::COUNTER_STYLE_NIEHORSTER_BIG: return stack.counter_textures_niehorster_big;
        case CounterStyle::COUNTER_STYLE_REAL: return stack.counter_textures_real;
    }
    std::unreachable();
}

// rough division counter placeholder: country color body, full name bar in formation color on top
inline void DrawFormationCounter(const WindowState& window_state, const UnitFormation& formation, const AABB& area, Pool<Label>& label_pool) {
    const ui::FontCollection& font_collection = Singleton::Get<ui::FontCollection>();

    ui::DrawRect(window_state, area.WithOffset(float2 { OFFSET_SHADOW * area.size.x }), colors::COLOR_BLACK.WithAlpha(0.15F));
    ui::DrawRect(window_state, area, colors::COLOR_BLACK);
    ui::DrawRect(window_state, area.WithPadding(float2 { BORDER_THICKNESS * area.size.x }), CountryBranchToColor(formation.tag, formation.branch));

    constexpr f32 NAME_PADDING = 0.05F;
    const AABB area_name = AABB::FromPoint(area.point + area.size * float2 { NAME_PADDING }, float2 { area.size.x * (1.0F - 2.0F * NAME_PADDING), area.size.y * 0.40F });
    ui::DrawRect(window_state, area_name, colors::COLOR_BLACK);
    ui::DrawRect(window_state, area_name.WithPadding(float2 { BORDER_THICKNESS * area.size.x }), formation.color);
    const ui::FontSize pt_name = static_cast<ui::FontSize>(area.size.y * 0.13F);
    if (pt_name >= ui::FONT_MIN_SIZE) {
        const Label& label_name = label_pool.Get();
        label_name.SetText(UnitNameToString(formation.name_full));
        DrawText(font_collection.GetFontBoldCompact(static_cast<ui::FontSizes>(pt_name)), label_name, area_name, colors::COLOR_BLACK, ui::TextAlignment::CENTER);
    }

    // stat row: fatigue, artillery max, prepared defense
    const ui::FontSize pt_stat = static_cast<ui::FontSize>(area.size.y * 0.10F);
    const ui::FontSize pt_stat_header = math::Max(static_cast<ui::FontSize>(area.size.y * 0.05F), ui::FONT_MIN_SIZE);
    if (pt_stat >= ui::FONT_MIN_SIZE) {
        constexpr f32 STAT_WIDTH = 0.22F;
        constexpr f32 STAT_HEADER_Y = 0.49F;
        constexpr f32 STAT_HEADER_HEIGHT = 0.08F;
        constexpr f32 STAT_BOX_Y = 0.58F;
        constexpr f32 STAT_BOX_HEIGHT = 0.15F;
        constexpr f32 STAT_X_FATIGUE = 0.08F;
        constexpr f32 STAT_X_ARTILLERY = 0.39F;
        constexpr f32 STAT_X_DEFENSE = 0.70F;
        const ui::Font& font_stat = font_collection.GetFontBoldCompact(static_cast<ui::FontSizes>(pt_stat));
        const ui::Font& font_stat_header = font_collection.GetFontBoldCompact(static_cast<ui::FontSizes>(pt_stat_header));
        const float2 stat_box_padding { area.size.x * 0.01F };
        const ui::ColorBox color_box_stat { .color_fill = colors::COLOR_WHITE_SMOKE, .color_stroke = colors::COLOR_BLACK, .color_text = colors::COLOR_BLACK };

        const Label& label_fatigue_header = label_pool.Get();
        label_fatigue_header.SetText("fatigue");
        DrawText(font_stat_header, label_fatigue_header, AABB::FromPoint(area.point + area.size * float2 { STAT_X_FATIGUE, STAT_HEADER_Y }, area.size * float2 { STAT_WIDTH, STAT_HEADER_HEIGHT }), colors::COLOR_BLACK, ui::TextAlignment::CENTER);
        const Label& label_fatigue = label_pool.Get();
        label_fatigue.SetText(std::format("{}", formation.fatigue));
        DrawColorBox(window_state, font_stat, label_fatigue, ui::AABBWithPadding { .area = AABB::FromPoint(area.point + area.size * float2 { STAT_X_FATIGUE, STAT_BOX_Y }, area.size * float2 { STAT_WIDTH, STAT_BOX_HEIGHT }), .padding = stat_box_padding }, color_box_stat, ui::TextAlignment::CENTER);

        const HandleOptional<Texture> texture_artillery = CounterStyleToTextures(COUNTER_THEME).artillery;
        if (texture_artillery.IsValid()) { ui::DrawTexture(window_state, texture_artillery.GetHandle(), AABB::FromPoint(area.point + area.size * float2 { STAT_X_ARTILLERY + STAT_WIDTH * 0.25F, STAT_HEADER_Y }, area.size * float2 { STAT_WIDTH * 0.5F, STAT_HEADER_HEIGHT }), colors::COLOR_WHITE); }
        const Label& label_artillery = label_pool.Get();
        label_artillery.SetText(std::format("{}", formation.artillery.max));
        DrawColorBox(window_state, font_stat, label_artillery, ui::AABBWithPadding { .area = AABB::FromPoint(area.point + area.size * float2 { STAT_X_ARTILLERY, STAT_BOX_Y }, area.size * float2 { STAT_WIDTH, STAT_BOX_HEIGHT }), .padding = stat_box_padding }, color_box_stat, ui::TextAlignment::CENTER);

        const Label& label_defense_header = label_pool.Get();
        label_defense_header.SetText("defense");
        DrawText(font_stat_header, label_defense_header, AABB::FromPoint(area.point + area.size * float2 { STAT_X_DEFENSE, STAT_HEADER_Y }, area.size * float2 { STAT_WIDTH, STAT_HEADER_HEIGHT }), colors::COLOR_BLACK, ui::TextAlignment::CENTER);
        const Label& label_defense = label_pool.Get();
        label_defense.SetText(formation.prepared_defense ? "yes" : "no");
        DrawColorBox(window_state, font_stat, label_defense, ui::AABBWithPadding { .area = AABB::FromPoint(area.point + area.size * float2 { STAT_X_DEFENSE, STAT_BOX_Y }, area.size * float2 { STAT_WIDTH, STAT_BOX_HEIGHT }), .padding = stat_box_padding }, color_box_stat, ui::TextAlignment::CENTER);
    }

    // decorative trim
    for (u32 i = 0; i < 3U; i++) {
        const f32 stripe_y = 0.775F + 0.055F * static_cast<f32>(i);
        ui::DrawRect(window_state, AABB::FromPoint(area.point + area.size * float2 { NAME_PADDING, stripe_y }, float2 { area.size.x * (1.0F - 2.0F * NAME_PADDING), area.size.y * 0.018F }), formation.color);
    }
}

inline void RenderCounters(const Pool<CounterStack>& counters) {
    const CameraState& camera = Singleton::Get<CameraState>();
    const WindowState& window_state = Singleton::Get<WindowState>();
    const ui::FontCollection& font_collection = Singleton::Get<ui::FontCollection>();
    const CounterTextures& counter_textures = CounterStyleToTextures(COUNTER_THEME);

    constexpr f32 COUNTER_SIZE = 1.1F;
    const float2 counter_size = float2 { camera.scale * COUNTER_SIZE };

    const float2 color_box_padding = counter_size * float2 { 0.01F };

    const ui::FontSize pt_22 = static_cast<ui::FontSize>(counter_size.y * 0.22F);
    const ui::FontSize pt_16 = static_cast<ui::FontSize>(counter_size.y * 0.16F);
    const ui::FontSize pt_12 = static_cast<ui::FontSize>(counter_size.y * 0.12F);
    const ui::FontSize pt_10 = static_cast<ui::FontSize>(counter_size.y * 0.10F);
    const ui::FontSize pt_08 = static_cast<ui::FontSize>(counter_size.y * 0.08F);
    const Optional<std::reference_wrapper<const ui::Font>> font_22_opt = pt_22 < ui::FONT_MIN_SIZE ? Optional<std::reference_wrapper<const ui::Font>> { std::nullopt } : font_collection.GetFontBoldCompact(static_cast<ui::FontSizes>(pt_22));
    const Optional<std::reference_wrapper<const ui::Font>> font_16_opt = pt_16 < ui::FONT_MIN_SIZE ? Optional<std::reference_wrapper<const ui::Font>> { std::nullopt } : font_collection.GetFontBoldCompact(static_cast<ui::FontSizes>(pt_16));
    const Optional<std::reference_wrapper<const ui::Font>> font_12_opt = pt_12 < ui::FONT_MIN_SIZE ? Optional<std::reference_wrapper<const ui::Font>> { std::nullopt } : font_collection.GetFontBoldCourier(static_cast<ui::FontSizes>(pt_12));
    const Optional<std::reference_wrapper<const ui::Font>> font_10_opt = pt_10 < ui::FONT_MIN_SIZE ? Optional<std::reference_wrapper<const ui::Font>> { std::nullopt } : font_collection.GetFontBoldCompact(static_cast<ui::FontSizes>(pt_10));
    const Optional<std::reference_wrapper<const ui::Font>> font_08_opt = pt_08 < ui::FONT_MIN_SIZE ? Optional<std::reference_wrapper<const ui::Font>> { std::nullopt } : font_collection.GetFontBoldCourier(static_cast<ui::FontSizes>(pt_08));

    for (const CounterStack& counter : counters) {
        const float2 world = HexAxialToWorld(counter.axial);
        const float2 counter_center = camera.WorldToScreen(world);
        const float2 counter_top_left = counter_center - counter_size * float2 { 0.5F };

        for (i32 i = static_cast<i32>(counter.stack.size()) - 1; i >= 0; i--) {
            const Counter& counter_stack = counter.stack[static_cast<u32>(i)];
            if (counter_stack.color_icon.a == 0) { continue; }

            const AABB area_counter = AABB::FromCenter(counter_center + float2 { OFFSET_STACK * counter_size.x * static_cast<f32>(i) }, counter_size);
            constexpr Color COLOR_SHADOW = colors::COLOR_BLACK.WithAlpha(0.15F);

            ui::DrawRect(window_state, area_counter.WithOffset(float2 { OFFSET_SHADOW * counter_size.x }), COLOR_SHADOW);
            ui::DrawRect(window_state, area_counter, counter_stack.color_border);
            ui::DrawRect(window_state, area_counter.WithPadding(float2 { BORDER_THICKNESS * counter_size.x }), counter_stack.color_background);
        }

        // div
        if (font_10_opt.has_value()) {
            const ui::AABBWithPadding area_name_div { .area = AABB::FromPoint(counter_top_left + counter_size * float2 { 0.0F, 0.09F }, counter_size * float2 { 1.0F, 0.25F }), .padding = color_box_padding };
            const ui::ColorBox color_box_name_div { .color_fill = counter.stack[0].color_icon, .color_stroke = colors::COLOR_BLACK, .color_text = colors::COLOR_BLACK };
            DrawColorBox(window_state, font_10_opt.value(), counter.label_name_div, area_name_div, color_box_name_div);

            const ui::AABBWithPadding area_name_sub { .area = AABB::FromPoint(counter_top_left + counter_size * float2 { 0.0F, 0.35F }, counter_size * float2 { 0.97F, 0.12F }), .padding = color_box_padding };
            const ui::ColorBox color_box_name_sub { .color_text = colors::COLOR_BLACK };
            DrawColorBox(window_state, font_10_opt.value(), counter.label_name_sub, area_name_sub, color_box_name_sub);
        } else {
            const AABB area_div = AABB::FromPoint(counter_top_left + counter_size * float2 { 0.0F, 0.09F }, counter_size * float2 { 1.0F, 0.5F });
            ui::DrawRect(window_state, area_div, counter.stack[0].color_icon);
        }

        // icon
        const AABB area_icon_border = AABB::FromPoint(counter_top_left, counter_size * float2 { 0.6F, 0.4F }).WithOffset(counter_size * float2 { 0.02F });
        // DrawRect(window_state, area_icon_border, colors::COLOR_BLACK);

        const AABB area_icon = area_icon_border.WithPadding(float2 { camera.scale / 40.0F });
        // const Color color_icon = counter.stack[0].color_icon;
        constexpr Color color_icon = colors::COLOR_WHITE;
        const HandleOptional<Texture> texture_icon = counter_textures.ForIcon(counter.icon);
        if (texture_icon.IsValid()) {
            ui::DrawTexture(window_state, texture_icon.GetHandle(), area_icon, color_icon);
        } else {
            ui::DrawRect(window_state, area_icon, color_icon);
            if (font_22_opt.has_value()) {
                const AABB area_placeholder = AABB::FromPoint(counter_top_left + float2 { 0.0F, counter_size.y * 0.4F - static_cast<f32>(pt_22) * 0.3F }, counter_size);
                DrawText(font_22_opt.value(), counter.label_icon_placeholder, area_placeholder, colors::COLOR_BLACK);
            }
        }

        if (font_08_opt.has_value()) {
            const AABB area_echelon = area_icon_border.WithOffset(float2 { 0.0F, 0.03F * counter_size.y });
            DrawText(font_08_opt.value(), counter.label_echelon, area_echelon, colors::COLOR_BLACK, ui::TextAlignment::CENTER);
        }

        if (font_12_opt.has_value()) {
            const ui::AABBWithPadding area_steps { .area = AABB::FromPoint(counter_top_left + counter_size * float2 { 0.82F, 0.5F }, counter_size * float2 { 0.14F }), .padding = color_box_padding };
            const ui::ColorBox color_box_steps { .color_fill = colors::COLOR_WHITE_SMOKE, .color_stroke = colors::COLOR_ORANGE, .color_text = colors::COLOR_BLACK };
            DrawColorBox(window_state, font_12_opt.value(), counter.label_steps, area_steps, color_box_steps);
        }

        const ui::ColorBox color_box_move = MoveTypeToColorBox(MoveTypeUnitIcon(counter.icon));
        const ui::ColorBox color_box_ranged = RangedTypeToColorBox(counter.ranged_type);

        if (font_22_opt.has_value()) {
            const ui::AABBWithPadding area_dmg { .area = AABB::FromPoint(counter_top_left + counter_size * float2 { 0.03F, 0.725F }, counter_size * float2 { 0.25F }), .padding = color_box_padding };
            DrawColorBox(window_state, font_22_opt.value(), counter.label_dmg, area_dmg, color_box_ranged, ui::TextAlignment::LEFT);

            const ui::AABBWithPadding area_move_allowance { .area = AABB::FromPoint(counter_top_left + counter_size * float2 { 0.725F }, counter_size * float2 { 0.25F }), .padding = color_box_padding };
            DrawColorBox(window_state, font_22_opt.value(), counter.label_move_allowance, area_move_allowance, color_box_move);
        }

        if (font_16_opt.has_value()) {
            const ui::AABBWithPadding area_dmg_ranged { .area = AABB::FromPoint(counter_top_left + counter_size * float2 { 0.03F, 0.545F }, counter_size * float2 { 0.18F }), .padding = color_box_padding };
            DrawColorBox(window_state, font_16_opt.value(), counter.label_dmg_ranged, area_dmg_ranged, color_box_ranged);
        }
    }
}
} // namespace hex
