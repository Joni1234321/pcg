module;

#include "SDL3/SDL_pixels.h"
#include "SDL3/SDL_rect.h"
#include "SDL3_image/SDL_image.h"
#include "SDL3_ttf/SDL_ttf.h"

export module pce.sdl;

import std;
import pce.globals;
import pce.window_state;
import pce.assets;
import pce.collections;
import pce.logger;
import pce.std;
import pce.math;

export namespace hex {
struct DestroyText {
    void operator()(TTF_Text* text) const {
        Logger().Destroyed("TTF_Text");
        TTF_DestroyText(text);
    }
};
struct AABB {
    float2 point;
    float2 size;

    [[nodiscard]] static constexpr AABB FromPoint(const float2 point, const float2 size) { return AABB { .point = point, .size = size }; }
    [[nodiscard]] static constexpr AABB FromCenter(const float2 center, const float2 size) { return AABB { .point = center - size * float2 { 0.5F }, .size = size }; }
    [[nodiscard]] constexpr operator const SDL_FRect*() const { return reinterpret_cast<const SDL_FRect*>(this); }

    [[nodiscard]] constexpr AABB WithPadding(const float2 padding) const { return FromPoint(point + padding, size - padding * float2 { 2.0F }); }
    [[nodiscard]] constexpr b8 Contains(const float2 position) const { return position.x >= point.x && position.x <= point.x + size.x && position.y >= point.y && position.y <= point.y + size.y; }
    [[nodiscard]] constexpr AABB WithOffset(const float2 offset) const { return FromPoint(point + offset, size); }
};
namespace polygon {
// convex hull, monotone chain
[[nodiscard]] inline List<float2> ConvexHull(List<float2> points) {
    std::ranges::sort(points, [](const float2 a, const float2 b) { return a.x < b.x || (a.x == b.x && a.y < b.y); });
    List<float2> hull { };
    for (u32 pass = 0; pass < 2; pass++) {
        const usize pass_start = hull.size();
        for (const float2 point : points) {
            while (hull.size() >= pass_start + 2 && math::Cross(hull[hull.size() - 1] - hull[hull.size() - 2], point - hull[hull.size() - 2]) <= 0.0F) { hull.pop_back(); }
            hull.push_back(point);
        }
        hull.pop_back();
        std::ranges::reverse(points);
    }
    return hull;
}

[[nodiscard]] inline b8 Contains(const List<float2>& polygon, const float2 point) {
    b8 all_left = true;
    b8 all_right = true;
    for (u32 v = 0; v < polygon.size(); v++) {
        const f32 side = math::Cross(polygon[(v + 1U) % polygon.size()] - polygon[v], point - polygon[v]);
        all_left = all_left && side >= 0.0F;
        all_right = all_right && side <= 0.0F;
    }
    return all_left || all_right;
}

// separating axis test, both polygons must be convex
[[nodiscard]] inline b8 Overlaps(const List<float2>& polygon_a, const List<float2>& polygon_b) {
    const auto separated_on_any_axis_of = [](const List<float2>& axis_polygon, const List<float2>& a, const List<float2>& b) -> b8 {
        for (u32 v = 0; v < axis_polygon.size(); v++) {
            const float2 edge = axis_polygon[(v + 1U) % axis_polygon.size()] - axis_polygon[v];
            const float2 axis { -edge.y, edge.x };
            f32 min_a = std::numeric_limits<f32>::max();
            f32 max_a = std::numeric_limits<f32>::lowest();
            for (const float2 point : a) {
                min_a = math::Min(min_a, math::Dot(axis, point));
                max_a = math::Max(max_a, math::Dot(axis, point));
            }
            f32 min_b = std::numeric_limits<f32>::max();
            f32 max_b = std::numeric_limits<f32>::lowest();
            for (const float2 point : b) {
                min_b = math::Min(min_b, math::Dot(axis, point));
                max_b = math::Max(max_b, math::Dot(axis, point));
            }
            if (max_a < min_b || max_b < min_a) { return true; }
        }
        return false;
    };
    if (polygon_a.size() == 0 || polygon_b.size() == 0) { return false; }
    return !separated_on_any_axis_of(polygon_a, polygon_a, polygon_b) && !separated_on_any_axis_of(polygon_b, polygon_a, polygon_b);
}

[[nodiscard]] inline float2 Centroid(const List<float2>& polygon) {
    float2 sum { };
    for (const float2 point : polygon) { sum += point; }
    return sum * float2 { 1.0F / static_cast<f32>(polygon.size()) };
}
} // namespace polygon

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
    [[nodiscard]] constexpr Color WithSaturation(const f32 factor) const {
        const f32 gray = 0.299F * static_cast<f32>(r) + 0.587F * static_cast<f32>(g) + 0.114F * static_cast<f32>(b);
        return Color { static_cast<u8>(gray + (static_cast<f32>(r) - gray) * factor), static_cast<u8>(gray + (static_cast<f32>(g) - gray) * factor), static_cast<u8>(gray + (static_cast<f32>(b) - gray) * factor), a };
    }
    [[nodiscard]] constexpr operator SDL_Color() const { return *reinterpret_cast<const SDL_Color*>(this); }
    constexpr static f32 TO_FCOLOR = 1.0F / 255.0F;
    [[nodiscard]] constexpr operator ColorF() const { return ColorF { r * TO_FCOLOR, g * TO_FCOLOR, b * TO_FCOLOR, a * TO_FCOLOR }; }
    [[nodiscard]] constexpr Color WithAlpha(const f32 a) const { return Color { r, g, b, static_cast<u8>(a * 255.0F) }; }
};
static_assert(sizeof(Color) == sizeof(SDL_Color));

