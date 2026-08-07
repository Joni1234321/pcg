module;

#include <SDL3/SDL_render.h>
#include <SDL3/SDL_surface.h>
#include <SDL3_ttf/SDL_ttf.h>

export module hex.counter;

import std;

import pce.std;
import pce.globals;
import pce.window_state;
import pce.assets;
import pce.font;
import pce.sdl;
import pcs.camera;
import pce.collections;
import pce.colors;

import hex.hex;
import hex.types;
import hex.terrain;

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
struct ColorBox {
    Color color_fill { };
    Color color_stroke { };
    Color color_text { };
};
[[nodiscard]] constexpr ColorBox MoveTypeToColorBox(const MoveType move_type) {
    switch (move_type) {
        case MoveType::MOVE_LEG: return ColorBox { .color_fill = colors::COLOR_SOFT_BLUE, .color_stroke = colors::COLOR_BLACK, .color_text = colors::COLOR_BLACK };
        case MoveType::MOVE_TAC: return ColorBox { .color_fill = colors::COLOR_CRIMSON, .color_stroke = colors::COLOR_BLACK, .color_text = colors::COLOR_WHITE_SMOKE_2 };
        case MoveType::MOVE_TRUCK: return ColorBox { .color_fill = colors::COLOR_SEA_GREEN, .color_stroke = colors::COLOR_BLACK, .color_text = colors::COLOR_WHITE_SMOKE_2 };
    }
    std::unreachable();
}

