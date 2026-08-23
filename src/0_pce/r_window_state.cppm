module;

#include <SDL3_ttf/SDL_ttf.h>
export module pce.window_state;

import pce.std;

export namespace hex {
constexpr f32 UI_SCALE = 1.0F; // user multiplier on top of the display scale

struct WindowState {
    SDL_Window* window;
    SDL_Renderer* renderer;
    TTF_TextEngine* text_engine;
    TTF_TextEngine* surface_text_engine;
    uint2 screen_size;
    f32 ui_scale { 1.0F }; // UI_SCALE * display scale, applied by the render and input systems
    SDL_Color clear_color;
};
} // namespace hex