struct Texture : LogLifetimeWithCount<Texture> {
    explicit Texture(const AbsolutePath& path) : texture(IMG_LoadTexture(Singleton::Get<WindowState>().renderer, path.string().c_str())) {
        if (texture.Get()) {
            Logger().Created("Texture {} {}", texture->w, texture->h);
        } else {
            Logger().Error("Texture FAILED to load: {}", path.string());
        }
    }
    [[nodiscard]] b8 FailedLoading() const { return texture.Get() == nullptr; }
    operator SDL_Texture*() const { return texture.Get(); }
    void SetColor(const Color color) const {
        (void)SDL_SetTextureColorMod(texture.Get(), color.r, color.g, color.b);
        (void)SDL_SetTextureAlphaMod(texture.Get(), color.a);
    }

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

struct Label {
    UniquePointer<TTF_Text, DestroyText> ttf_text { nullptr };
    [[nodiscard]] Label(TTF_Font* font = nullptr, const String& text = { }) : ttf_text { TTF_CreateText(Singleton::Get<WindowState>().text_engine, font, text.c_str(), text.size()) } { }
    operator TTF_Text*() const { return ttf_text.Get(); }
    void SetFont(TTF_Font* font) const { (void)TTF_SetTextFont(ttf_text.Get(), font); }
    void SetText(const String& string) const { (void)TTF_SetTextString(ttf_text.Get(), string.c_str(), string.size()); }
    void SetColor(const Color color) const { (void)TTF_SetTextColor(ttf_text.Get(), color.r, color.g, color.b, color.a); }
    void SetWrapWidth(const f32 width) const { (void)TTF_SetTextWrapWidth(ttf_text.Get(), static_cast<i32>(width)); }
    void Draw(const float2 position) const { (void)TTF_DrawRendererText(ttf_text.Get(), position.x, position.y); }
};
struct SurfaceLabel {
    UniquePointer<TTF_Text, DestroyText> ttf_text { TTF_CreateText(Singleton::Get<WindowState>().surface_text_engine, nullptr, "", 0) };
    operator TTF_Text*() const { return ttf_text.Get(); }
    void SetText(const String& string) const { TTF_SetTextString(ttf_text.Get(), string.c_str(), string.size()); }
};

struct Vertex {
    float2 position;
    ColorF color;
    float2 coordinate { };
    constexpr Vertex() = default;
    constexpr Vertex(const float2 position, const ColorF color) : position { position }, color { color } { }
    [[nodiscard]] constexpr operator SDL_Vertex() const { return *reinterpret_cast<const SDL_Vertex*>(this); }
};
static_assert(sizeof(Vertex) == sizeof(SDL_Vertex));

inline b8 SDL_RenderGeometry(SDL_Renderer* renderer, SDL_Texture* texture, const Span<const Vertex> vertices, const Span<const i32> indices) { return SDL_RenderGeometry(renderer, texture, reinterpret_cast<const SDL_Vertex*>(vertices.data()), vertices.size(), indices.data(), indices.size()); }
inline b8 SDL_RenderGeometry(SDL_Renderer* renderer, SDL_Texture* texture, const Span<const Vertex> vertices) { return SDL_RenderGeometry(renderer, texture, reinterpret_cast<const SDL_Vertex*>(vertices.data()), vertices.size(), nullptr, 0); }
} // namespace hex
