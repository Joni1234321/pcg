module;

#include "0_engine/u_texture.hpp"
#include "0_engine/u_util.hpp"

export module pce.colors;

import pce.std;

export namespace pce::colors {
constexpr Color ColorLighten(const Color color, const f32 factor) {
    auto lerp = [factor](const u8 k) -> u8 { return static_cast<u8>(k + (255 - k) * factor); };
    return Color { lerp(color.r), lerp(color.g), lerp(color.b), color.a };
}
constexpr ColorF ColorLighten(const ColorF color, const f32 factor) {
    auto lerp = [factor](const f32 k) -> f32 { return k + (1.0F - k) * factor; };
    return ColorF { lerp(color.r), lerp(color.g), lerp(color.b), color.a };
}
constexpr ColorF ColorMul(const ColorF color, const f32 factor) { return ColorF { color.r * factor, color.g * factor, color.b * factor, color.a }; }
constexpr Color ColorWithAlpha(Color color, const f32 a) {
    color.a = static_cast<u8>(a * 255U);
    return color;
}

constexpr Color COLOR_CLEAR { 0, 0, 0, 0 };
constexpr Color COLOR_BLACK { 0, 0, 0 };
constexpr Color COLOR_WHITE { 255U, 255U, 255U };

constexpr Color COLOR_LIGHT_SKY_BLUE { 135U, 206U, 250U };
constexpr Color COLOR_DARK_DARK_BROWN { 62U, 68U, 43U };

constexpr Color COLOR_RED { 255U, 0, 0 };
constexpr Color COLOR_GREEN { 0, 255U, 0 };
constexpr Color COLOR_BLUE { 0, 0, 255U };
constexpr Color COLOR_YELLOW { 255U, 255U, 0 };
constexpr Color COLOR_CYAN { 0, 255U, 255U };
constexpr Color COLOR_MAGENTA { 255U, 0, 255U };

constexpr Color COLOR_GRAY { 128U, 128U, 128U };
constexpr Color COLOR_MID_GRAY { 120U, 120U, 120U };
constexpr Color COLOR_SEMI_MID_GRAY { 124U, 124U, 124U };
constexpr Color COLOR_DARK_GRAY { 64U, 64U, 64U };
constexpr Color COLOR_LIGHT_GRAY { 192U, 192U, 192U };

constexpr Color COLOR_ORANGE { 255U, 165U, 0 };
constexpr Color COLOR_PINK { 255U, 192U, 203U };
constexpr Color COLOR_PURPLE { 128U, 0, 128U };
constexpr Color COLOR_BROWN { 165U, 42U, 42U };
constexpr Color COLOR_GOLD { 255U, 215U, 0 };

constexpr Color COLOR_NAVY { 0, 0, 128U };
constexpr Color COLOR_TEAL { 0, 128U, 128U };
constexpr Color COLOR_OLIVE { 128U, 128U, 0 };
constexpr Color COLOR_MAROON { 128U, 0, 0 };
constexpr Color COLOR_LIME { 50U, 205U, 50U };

constexpr Color COLOR_SKY_BLUE { 135U, 206U, 235U };
constexpr Color COLOR_DEEP_SKY_BLUE { 0, 191U, 255U };
constexpr Color COLOR_ROYAL_BLUE { 65U, 105U, 225U };
constexpr Color COLOR_SEA_GREEN { 46U, 139U, 87U };

constexpr Color COLOR_INDIGO { 75U, 0, 130U };
constexpr Color COLOR_VIOLET { 238U, 130U, 238U };
constexpr Color COLOR_LAVENDER { 230U, 230U, 250U };
constexpr Color COLOR_BEIGE { 245U, 245U, 220U };
constexpr Color COLOR_IVORY { 255U, 255U, 240U };
constexpr Color COLOR_NEON_BLUE { 0U, 128U, 255U };

constexpr Color COLOR_CHOCOLATE { 210U, 105U, 30U };
constexpr Color COLOR_CORAL { 255U, 127U, 80U };
constexpr Color COLOR_SALMON { 250U, 128U, 114U };
constexpr Color COLOR_KHAKI { 240U, 230U, 140U };

constexpr Color COLOR_DEEP_PURPLE { 75, 0, 130 };
constexpr Color COLOR_RADIANT_ORANGE { 255, 94, 58 };
constexpr Color COLOR_COOL_TEAL { 32, 178, 170 };
constexpr Color COLOR_RUBY_RED { 224, 17, 95 };
constexpr Color COLOR_DEEP_GOLD { 218, 165, 32 };
constexpr Color COLOR_SILVER { 192, 192, 192 };
constexpr Color COLOR_NAVY_BLUE { 0, 0, 128 };
constexpr Color COLOR_WHITE_SMOKE { 245, 245, 245 };
constexpr Color COLOR_MIDNIGHT_NAVY { 25, 25, 112 };
constexpr Color COLOR_COSMIC_PURPLE { 50, 0, 90 };
constexpr Color COLOR_DARK_SLATE { 47, 79, 79 };

constexpr Color COLOR_FADED_GREEN { 204U, 238U, 204U };
constexpr Color COLOR_GRAY_TINT { 218U, 218U, 218U };
constexpr Color COLOR_DARK_BLUE { 0, 0, 139 };
constexpr Color COLOR_DARK_NAVY_BLUE { 0U, 0U, 107U };
constexpr Color COLOR_DARK_GREY { 15U, 15U, 15U };
constexpr Color COLOR_JET { 60, 55, 68 };
constexpr Color COLOR_CERULEAN { 0, 112, 144 };

constexpr Color COLOR_SOFT_BLUE { 129U, 170U, 180U };
constexpr Color COLOR_DEEP_TEAL { 0U, 85U, 102U };
constexpr Color COLOR_DUSK_PURPLE { 88U, 66U, 124U };
constexpr Color COLOR_WARM_GRAY { 136U, 128U, 124U };
constexpr Color COLOR_OCEAN_BLUE { 0U, 105U, 148U };
constexpr Color COLOR_BURNT_ORANGE { 208U, 107U, 5U };
constexpr Color COLOR_ROYAL_PURPLE { 177U, 92U, 194U };
constexpr Color COLOR_FOREST_GREEN { 75U, 133U, 57U };
constexpr Color COLOR_SKY_CYAN { 105U, 232U, 255U };
constexpr Color COLOR_CHARCOAL_GRAY { 63U, 63U, 63U };
constexpr Color COLOR_STEEL_GRAY { 120U, 120U, 120U };
constexpr Color COLOR_ASH_GRAY { 124U, 124U, 124U };

constexpr Color COLOR_DARK_GREEN { 0U, 110U, 60U };
constexpr Color COLOR_MAP_BACKGROUND = Color::FromHsl(40.0F, 0.50F, 0.2F);
constexpr Color COLOR_HEX_HOVER { 190U, 205U, 175U };
constexpr Color COLOR_HEX_SELECT { 210U, 165U, 30U };

constexpr Color COLOR_ROAD_TAN { 196U, 168U, 120U };
constexpr Color COLOR_ROAD_CASING_BROWN { 48U, 32U, 18U };
constexpr Color COLOR_ROAD_GREY { 70U, 65U, 58U };
constexpr Color COLOR_RIVER_BLUE { 78U, 138U, 196U };
constexpr Color COLOR_RIVER_DEEP_BLUE { 22U, 52U, 92U };
constexpr Color COLOR_RIVER_HIGHLIGHT_BLUE { 168U, 206U, 236U };

constexpr Color COLOR_FEATURE_CITY { 140U, 25U, 25U };
constexpr Color COLOR_FEATURE_VILLAGE { 120U, 75U, 45U };
constexpr Color COLOR_FEATURE_WOODED_LIGHT { 150U, 180U, 70U };
constexpr Color COLOR_FEATURE_WOODED_HEAVY { 20U, 70U, 30U };
constexpr Color COLOR_FEATURE_FIELD { 205U, 175U, 95U };
constexpr Color COLOR_FEATURE_MARSH { 95U, 115U, 85U };

constexpr f32 COUNTRY_LUMINANCE = 0.3F;
constexpr Color COLOR_WG_GER_BG = Color::FromHsl(90.0F, 0.20F, COUNTRY_LUMINANCE);
constexpr Color COLOR_WG_SOV_BG = Color::FromHsl(2.0F, 0.60F, COUNTRY_LUMINANCE);
constexpr Color COLOR_WG_USA_BG = Color::FromHsl(218.0F, 0.60F, COUNTRY_LUMINANCE);

inline Color AnimateFast(const f32 t) {
    const u8 red = static_cast<u8>((std::sin(t * 0.5F + 0.0F) * 0.5F + 0.5F) * 255U);
    const u8 green = static_cast<u8>((std::sin(t * 0.7F + 2.0F) * 0.5F + 0.5F) * 255U);
    const u8 blue = static_cast<u8>((std::sin(t * 1.1F + 4.0F) * 0.5F + 0.5F) * 255U);
    return Color { red, green, blue };
}
inline Color AnimateDamp(const f32 t) {
    constexpr float base = 127.0F;
    constexpr float amp = 64.0F;
    const u8 red = static_cast<u8>(base + amp * std::sin(t * 0.5F + 0.0F));
    const u8 green = static_cast<u8>(base + amp * std::sin(t * 0.7F + 2.0F));
    const u8 blue = static_cast<u8>(base + amp * std::sin(t * 1.1F + 4.0F));
    return Color { red, green, blue };
}
} // namespace pce::colors
