#pragma once

#include <SDL3/SDL_init.h>
#include <SDL3/SDL_log.h>

#include "0_engine/g_globals.hpp"
#include "0_engine/r_window_state.hpp"
#include "0_engine/u_assets.hpp"
#include "0_engine/u_fonts.hpp"
#include "0_engine/u_texture.hpp"
#include "0_engine/u_types.hpp"

namespace pce {
static const RelativePath FONT_PATH { "font.ttf" };
static const RelativePath FONT_BOLD_PATH { "TitilliumWeb-SemiBold.ttf" };
struct Window {
    explicit Window(const uint2 size) {
        constexpr u32 window_flags = SDL_WINDOW_RESIZABLE;
        WindowState& window_state = singleton.Get<WindowState>();
        if (!SDL_CreateWindowAndRenderer("Video Game", static_cast<i32>(size.x), static_cast<i32>(size.y), window_flags, &window_state.window, &window_state.renderer)) {
            SDL_Log("SDL_CreateWindowAndRenderer failed (%s)", SDL_GetError());
            SDL_Quit();
        }
        if (!SDL_Init(SDL_INIT_VIDEO)) {
            SDL_Log("SDL_Init failed (%s)", SDL_GetError());
            SDL_Quit();
        }
        if (!TTF_Init()) {
            SDL_Log("SDL_ttf failed (%s)", SDL_GetError());
            SDL_Quit();
        }
        SDL_SetRenderDrawBlendMode(window_state.renderer, SDL_BLENDMODE_BLEND);
        window_state.text_engine = TTF_CreateRendererTextEngine(window_state.renderer);
        window_state.screen_size = size;

        singleton.Get<ui::FontCollection>().SetFontFile(Asset(FONT_PATH));
    }
    ~Window() {
        singleton.Get<ui::FontCollection>().Clear();
        data.Get<Texture>().Clear();
        SDL_DestroyRenderer(singleton.Get<WindowState>().renderer);
        SDL_DestroyWindow(singleton.Get<WindowState>().window);
        TTF_DestroyRendererTextEngine(singleton.Get<WindowState>().text_engine);
        TTF_Quit();
        SDL_Quit();
    }
};
} // namespace pce