[[nodiscard]] constexpr ColorBox RangedTypeToColorBox(const RangedType ranged_type) {
    switch (ranged_type) {
        case RangedType::RANGED_NONE: return ColorBox { .color_fill = colors::COLOR_CLEAR, .color_stroke = colors::COLOR_CLEAR, .color_text = colors::COLOR_CLEAR };
        case RangedType::RANGED_DEFENSE: return ColorBox { .color_fill = colors::COLOR_BLACK, .color_stroke = colors::COLOR_ASH_GRAY, .color_text = colors::COLOR_WHITE_SMOKE_2 };
        case RangedType::RANGED_ATTACK: return ColorBox { .color_fill = colors::COLOR_CRIMSON, .color_stroke = colors::COLOR_BLACK, .color_text = colors::COLOR_WHITE_SMOKE_2 };
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

inline void RenderCounters(const Pool<CounterStack>& counters) {
    const CameraState& camera = Singleton::Get<CameraState>();
    const WindowState& window_state = Singleton::Get<WindowState>();
    const ui::FontCollection& font_collection = Singleton::Get<ui::FontCollection>();
    const CounterTextures& counter_textures = CounterStyleToTextures(COUNTER_THEME);

    constexpr f32 COUNTER_SIZE = 1.1F;
    const float2 counter_size = float2 { camera.scale * COUNTER_SIZE };

    const ui::FontSize pt_32 = static_cast<ui::FontSize>(counter_size.y * 0.32F);
    const ui::FontSize pt_22 = static_cast<ui::FontSize>(counter_size.y * 0.22F);
    const ui::FontSize pt_14 = static_cast<ui::FontSize>(counter_size.y * 0.10F);
    const ui::FontSize pt_16 = static_cast<ui::FontSize>(counter_size.y * 0.16F);
    const ui::FontSize pt_12 = static_cast<ui::FontSize>(counter_size.y * 0.12F);
    const ui::FontSize pt_10 = static_cast<ui::FontSize>(counter_size.y * 0.10F);
    const ui::FontSize pt_08 = static_cast<ui::FontSize>(counter_size.y * 0.08F);
    const Optional<std::reference_wrapper<const ui::Font>> font_32_opt = pt_32 < ui::FONT_MIN_SIZE ? Optional<std::reference_wrapper<const ui::Font>> { std::nullopt } : font_collection.GetFontBoldCompact(static_cast<ui::FontSizes>(pt_32));
    const Optional<std::reference_wrapper<const ui::Font>> font_22_opt = pt_22 < ui::FONT_MIN_SIZE ? Optional<std::reference_wrapper<const ui::Font>> { std::nullopt } : font_collection.GetFontBoldCompact(static_cast<ui::FontSizes>(pt_22));
    const Optional<std::reference_wrapper<const ui::Font>> font_14_opt = pt_14 < ui::FONT_MIN_SIZE ? Optional<std::reference_wrapper<const ui::Font>> { std::nullopt } : font_collection.GetFontBoldCourier(static_cast<ui::FontSizes>(pt_14));
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

            constexpr f32 OFFSET_STACK = 1.0F / 16.0F;
            constexpr f32 OFFSET_SHADOW = OFFSET_STACK * 0.15F;
            constexpr f32 BORDER_THICKNESS = OFFSET_STACK * 0.30F;

            const AABB area_counter = AABB::FromCenter(counter_center + float2 { OFFSET_STACK * counter_size.x * static_cast<f32>(i) }, counter_size);

            const AABB area_shadow = area_counter.WithOffset(float2 { OFFSET_SHADOW * counter_size.x });
            constexpr Color COLOR_SHADOW = colors::ColorWithAlpha(colors::COLOR_BLACK, 0.5F);
            (void)SDL_SetRenderDrawColor(window_state.renderer, COLOR_SHADOW.r, COLOR_SHADOW.g, COLOR_SHADOW.b, COLOR_SHADOW.a);
            (void)SDL_RenderFillRect(window_state.renderer, area_shadow);

            (void)SDL_SetRenderDrawColor(window_state.renderer, counter_stack.color_border.r, counter_stack.color_border.g, counter_stack.color_border.b, counter_stack.color_border.a);
            (void)SDL_RenderFillRect(window_state.renderer, area_counter);

            const AABB area_background = area_counter.WithPadding(float2 { BORDER_THICKNESS * counter_size.x });
            (void)SDL_SetRenderDrawColor(window_state.renderer, counter_stack.color_background.r, counter_stack.color_background.g, counter_stack.color_background.b, counter_stack.color_background.a);
            (void)SDL_RenderFillRect(window_state.renderer, area_background);
        }

        auto draw_color_box = [&](const ui::Font& font, const Label& label, const ColorBox& color_box, AABB area_stroke) {
            const AABB area_fill = area_stroke.WithPadding(counter_size * float2 { 0.01F });

            Color color = color_box.color_stroke;
            if (color.a > 0) {
                (void)SDL_SetRenderDrawColor(window_state.renderer, color.r, color.g, color.b, color.a);
                (void)SDL_RenderFillRect(window_state.renderer, area_stroke);
            }

            color = color_box.color_fill;
            if (color.a > 0) {
                (void)SDL_SetRenderDrawColor(window_state.renderer, color.r, color.g, color.b, color.a);
                (void)SDL_RenderFillRect(window_state.renderer, area_fill);
            }

            color = color_box.color_text;
            if (color.a > 0) {
                TTF_SetFontWrapAlignment(font, TTF_HORIZONTAL_ALIGN_RIGHT);
                (void)TTF_SetTextFont(label, font);
                (void)TTF_SetTextColor(label, color.r, color.g, color.b, color.a);
                (void)TTF_SetTextWrapWidth(label, area_fill.size.x);
                (void)TTF_DrawRendererText(label, area_fill.point.x, area_stroke.point.y);
            }
        };

        // div
        if (font_14_opt.has_value()) {
            {
                AABB area_div = AABB::FromPoint(counter_top_left + counter_size * float2 { 0.0F, 0.09F }, counter_size * float2 { 1.0F, 0.25F });
                ColorBox color_box { .color_fill = counter.stack[0].color_icon, .color_stroke = colors::COLOR_BLACK, .color_text = colors::COLOR_BLACK };
                draw_color_box(font_14_opt.value(), counter.label_name_div, color_box, area_div);
            }
        }
        else {
            AABB area_div = AABB::FromPoint(counter_top_left + counter_size * float2 { 0.0F, 0.09F }, counter_size * float2 { 1.0F, 0.5F });
            Color color = counter.stack[0].color_icon;
            (void)SDL_SetRenderDrawColor(window_state.renderer, color.r, color.g, color.b, color.a);
            (void)SDL_RenderFillRect(window_state.renderer, area_div);
        }

        if (font_10_opt.has_value()) {
            {
                AABB area_div = AABB::FromPoint(counter_top_left + counter_size * float2 { 0.0F, 0.02F }, counter_size * float2 { 0.97F, 0.12F });
                ColorBox color_box { .color_fill = colors::COLOR_CLEAR, .color_stroke = colors::COLOR_CLEAR, .color_text = colors::COLOR_BLACK };
                draw_color_box(font_10_opt.value(), counter.label_name_sub, color_box, area_div);
            }
        }

        // icon
        const AABB area_icon_border = AABB::FromPoint(counter_top_left, counter_size * float2 { 0.6F, 0.4F }).WithOffset(counter_size * float2 { 0.02F });
        constexpr Color COLOR_ICON_BORDER { colors::COLOR_BLACK };
        (void)SDL_SetRenderDrawColor(window_state.renderer, COLOR_ICON_BORDER.r, COLOR_ICON_BORDER.g, COLOR_ICON_BORDER.b, COLOR_ICON_BORDER.a);
        // (void)SDL_RenderFillRect(window_state.renderer, area_icon_border);

        const AABB area_icon = area_icon_border.WithPadding(float2 { camera.scale / 40.0F });
        // const Color color_icon = counter.stack[0].color_icon;
        const Color color_icon = colors::COLOR_WHITE;
        const HandleOptional<Texture> texture_handle = counter_textures.ForIcon(counter.icon);
        SDL_Texture* icon_texture = texture_handle.IsValid() ? globalData[texture_handle.GetHandle()] : nullptr;
        if (icon_texture) {
            (void)SDL_SetTextureColorMod(icon_texture, color_icon.r, color_icon.g, color_icon.b);
            (void)SDL_SetTextureAlphaMod(icon_texture, color_icon.a);
            (void)SDL_RenderTexture(window_state.renderer, icon_texture, nullptr, area_icon);
        } else {
            (void)SDL_SetRenderDrawColor(window_state.renderer, color_icon.r, color_icon.g, color_icon.b, color_icon.a);
            (void)SDL_RenderFillRect(window_state.renderer, area_icon);
            if (font_22_opt.has_value()) {
                const ui::Font& font_22 = font_22_opt.value();
                (void)TTF_SetTextFont(counter.label_icon_placeholder, font_22);
                (void)TTF_SetTextColorFloat(counter.label_icon_placeholder, 0.0F, 0.0F, 0.0F, 1.0F);
                (void)TTF_SetTextWrapWidth(counter.label_icon_placeholder, static_cast<i32>(counter_size.x));
                (void)TTF_DrawRendererText(counter.label_icon_placeholder, counter_top_left.x, counter_top_left.y + counter_size.y * 0.4F - static_cast<f32>(pt_22) * 0.3F);
            }
        }

        if (font_08_opt.has_value()) {
            const ui::Font& font_08 = font_08_opt.value();

            TTF_SetFontWrapAlignment(font_08, TTF_HORIZONTAL_ALIGN_CENTER);
            (void)TTF_SetTextColorFloat(counter.label_echelon, 0.0F, 0.0F, 0.0F, 1.0F);
            (void)TTF_SetTextFont(counter.label_echelon, font_08);
            (void)TTF_SetTextWrapWidth(counter.label_echelon, area_icon_border.size.x);
            (void)TTF_DrawRendererText(counter.label_echelon, area_icon_border.point.x, area_icon_border.point.y + 0.03F * counter_size.y);
        }

        if (font_12_opt.has_value()) {
            const AABB area_stroke = AABB::FromPoint(counter_top_left + counter_size * float2 { 0.82F, 0.56F }, counter_size * float2 { 0.14F });
            constexpr ColorBox COLOR_BOX_STEPS = { .color_fill = colors::COLOR_WHITE_SMOKE, .color_stroke = colors::COLOR_ORANGE, .color_text = colors::COLOR_BLACK };
            draw_color_box(font_12_opt.value(), counter.label_steps, COLOR_BOX_STEPS, area_stroke);
        }

        if (font_22_opt.has_value()) {
            {
                const AABB area_stroke = AABB::FromPoint(counter_top_left + counter_size * float2 { 0.0F, 0.90F }, counter_size * float2 { 1.00F, 0.15F });
                ColorBox color_box = MoveTypeToColorBox(MoveTypeUnitIcon(counter.icon));
                color_box.color_text = colors::COLOR_CLEAR;
                draw_color_box(font_22_opt.value(), counter.label_move_allowance, color_box, area_stroke);
            }
            {
                const AABB area_stroke = AABB::FromPoint(counter_top_left + counter_size * float2 { 0.725F }, counter_size * float2 { 0.25F, 0.25F });
                ColorBox color_box = MoveTypeToColorBox(MoveTypeUnitIcon(counter.icon));
                color_box.color_text = colors::COLOR_RED;
                color_box.color_fill = colors::COLOR_CLEAR;
                color_box.color_stroke = colors::COLOR_CLEAR;
                draw_color_box(font_22_opt.value(), counter.label_move_allowance, color_box, area_stroke);
            }


            {
                const AABB area_stroke = AABB::FromPoint(counter_top_left + counter_size * float2 { 0.03F, 0.725F }, counter_size * float2 { 0.25F });
                ColorBox color_box = MoveTypeToColorBox(MoveTypeUnitIcon(counter.icon));
                color_box.color_fill = colors::COLOR_CLEAR;
                color_box.color_stroke = colors::COLOR_CLEAR;
                draw_color_box(font_22_opt.value(), counter.label_dmg, color_box, area_stroke);
            }

            {
                const AABB area_stroke = AABB::FromPoint(counter_top_left + counter_size * float2 { 0.03F, 0.53F }, counter_size * float2 { 0.35F, 0.15F });
                ColorBox color_box = RangedTypeToColorBox(RangedTypeUnitIcon(counter.icon));
                color_box.color_text = colors::COLOR_CLEAR;
                draw_color_box(font_22_opt.value(), counter.label_dmg_ranged, color_box, area_stroke);
            }
            {
                const AABB area_stroke = AABB::FromPoint(counter_top_left + counter_size * float2 { 0.03F, 0.43F }, counter_size * float2 { 0.25F });
                ColorBox color_box = RangedTypeToColorBox(RangedTypeUnitIcon(counter.icon));
                color_box.color_fill = colors::COLOR_CLEAR;
                color_box.color_stroke = colors::COLOR_CLEAR;
                draw_color_box(font_22_opt.value(), counter.label_dmg_ranged, color_box, area_stroke);
            }

        }
    }
}
} // namespace hex
