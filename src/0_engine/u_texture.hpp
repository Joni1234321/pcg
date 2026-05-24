#pragma once

#include "0_engine/g_globals.hpp"
#include "0_engine/r_window_state.hpp"
#include "0_engine/u_assets.hpp"
#include "0_engine/u_logger.hpp"
#include "0_engine/u_collections.hpp"

#include "SDL3_ttf/SDL_ttf.h"
#include "SDL3/SDL_pixels.h"
#include "SDL3/SDL_rect.h"
#include "SDL3_image/SDL_image.h"

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
    void SetText (const String& string) const { TTF_SetTextString(ttf_text.Get(), string.c_str(), string.size());  }
};
struct AABBF {
    float2 point;
    float2 size;

    [[nodiscard]] static constexpr AABBF FromPoint(const float2 point, const float2 size) { return AABBF { .point = point, .size = size }; }
    [[nodiscard]] static constexpr AABBF FromCenter(const float2 center, const float2 size) { return AABBF { .point = center - size * float2 { 0.5F }, .size = size }; }
    [[nodiscard]] constexpr operator const SDL_FRect*() const { return reinterpret_cast<const SDL_FRect*>(this); }
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
    [[nodiscard]] constexpr operator SDL_Color() const { return *reinterpret_cast<const SDL_Color*>(this); }
    constexpr static f32 TO_FCOLOR = 1.0F / 255.0F;
    [[nodiscard]] constexpr operator ColorF() const { return ColorF { r * TO_FCOLOR, g * TO_FCOLOR, b * TO_FCOLOR, a * TO_FCOLOR }; }
};
} // namespace pce
