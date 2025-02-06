#pragma once

#include <SDL3/SDL_init.h>

namespace pce::colors {
constexpr SDL_Color clear { 0, 0, 0, 0 };
constexpr SDL_Color black { 0, 0, 0, 255U };
constexpr SDL_Color white { 255U, 255U, 255U, 255U };

// Color palette
constexpr SDL_Color light_sky_blue { 135U, 206U, 250U, 255U };
constexpr SDL_Color dark_dark_brown { 62U, 68U, 43U, 255U };

constexpr SDL_Color red { 255U, 0, 0, 255U };
constexpr SDL_Color green { 0, 255U, 0, 255U };
constexpr SDL_Color blue { 0, 0, 255U, 255U };
constexpr SDL_Color yellow { 255U, 255U, 0, 255U };
constexpr SDL_Color cyan { 0, 255U, 255U, 255U };
constexpr SDL_Color magenta { 255U, 0, 255U, 255U };

constexpr SDL_Color gray { 128U, 128U, 128U, 255U };
constexpr SDL_Color dark_gray { 64U, 64U, 64U, 255U };
constexpr SDL_Color light_gray { 192U, 192U, 192U, 255U };

constexpr SDL_Color orange { 255U, 165U, 0, 255U };
constexpr SDL_Color pink { 255U, 192U, 203U, 255U };
constexpr SDL_Color purple { 128U, 0, 128U, 255U };
constexpr SDL_Color brown { 165U, 42U, 42U, 255U };
constexpr SDL_Color gold { 255U, 215U, 0, 255U };

constexpr SDL_Color navy { 0, 0, 128U, 255U };
constexpr SDL_Color teal { 0, 128U, 128U, 255U };
constexpr SDL_Color olive { 128U, 128U, 0, 255U };
constexpr SDL_Color maroon { 128U, 0, 0, 255U };
constexpr SDL_Color lime { 50U, 205U, 50U, 255U };

constexpr SDL_Color sky_blue { 135U, 206U, 235U, 255U };
constexpr SDL_Color deep_sky_blue { 0, 191U, 255U, 255U };
constexpr SDL_Color royal_blue { 65U, 105U, 225U, 255U };
constexpr SDL_Color forest_green { 34U, 139U, 34U, 255U };
constexpr SDL_Color sea_green { 46U, 139U, 87U, 255U };

constexpr SDL_Color indigo { 75U, 0, 130U, 255U };
constexpr SDL_Color violet { 238U, 130U, 238U, 255U };
constexpr SDL_Color lavender { 230U, 230U, 250U, 255U };
constexpr SDL_Color beige { 245U, 245U, 220U, 255U };
constexpr SDL_Color ivory { 255U, 255U, 240U, 255U };

constexpr SDL_Color chocolate { 210U, 105U, 30U, 255U };
constexpr SDL_Color coral { 255U, 127U, 80U, 255U };
constexpr SDL_Color salmon { 250U, 128U, 114U, 255U };
constexpr SDL_Color khaki { 240U, 230U, 140U, 255U };

constexpr SDL_Color deep_purple { 75, 0, 130, 255U };
constexpr SDL_Color radiant_orange { 255, 94, 58, 255U };
constexpr SDL_Color cool_teal { 32, 178, 170, 255U };
constexpr SDL_Color ruby_red { 224, 17, 95, 255U };
constexpr SDL_Color deep_gold { 218, 165, 32, 255U };
constexpr SDL_Color silver { 192, 192, 192, 255U };
constexpr SDL_Color navy_blue { 0, 0, 128, 255U };
constexpr SDL_Color white_smoke { 245, 245, 245, 255U };
constexpr SDL_Color midnight_navy { 25, 25, 112, 255U }; // Deep blue, immersive and sleek
constexpr SDL_Color cosmic_purple { 50, 0, 90, 255U };   // Deep purple with a space-like glow
constexpr SDL_Color dark_slate { 47, 79, 79, 255U };     // Cool, dark, and slightly muted teal

// CosmoClicker
constexpr SDL_Color faded_green { 204U, 238U, 204U, 255U }; // #CEC // close to tea green
constexpr SDL_Color gray_tint { 218U, 218U, 218U, 255U };   // #DA
constexpr SDL_Color dark_navy_blue { 0U, 0U, 107U, 255U };

inline SDL_Color AnimateFast(const f32 t) {
    const u8 red = static_cast<u8>((std::sin(t * 0.5F + 0.0F) * 0.5F + 0.5F) * 255U);
    const u8 green = static_cast<u8>((std::sin(t * 0.7F + 2.0F) * 0.5F + 0.5F) * 255U);
    const u8 blue = static_cast<u8>((std::sin(t * 1.1F + 4.0F) * 0.5F + 0.5F) * 255U);
    return SDL_Color { red, green, blue, 255U };
}
inline SDL_Color AnimateDamp(const f32 t) {
    constexpr float base = 127.0F;
    constexpr float amp = 64.0F;
    const u8 red = static_cast<u8>(base + amp * std::sin(t * 0.5F + 0.0F));
    const u8 green = static_cast<u8>(base + amp * std::sin(t * 0.7F + 2.0F));
    const u8 blue = static_cast<u8>(base + amp * std::sin(t * 1.1F + 4.0F));
    return SDL_Color { red, green, blue, 255U };
}
}
