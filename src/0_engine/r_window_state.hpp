#pragma once

#include <SDL3_ttf/SDL_ttf.h>

#include "u_fonts.hpp"
#include "0_engine/u_types.hpp"

namespace pce {
struct WindowState {
    SDL_Window* window;
    SDL_Renderer* renderer;
    TTF_TextEngine* text_engine;
    uint2 screen_size;
    SDL_Color clear_color;
};
} // namespace pce
