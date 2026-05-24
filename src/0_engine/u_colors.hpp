#pragma once

#include <complex>

#include "0_engine/u_types.hpp"
#include "u_texture.hpp"

namespace pce::colors {
// NOLINTBEGIN(*-use-designated-initializers)
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

constexpr Color CLEAR { 0, 0, 0, 0 };
constexpr Color BLACK { 0, 0, 0 };
constexpr Color WHITE { 255U, 255U, 255U };

// Color palette
constexpr Color LIGHT_SKY_BLUE { 135U, 206U, 250U };
constexpr Color DARK_DARK_BROWN { 62U, 68U, 43U };

constexpr Color RED { 255U, 0, 0 };
constexpr Color GREEN { 0, 255U, 0 };
constexpr Color BLUE { 0, 0, 255U };
constexpr Color YELLOW { 255U, 255U, 0 };
constexpr Color CYAN { 0, 255U, 255U };
constexpr Color MAGENTA { 255U, 0, 255U };

constexpr Color GRAY { 128U, 128U, 128U };
constexpr Color MID_GRAY { 120U, 120U, 120U };
constexpr Color SEMI_MID_GRAY { 124U, 124U, 124U };
constexpr Color DARK_GRAY { 64U, 64U, 64U };
constexpr Color LIGHT_GRAY { 192U, 192U, 192U };

constexpr Color ORANGE { 255U, 165U, 0 };
constexpr Color PINK { 255U, 192U, 203U };
constexpr Color PURPLE { 128U, 0, 128U };
constexpr Color BROWN { 165U, 42U, 42U };
constexpr Color GOLD { 255U, 215U, 0 };

constexpr Color NAVY { 0, 0, 128U };
constexpr Color TEAL { 0, 128U, 128U };
constexpr Color OLIVE { 128U, 128U, 0 };
constexpr Color MAROON { 128U, 0, 0 };
constexpr Color LIME { 50U, 205U, 50U };

constexpr Color SKY_BLUE { 135U, 206U, 235U };
constexpr Color DEEP_SKY_BLUE { 0, 191U, 255U };
constexpr Color ROYAL_BLUE { 65U, 105U, 225U };
constexpr Color SEA_GREEN { 46U, 139U, 87U };

constexpr Color INDIGO { 75U, 0, 130U };
constexpr Color VIOLET { 238U, 130U, 238U };
constexpr Color LAVENDER { 230U, 230U, 250U };
constexpr Color BEIGE { 245U, 245U, 220U };
constexpr Color IVORY { 255U, 255U, 240U };
constexpr Color NEON_BLUE { 0U, 128U, 255U };

constexpr Color CHOCOLATE { 210U, 105U, 30U };
constexpr Color CORAL { 255U, 127U, 80U };
constexpr Color SALMON { 250U, 128U, 114U };
constexpr Color KHAKI { 240U, 230U, 140U };

constexpr Color DEEP_PURPLE { 75, 0, 130 };
constexpr Color RADIANT_ORANGE { 255, 94, 58 };
constexpr Color COOL_TEAL { 32, 178, 170 };
constexpr Color RUBY_RED { 224, 17, 95 };
constexpr Color DEEP_GOLD { 218, 165, 32 };
constexpr Color SILVER { 192, 192, 192 };
constexpr Color NAVY_BLUE { 0, 0, 128 };
constexpr Color WHITE_SMOKE { 245, 245, 245 };
constexpr Color MIDNIGHT_NAVY { 25, 25, 112 }; // Deep blue, immersive and sleek
constexpr Color COSMIC_PURPLE { 50, 0, 90 };   // Deep purple with a space-like glow
constexpr Color DARK_SLATE { 47, 79, 79 };     // Cool, dark, and slightly muted teal

// CosmoClicker
constexpr Color FADED_GREEN { 204U, 238U, 204U }; // #CEC // close to tea green
constexpr Color GRAY_TINT { 218U, 218U, 218U };   // #DA
constexpr Color DARK_BLUE { 0, 0, 139 };
constexpr Color DARK_NAVY_BLUE { 0U, 0U, 107U };
constexpr Color DARK_GREY { 15U, 15U, 15U };
constexpr Color JET { 60, 55, 68 };
constexpr Color CERULEAN { 0, 112, 144 };

// Command Strike
constexpr Color SOFT_BLUE { 129U, 170U, 180U };   // #81AAB4 - Soft, muted blue-green
constexpr Color DEEP_TEAL { 0U, 85U, 102U };      // #005566 - Dark teal
constexpr Color DUSK_PURPLE { 88U, 66U, 124U };   // #58427C - Muted purple
constexpr Color WARM_GRAY { 136U, 128U, 124U };   // #88807C - Neutral warm gray
constexpr Color OCEAN_BLUE { 0U, 105U, 148U };    // #006994 - Deep oceanic blue
constexpr Color BURNT_ORANGE { 208U, 107U, 5U };  // #D06B05 - Warm, earthy orange
constexpr Color ROYAL_PURPLE { 177U, 92U, 194U }; // #B15CC2 - Vibrant purple
constexpr Color FOREST_GREEN { 75U, 133U, 57U };  // #4B8539 - Deep forest green
constexpr Color SKY_CYAN { 105U, 232U, 255U };    // #69E8FF - Bright cyan
constexpr Color CHARCOAL_GRAY { 63U, 63U, 63U };  // #3F3F3F - Dark charcoal gray
constexpr Color STEEL_GRAY { 120U, 120U, 120U };  // #787878 - Mid-tone gray
constexpr Color ASH_GRAY { 124U, 124U, 124U };    // #7C7C7C - Muted ash gray

// Hex Battle
constexpr Color HEX_HOVER { 190U, 205U, 175U }; // muted sage-green hover ring
constexpr Color HEX_SELECT { 210U, 165U, 30U }; // warm amber-gold selection ring

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
// NOLINTEND(*-use-designated-initializers)
} // namespace pce::colors
