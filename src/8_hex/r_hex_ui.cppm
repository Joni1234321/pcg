module;

#include <SDL3/SDL_render.h>
#include <SDL3_ttf/SDL_ttf.h>

export module hex.ui;

import std;

import pce.std;
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

struct ColorBoxCmd {
    const ui::Font& font;
    const Label& label;
    AABB area { };
    float2 padding { };
    ColorBox colors { };
};

inline void DrawColorBox(const WindowState& window_state, const ColorBoxCmd& cmd) {
    const AABB area_fill = cmd.area.WithPadding(cmd.padding);

    Color color = cmd.colors.color_stroke;
    if (color.a > 0) {
        (void)SDL_SetRenderDrawColor(window_state.renderer, color.r, color.g, color.b, color.a);
        (void)SDL_RenderFillRect(window_state.renderer, cmd.area);
    }

    color = cmd.colors.color_fill;
    if (color.a > 0) {
        (void)SDL_SetRenderDrawColor(window_state.renderer, color.r, color.g, color.b, color.a);
        (void)SDL_RenderFillRect(window_state.renderer, area_fill);
    }

    color = cmd.colors.color_text;
    if (color.a > 0) {
        TTF_SetFontWrapAlignment(cmd.font, TTF_HORIZONTAL_ALIGN_RIGHT);
        (void)TTF_SetTextFont(cmd.label, cmd.font);
        (void)TTF_SetTextColor(cmd.label, color.r, color.g, color.b, color.a);
        (void)TTF_SetTextWrapWidth(cmd.label, area_fill.size.x);
        (void)TTF_DrawRendererText(cmd.label, area_fill.point.x, cmd.area.point.y);
    }
}

}
