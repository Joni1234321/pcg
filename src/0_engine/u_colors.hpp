#pragma once

#include <SDL3/SDL.h>
#include <complex>

#include "u_texture.hpp"
#include "0_engine/u_types.hpp"

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

constexpr Color clear { 0, 0, 0, 0 };
constexpr Color black { 0, 0, 0, 255U };
constexpr Color white { 255U, 255U, 255U, 255U };

// Color palette
constexpr Color light_sky_blue { 135U, 206U, 250U, 255U };
constexpr Color dark_dark_brown { 62U, 68U, 43U, 255U };

constexpr Color red { 255U, 0, 0, 255U };
constexpr Color green { 0, 255U, 0, 255U };
constexpr Color blue { 0, 0, 255U, 255U };
constexpr Color yellow { 255U, 255U, 0, 255U };
constexpr Color cyan { 0, 255U, 255U, 255U };
constexpr Color magenta { 255U, 0, 255U, 255U };

constexpr Color gray { 128U, 128U, 128U, 255U };
constexpr Color mid_gray { 120U, 120U, 120U, 255U };
constexpr Color semi_mid_gray { 124U, 124U, 124U, 255U };
constexpr Color dark_gray { 64U, 64U, 64U, 255U };
constexpr Color light_gray { 192U, 192U, 192U, 255U };

constexpr Color orange { 255U, 165U, 0, 255U };
constexpr Color pink { 255U, 192U, 203U, 255U };
constexpr Color purple { 128U, 0, 128U, 255U };
constexpr Color brown { 165U, 42U, 42U, 255U };
constexpr Color gold { 255U, 215U, 0, 255U };

constexpr Color navy { 0, 0, 128U, 255U };
constexpr Color teal { 0, 128U, 128U, 255U };
constexpr Color olive { 128U, 128U, 0, 255U };
constexpr Color maroon { 128U, 0, 0, 255U };
constexpr Color lime { 50U, 205U, 50U, 255U };

constexpr Color sky_blue { 135U, 206U, 235U, 255U };
constexpr Color deep_sky_blue { 0, 191U, 255U, 255U };
constexpr Color royal_blue { 65U, 105U, 225U, 255U };
constexpr Color sea_green { 46U, 139U, 87U, 255U };

constexpr Color indigo { 75U, 0, 130U, 255U };
constexpr Color violet { 238U, 130U, 238U, 255U };
constexpr Color lavender { 230U, 230U, 250U, 255U };
constexpr Color beige { 245U, 245U, 220U, 255U };
constexpr Color ivory { 255U, 255U, 240U, 255U };
constexpr Color neon_blue { 0U, 128U, 255U, 255U };

constexpr Color chocolate { 210U, 105U, 30U, 255U };
constexpr Color coral { 255U, 127U, 80U, 255U };
constexpr Color salmon { 250U, 128U, 114U, 255U };
constexpr Color khaki { 240U, 230U, 140U, 255U };

constexpr Color deep_purple { 75, 0, 130, 255U };
constexpr Color radiant_orange { 255, 94, 58, 255U };
constexpr Color cool_teal { 32, 178, 170, 255U };
constexpr Color ruby_red { 224, 17, 95, 255U };
constexpr Color deep_gold { 218, 165, 32, 255U };
constexpr Color silver { 192, 192, 192, 255U };
constexpr Color navy_blue { 0, 0, 128, 255U };
constexpr Color white_smoke { 245, 245, 245, 255U };
constexpr Color midnight_navy { 25, 25, 112, 255U }; // Deep blue, immersive and sleek
constexpr Color cosmic_purple { 50, 0, 90, 255U };   // Deep purple with a space-like glow
constexpr Color dark_slate { 47, 79, 79, 255U };     // Cool, dark, and slightly muted teal

// CosmoClicker
constexpr Color faded_green { 204U, 238U, 204U, 255U }; // #CEC // close to tea green
constexpr Color gray_tint { 218U, 218U, 218U, 255U };   // #DA
constexpr Color dark_blue { 0, 0, 139, 255 };
constexpr Color dark_navy_blue { 0U, 0U, 107U, 255U };
constexpr Color dark_grey { 15U, 15U, 15U, 255U };
constexpr Color jet { 60, 55, 68, 255U };
constexpr Color cerulean { 0, 112, 144, 255U };

// Command Strike
constexpr Color soft_blue { 129U, 170U, 180U, 255U };   // #81AAB4 - Soft, muted blue-green
constexpr Color deep_teal { 0U, 85U, 102U, 255U };      // #005566 - Dark teal
constexpr Color dusk_purple { 88U, 66U, 124U, 255U };   // #58427C - Muted purple
constexpr Color warm_gray { 136U, 128U, 124U, 255U };   // #88807C - Neutral warm gray
constexpr Color ocean_blue { 0U, 105U, 148U, 255U };    // #006994 - Deep oceanic blue
constexpr Color burnt_orange { 208U, 107U, 5U, 255U };  // #D06B05 - Warm, earthy orange
constexpr Color royal_purple { 177U, 92U, 194U, 255U }; // #B15CC2 - Vibrant purple
constexpr Color forest_green { 75U, 133U, 57U, 255U };  // #4B8539 - Deep forest green
constexpr Color sky_cyan { 105U, 232U, 255U, 255U };    // #69E8FF - Bright cyan
constexpr Color charcoal_gray { 63U, 63U, 63U, 255U };  // #3F3F3F - Dark charcoal gray
constexpr Color steel_gray { 120U, 120U, 120U, 255U };  // #787878 - Mid-tone gray
constexpr Color ash_gray { 124U, 124U, 124U, 255U };    // #7C7C7C - Muted ash gray

// Hex Battle
constexpr Color hex_hover { 190U, 205U, 175U, 255U }; // muted sage-green hover ring
constexpr Color hex_select { 210U, 165U, 30U, 255U }; // warm amber-gold selection ring

inline Color AnimateFast(const f32 t) {
    const u8 red = static_cast<u8>((std::sin(t * 0.5F + 0.0F) * 0.5F + 0.5F) * 255U);
    const u8 green = static_cast<u8>((std::sin(t * 0.7F + 2.0F) * 0.5F + 0.5F) * 255U);
    const u8 blue = static_cast<u8>((std::sin(t * 1.1F + 4.0F) * 0.5F + 0.5F) * 255U);
    return Color { red, green, blue, 255U };
}
inline Color AnimateDamp(const f32 t) {
    constexpr float base = 127.0F;
    constexpr float amp = 64.0F;
    const u8 red = static_cast<u8>(base + amp * std::sin(t * 0.5F + 0.0F));
    const u8 green = static_cast<u8>(base + amp * std::sin(t * 0.7F + 2.0F));
    const u8 blue = static_cast<u8>(base + amp * std::sin(t * 1.1F + 4.0F));
    return Color { red, green, blue, 255U };
}
// NOLINTEND(*-use-designated-initializers)
}
