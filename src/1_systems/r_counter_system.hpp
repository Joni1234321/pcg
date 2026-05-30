#pragma once
#include <SDL3/SDL_render.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <functional>
#include <optional>
#include <utility>

#include "0_engine/g_globals.hpp"
#include "0_engine/r_window_state.hpp"
#include "0_engine/u_assets.hpp"
#include "0_engine/u_collections.hpp"
#include "0_engine/u_colors.hpp"
#include "0_engine/u_fonts.hpp"
#include "0_engine/u_texture.hpp"
#include "r_camera_system.hpp"
#include "r_hex_system.hpp"

namespace pce {
enum class Echelon : u8 { ECHELON_SQUAD, ECHELON_PLATOON, ECHELON_COMPANY, ECHELON_BATTALION, ECHELON_REGIMENT, ECHELON_BRIGADE, ECHELON_DIVISION, ECHELON_CORPS, ECHELON_ARMY };
enum class UnitIcon : u8 { ICON_INF, ICON_ART, ICON_HQ, ICON_TANK };

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
    ;
}

[[nodiscard]] constexpr String UnitIconToString(const UnitIcon icon) {
    switch (icon) {
        case UnitIcon::ICON_INF: return "inf";
        case UnitIcon::ICON_ART: return "art";
        case UnitIcon::ICON_HQ: return "hq";
        case UnitIcon::ICON_TANK: return "tnk";
    }
    std::unreachable();
    ;
}
struct Counter {
    Color color_background;
    Color color_icon;
    Color color_border;
};
struct CounterStack {
    int2 axial { };
    UnitIcon icon { };
    Array<Counter, 12> stack { };
    Label label_top;
    Label label_center;
    Label label_bottom;
    SurfaceLabel label_vertical;
};

struct CounterTextures {
    HandleOptional<Texture> infantry;
    HandleOptional<Texture> artillery;
    HandleOptional<Texture> armor;
    HandleOptional<Texture> headquarters;

    explicit CounterTextures(const RelativePath& dir) {
        infantry = globalData.Create<Texture>(Asset(dir / "counter-inf.jpg"));
        artillery = globalData.Create<Texture>(Asset(dir / "counter-art.jpg"));
        armor = globalData.Create<Texture>(Asset(dir / "counter-armor.jpg"));
        headquarters = globalData.Create<Texture>(Asset(dir / "counter-hq.jpg"));
    }
    [[nodiscard]] HandleOptional<Texture> ForIcon(const UnitIcon icon) const {
        switch (icon) {
            case UnitIcon::ICON_INF: return infantry;
            case UnitIcon::ICON_ART: return artillery;
            case UnitIcon::ICON_TANK: return armor;
            case UnitIcon::ICON_HQ: return headquarters;
        }
        std::unreachable();
    }
};
struct CounterTextureStack {
    CounterTextures counter_textures_niehorster { "counters/counter-niehorster" };
    CounterTextures counter_textures_niehorster_big { "counters/counter-niehorster-big" };
    CounterTextures counter_textures_real { "counters/counter-real" };
};

