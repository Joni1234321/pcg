#pragma once

#include "0_engine/g_globals.hpp"
#include "0_engine/r_window_state.hpp"
#include "0_engine/u_assets.hpp"
#include "0_engine/u_collections.hpp"
#include "0_engine/u_logger.hpp"

#include "SDL3/SDL_pixels.h"
#include "SDL3/SDL_rect.h"
#include "SDL3_image/SDL_image.h"
#include "SDL3_ttf/SDL_ttf.h"

namespace pce {
struct Texture : LogLifetimeWithCount<Texture> {
    explicit Texture(const AbsolutePath& path) : texture(IMG_LoadTexture(Singleton::Get<WindowState>().renderer, path.string().c_str())) {
        if (texture.Get()) {
            Logger().Created("Texture {} {}", texture->w, texture->h);
        } else {
            Logger().Created("Texture FAILED to load: {}", path.string());
        }
    }
    [[nodiscard]] b8 FailedLoading() const { return texture.Get() == nullptr; }
    [[nodiscard]] constexpr SDL_Texture* ToSDL() const { return texture.Get(); }

private:
    struct CloseTexture {
        void operator()(SDL_Texture* texture) const {
            if (texture) {
                Logger().Destroyed("SDL_Texture destroyed {} {}", texture->w, texture->h);
                SDL_DestroyTexture(texture);
            }
        }
    };
    UniquePointer<SDL_Texture, CloseTexture> texture;
};
struct DestroyText {
    void operator()(TTF_Text* text) const {
        Logger().Destroyed("TTF_Text");
        TTF_DestroyText(text);
    }
};
struct Label {
    UniquePointer<TTF_Text, DestroyText> ttf_text { TTF_CreateText(Singleton::Get<WindowState>().text_engine, nullptr, "", 0) };
    operator TTF_Text*() const { return ttf_text.Get(); }
    void SetText(const String& string) const { TTF_SetTextString(ttf_text.Get(), string.c_str(), string.size()); }
};
struct SurfaceLabel {
    UniquePointer<TTF_Text, DestroyText> ttf_text { TTF_CreateText(Singleton::Get<WindowState>().surface_text_engine, nullptr, "", 0) };
    operator TTF_Text*() const { return ttf_text.Get(); }
    void SetText(const String& string) const { TTF_SetTextString(ttf_text.Get(), string.c_str(), string.size()); }
};
struct AABBF {
    float2 point;
    float2 size;

    [[nodiscard]] static constexpr AABBF FromPoint(const float2 point, const float2 size) { return AABBF { .point = point, .size = size }; }
    [[nodiscard]] static constexpr AABBF FromCenter(const float2 center, const float2 size) { return AABBF { .point = center - size * float2 { 0.5F }, .size = size }; }
    [[nodiscard]] constexpr operator const SDL_FRect*() const { return reinterpret_cast<const SDL_FRect*>(this); }

    [[nodiscard]] constexpr AABBF WithPadding(const float2 padding) const { return FromPoint(point + padding, size - padding * float2 { 2.0F }); }
    [[nodiscard]] constexpr AABBF WithOffset(const float2 offset) const { return FromPoint(point + offset, size); }
};
struct ColorF {
    f32 r;
    f32 g;
    f32 b;
    f32 a;

    constexpr ColorF() = default;
    constexpr ColorF(f32 r, f32 g, f32 b, f32 a = 1.0F) : r(r), g(g), b(b), a(a) { }
    [[nodiscard]] constexpr operator SDL_FColor() const { return *reinterpret_cast<const SDL_FColor*>(this); }
};

struct Color {
    u8 r;
    u8 g;
    u8 b;
    u8 a;

    constexpr Color() = default;
    constexpr Color(u8 r, u8 g, u8 b, u8 a = 255U) : r(r), g(g), b(b), a(a) { }
    [[nodiscard]] static constexpr Color FromHsl(f32 hue, f32 saturation, f32 luminance, f32 a = 1.0F) {
        const f32 c = (1.0F - (luminance >= 0.5F ? 2.0F * luminance - 1.0F : 1.0F - 2.0F * luminance)) * saturation;
        const f32 hp = hue / 60.0F;
        const f32 hp2 = hp - 2.0F * static_cast<f32>(static_cast<i32>(hp * 0.5F));
        const f32 xa = hp2 - 1.0F;
        const f32 x = c * (1.0F - (xa < 0.0F ? -xa : xa));
        f32 r = 0.0F;
        f32 g = 0.0F;
        f32 b = 0.0F;
        if (hp < 1.0F) {
            r = c;
            g = x;
        } else if (hp < 2.0F) {
            r = x;
            g = c;
        } else if (hp < 3.0F) {
            g = c;
            b = x;
        } else if (hp < 4.0F) {
            g = x;
            b = c;
        } else if (hp < 5.0F) {
            r = x;
            b = c;
        } else {
            r = c;
            b = x;
        }
        const f32 m = luminance - c * 0.5F;
        return Color {
            static_cast<u8>((r + m) * 255.0F),
            static_cast<u8>((g + m) * 255.0F),
            static_cast<u8>((b + m) * 255.0F),
            static_cast<u8>(a * 255.0F),
        };
    }
    [[nodiscard]] constexpr Color Mul(const f32 factor) const { return Color { static_cast<u8>(r * factor), static_cast<u8>(g * factor), static_cast<u8>(b * factor), a }; }
    [[nodiscard]] constexpr operator SDL_Color() const { return *reinterpret_cast<const SDL_Color*>(this); }
    constexpr static f32 TO_FCOLOR = 1.0F / 255.0F;
    [[nodiscard]] constexpr operator ColorF() const { return ColorF { r * TO_FCOLOR, g * TO_FCOLOR, b * TO_FCOLOR, a * TO_FCOLOR }; }
    [[nodiscard]] constexpr Color WithAlpha(const u8 a) const { return Color { r, g, b, a }; }
    [[nodiscard]] constexpr Color WithAlpha(const f32 a) const { return Color { r, g, b, static_cast<u8>(a * 255.0F) }; }
};
} // namespace pce
