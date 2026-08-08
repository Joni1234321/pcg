module;

#include <SDL3/SDL_render.h>
#include <SDL3_ttf/SDL_ttf.h>

export module pcs.ui;

import std;

import pce.std;
import pce.globals;
import pce.window_state;
import pce.font;
import pce.sdl;
import pce.colors;

export namespace hex {

struct ColorBox {
    Color color_fill { };
    Color color_stroke { };
    Color color_text { };
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

inline b8 DrawTexture(const WindowState& window_state, const HandleOptional<Texture> texture_handle, const AABB area, const Color color) {
    if (!texture_handle.IsValid()) { return false; }
    SDL_Texture* texture = globalData[texture_handle.GetHandle()];
    (void)SDL_SetTextureColorMod(texture, color.r, color.g, color.b);
    (void)SDL_SetTextureAlphaMod(texture, color.a);
    (void)SDL_RenderTexture(window_state.renderer, texture, nullptr, area);
    return true;
}

inline void DrawText(const ui::Font& font, const Label& label, const AABB area, const Color color, const TTF_HorizontalAlignment alignment = TTF_HORIZONTAL_ALIGN_RIGHT) {
    if (color.a == 0) { return; }
    TTF_SetFontWrapAlignment(font, alignment);
    (void)TTF_SetTextFont(label, font);
    (void)TTF_SetTextColor(label, color.r, color.g, color.b, color.a);
    (void)TTF_SetTextWrapWidth(label, area.size.x);
    (void)TTF_DrawRendererText(label, area.point.x, area.point.y);
}

inline void DrawColorBox(const WindowState& window_state, const ui::Font& font, const Label& label, const AABBWithPadding& area_with_padding, const ColorBox& color_box, const TTF_HorizontalAlignment alignment = TTF_HORIZONTAL_ALIGN_RIGHT) {
    const AABB area_fill = area_with_padding.area.WithPadding(area_with_padding.padding);

    DrawRect(window_state, area_with_padding.area, color_box.color_stroke);
    DrawRect(window_state, area_fill, color_box.color_fill);

    const AABB area_text = AABB::FromPoint(float2 { area_fill.point.x, area_with_padding.area.point.y }, area_fill.size);
    DrawText(font, label, area_text, color_box.color_text, alignment);
}

}
