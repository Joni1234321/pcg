module;

#include "SDL3/SDL_pixels.h"
#include "SDL3/SDL_rect.h"
#include "SDL3_image/SDL_image.h"
#include "SDL3_ttf/SDL_ttf.h"

export module pce.sdl;

import pce.globals;
import pce.window_state;
import pce.assets;
import pce.collections;
import pce.logger;
import pce.std;
import pce.math;

export namespace hex {
struct Texture : LogLifetimeWithCount<Texture> {
    explicit Texture(const AbsolutePath& path) : texture(IMG_LoadTexture(Singleton::Get<WindowState>().renderer, path.string().c_str())) {
        if (texture.Get()) {
            Logger().Created("Texture {} {}", texture->w, texture->h);
        } else {
            Logger().Created("Texture FAILED to load: {}", path.string());
        }
    }
    [[nodiscard]] b8 FailedLoading() const { return texture.Get() == nullptr; }
    operator SDL_Texture*() const { return texture.Get(); }

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
struct AABB {
    float2 point;
    float2 size;

    [[nodiscard]] static constexpr AABB FromPoint(const float2 point, const float2 size) { return AABB { .point = point, .size = size }; }
    [[nodiscard]] static constexpr AABB FromCenter(const float2 center, const float2 size) { return AABB { .point = center - size * float2 { 0.5F }, .size = size }; }
    [[nodiscard]] constexpr operator const SDL_FRect*() const { return reinterpret_cast<const SDL_FRect*>(this); }

    [[nodiscard]] constexpr AABB WithPadding(const float2 padding) const { return FromPoint(point + padding, size - padding * float2 { 2.0F }); }
    [[nodiscard]] constexpr AABB WithOffset(const float2 offset) const { return FromPoint(point + offset, size); }
};
struct OBB {
    float2 center;
    float2 size;
    float angle;

    [[nodiscard]] static constexpr OBB FromCenter(const float2 center, const float2 size, const float angle) { return OBB { .center = center, .size = size, .angle = angle }; }
    [[nodiscard]] static constexpr OBB BetweenPoints(const float2 point_a, const float2 point_b, const f32 thickness) {
        const float2 diff = point_b - point_a;
        return OBB { .center = point_a + diff * float2 { 0.5F }, .size = float2 { math::Hypot(diff), thickness }, .angle = math::Atan2(diff.y, diff.x) };
    }
    // [[nodiscard]] static OBBF BetweenPoints(const float2 a, const float2 b, const f32 thickness) {
    //     const float2 delta = b - a;
    //     const f32 length = math::Hypot(delta);
    //     const f32 angle = math::Atan2(delta.y, delta.x);
    //     const float2 perp_offset = math::Rotate(float2 { 0.0F, -thickness * 0.5F }, angle);
    //     return OBBF { .point = a + perp_offset, .size = { length, thickness }, .angle = angle };
    // }
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
static_assert(sizeof(ColorF) == sizeof(SDL_FColor));

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
    [[nodiscard]] constexpr Color WithAlpha(const f32 a) const { return Color { r, g, b, static_cast<u8>(a * 255.0F) }; }
};
static_assert(sizeof(Color) == sizeof(SDL_Color));

struct Vertex {
    float2 position;
    ColorF color;
    float2 coordinate { };
    constexpr Vertex() = default;
    constexpr Vertex(const float2 position, const ColorF color) : position { position }, color { color } { }
    [[nodiscard]] constexpr operator SDL_Vertex() const { return *reinterpret_cast<const SDL_Vertex*>(this); }
};
static_assert(sizeof(Vertex) == sizeof(SDL_Vertex));
} // namespace hex
