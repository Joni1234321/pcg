module;

#include <SDL3_ttf/SDL_ttf.h>
export module pce.window_state;

import pce.std;
import pce.font;

export namespace hex {
 struct WindowState {
    SDL_Window* window;
    SDL_Renderer* renderer;
    TTF_TextEngine* text_engine;
    TTF_TextEngine* surface_text_engine;
    uint2 screen_size;
    SDL_Color clear_color;
};
} // namespace pce
