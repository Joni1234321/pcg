module;

#include <SDL3/SDL_render.h>
#include <SDL3_ttf/SDL_ttf.h>

export module pce.ui;

import pce.std;
import pce.globals;
import pce.window_state;
import pce.font;
import pce.sdl;

export namespace hex::ui {

struct ColorBox {
    Color color_fill { };
    Color color_stroke { };
    Color color_text { };
    Color color_text_shadow { };
};

struct AABBWithPadding {
    AABB area { };
    float2 padding { };
};

inline void DrawRect(const WindowState& window_state, const AABB area, const Color color) {
    if (color.a == 0) { return; }
    (void)SDL_SetRenderDrawColor(window_state.renderer, color.r, color.g, color.b, color.a);
    (void)SDL_RenderFillRect(window_state.renderer, area);
}

inline void DrawTexture(const WindowState& window_state, const Handle<Texture> texture_handle, const AABB area, const Color color) {
    Texture& texture = globalData[texture_handle];
    texture.SetColor(color);
    (void)SDL_RenderTexture(window_state.renderer, texture, nullptr, area);
}

inline void DrawText(const Font& font, const Label& label, const AABB area, const Color color, const TextAlignment alignment = TextAlignment::RIGHT) {
    if (color.a == 0) { return; }
    font.SetWrapAlignment(alignment);
    label.SetFont(font);
    label.SetColor(color);
    label.SetWrapWidth(area.size.x);
    label.Draw(area.point);
}

inline void DrawColorBox(const WindowState& window_state, const Font& font, const Label& label, const AABBWithPadding& area_with_padding, const ColorBox& color_box, const TextAlignment alignment = TextAlignment::RIGHT) {
    const AABB area_fill = area_with_padding.area.WithPadding( area_with_padding.padding);

    DrawRect(window_state, area_with_padding.area, color_box.color_stroke);
    DrawRect(window_state, area_fill, color_box.color_fill);

    const AABB area_text = AABB::FromPoint(float2 { area_fill.point.x, area_with_padding.area.point.y }, area_fill.size);
    if (color_box.color_text_shadow.a != 0) { DrawText(font, label, area_text.WithOffset(float2 { static_cast<f32>(font.GetSize()) * 0.04F }), color_box.color_text_shadow, alignment); }
    DrawText(font, label, area_text, color_box.color_text, alignment);
}

}