inline void RenderCounters(const Pool<CounterStack>& counters) {
    const CameraState& camera = Singleton::Get<CameraState>();
    const WindowState& window_state = Singleton::Get<WindowState>();
    const ui::FontCollection& font_collection = Singleton::Get<ui::FontCollection>();
    const CounterTextures& counter_textures = Singleton::Get<CounterTextureStack>().counter_textures_real;

    constexpr f32 COUNTER_SIZE = 1.1F;
    const float2 counter_size = float2 { camera.scale * COUNTER_SIZE } * float2 { 1.0F, 0.8F };

    const ui::FontSize pt_normal = static_cast<ui::FontSize>(counter_size.y * 0.25F);
    const ui::FontSize pt_small = static_cast<ui::FontSize>(counter_size.y * 0.15F);
    // Pre-warm the font cache for both sizes before taking any references. Since second get can cause move interannly
    if (pt_normal >= ui::FONT_MIN_SIZE) { (void)font_collection.GetFontBold(static_cast<ui::FontSizes>(pt_normal)); }
    if (pt_small >= ui::FONT_MIN_SIZE) { (void)font_collection.GetFontBold(static_cast<ui::FontSizes>(pt_small)); }
    const Optional<std::reference_wrapper<const ui::Font>> font_normal_opt = pt_normal < ui::FONT_MIN_SIZE ? Optional<std::reference_wrapper<const ui::Font>> { std::nullopt } : font_collection.GetFontBold(static_cast<ui::FontSizes>(pt_normal));
    const Optional<std::reference_wrapper<const ui::Font>> font_small_opt = pt_small < ui::FONT_MIN_SIZE ? Optional<std::reference_wrapper<const ui::Font>> { std::nullopt } : font_collection.GetFontBold(static_cast<ui::FontSizes>(pt_small));

    for (const CounterStack& counter : counters) {
        const float2 world = HexAxialToWorld(counter.axial);
        const float2 counter_center = camera.WorldToScreen(world);
        const float2 counter_point = counter_center - counter_size * float2 { 0.5F };

        // draw static counters
        for (i32 i = static_cast<i32>(counter.stack.size()) - 1; i >= 0; i--) {
            const Counter& counter_stack = counter.stack[static_cast<u32>(i)];
            if (counter_stack.color_icon.a == 0) { continue; }

            constexpr f32 OFFSET_STACK = 1.0F / 16.0F;
            constexpr f32 OFFSET_SHADOW = OFFSET_STACK * 0.15F;
            constexpr f32 BORDER_THICKNESS = OFFSET_STACK * 0.30F;

            const AABBF area_counter = AABBF::FromCenter(counter_center + float2 { OFFSET_STACK * counter_size.x * static_cast<f32>(i) }, counter_size);

            // shadow
            const AABBF area_shadow = area_counter.WithOffset(float2 { OFFSET_SHADOW * counter_size.x });
            constexpr Color COLOR_SHADOW = colors::ColorWithAlpha(colors::BLACK, 0.5F);
            (void)SDL_SetRenderDrawColor(window_state.renderer, COLOR_SHADOW.r, COLOR_SHADOW.g, COLOR_SHADOW.b, COLOR_SHADOW.a);
            (void)SDL_RenderFillRect(window_state.renderer, area_shadow);

            // border
            (void)SDL_SetRenderDrawColor(window_state.renderer, counter_stack.color_border.r, counter_stack.color_border.g, counter_stack.color_border.b, counter_stack.color_border.a);
            (void)SDL_RenderFillRect(window_state.renderer, area_counter);

            // background
            const AABBF area_background = area_counter.WithPadding(float2 { BORDER_THICKNESS * counter_size.x });
            (void)SDL_SetRenderDrawColor(window_state.renderer, counter_stack.color_background.r, counter_stack.color_background.g, counter_stack.color_background.b, counter_stack.color_background.a);
            (void)SDL_RenderFillRect(window_state.renderer, area_background);
        }

        // area
        const AABBF area_icon_border = AABBF::FromCenter(counter_center, counter_size * float2 { 0.6F, 0.4F }).WithOffset(counter_size * float2 { 0.0F, -0.05F });
        constexpr Color COLOR_ICON_BORDER { colors::BLACK };
        (void)SDL_SetRenderDrawColor(window_state.renderer, COLOR_ICON_BORDER.r, COLOR_ICON_BORDER.g, COLOR_ICON_BORDER.b, COLOR_ICON_BORDER.a);
        (void)SDL_RenderFillRect(window_state.renderer, area_icon_border);

        const AABBF area_icon = area_icon_border.WithPadding(float2 { camera.scale / 40.0F });
        const Color color_icon = counter.stack[0].color_icon;
        const HandleOptional<Texture> texture_handle = counter_textures.ForIcon(counter.icon);
        SDL_Texture* icon_texture = texture_handle.IsValid() ? globalData[texture_handle.GetHandle()].ToSDL() : nullptr;
        if (icon_texture) {
            (void)SDL_SetTextureColorMod(icon_texture, color_icon.r, color_icon.g, color_icon.b);
            (void)SDL_SetTextureAlphaMod(icon_texture, color_icon.a);
            (void)SDL_RenderTexture(window_state.renderer, icon_texture, nullptr, area_icon);
        } else {
            (void)SDL_SetRenderDrawColor(window_state.renderer, color_icon.r, color_icon.g, color_icon.b, color_icon.a);
            (void)SDL_RenderFillRect(window_state.renderer, area_icon);
            if (font_normal_opt.has_value()) {
                const ui::Font& font_normal = font_normal_opt.value();
                (void)TTF_SetTextFont(counter.label_center, font_normal);
                (void)TTF_SetTextColorFloat(counter.label_center, 0.0F, 0.0F, 0.0F, 1.0F);
                (void)TTF_SetTextWrapWidth(counter.label_center, static_cast<i32>(counter_size.x));
                (void)TTF_DrawRendererText(counter.label_center, counter_point.x, counter_point.y + counter_size.y * 0.4F - static_cast<f32>(pt_normal) * 0.3F);
            }
        }

        //labels
        if (font_normal_opt.has_value()) {
            const ui::Font& font_normal = font_normal_opt.value();
            TTF_SetFontWrapAlignment(font_normal, TTF_HORIZONTAL_ALIGN_CENTER);

            (void)TTF_SetTextFont(counter.label_bottom, font_normal);
            // (void)TTF_SetTextColorFloat(counter.label_bottom, 0.0F, 0.0F, 0.0F, 1.0F);
            (void)TTF_SetTextWrapWidth(counter.label_bottom, static_cast<i32>(counter_size.x));
            (void)TTF_DrawRendererText(counter.label_bottom, counter_point.x, counter_point.y + counter_size.y - static_cast<f32>(pt_normal));

            (void)TTF_SetTextFont(counter.label_top, font_normal);
            // (void)TTF_SetTextColorFloat(counter.label_top, 0.0F, 0.0F, 0.0F, 1.0F);
            (void)TTF_SetTextWrapWidth(counter.label_top, static_cast<i32>(counter_size.x));
            (void)TTF_DrawRendererText(counter.label_top, counter_point.x, counter_point.y);
        }

        if (font_small_opt.has_value()) {
            const ui::Font& font_small = font_small_opt.value();
            // surface bc then i draw vertically
            (void)TTF_SetTextFont(counter.label_vertical, font_small);
            (void)TTF_SetTextWrapWidth(counter.label_vertical, static_cast<i32>(counter_size.y));
            SDL_Surface* surface = SDL_CreateSurface(static_cast<i32>(counter_size.y), pt_small, SDL_PIXELFORMAT_ARGB8888);
            if (surface) {
                (void)TTF_DrawSurfaceText(counter.label_vertical, 0, 0, surface);
                SDL_Texture* tex = SDL_CreateTextureFromSurface(window_state.renderer, surface);
                SDL_DestroySurface(surface);
                if (tex) {
                    const AABBF dst = AABBF::FromCenter(float2 { counter_point.x + counter_size.x - static_cast<f32>(pt_small) * 0.5F, counter_center.y }, float2 { counter_size.y, static_cast<f32>(pt_small) });
                    constexpr f32 ANGLE_DEGREES = 90.0;
                    SDL_RenderTextureRotated(window_state.renderer, tex, nullptr, dst, ANGLE_DEGREES, nullptr, SDL_FLIP_NONE);
                    SDL_DestroyTexture(tex);
                }
            }
        }
    }
}
} // namespace pce
