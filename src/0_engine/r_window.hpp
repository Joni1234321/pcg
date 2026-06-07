#pragma once

#include <SDL3/SDL_init.h>
#include <SDL3/SDL_log.h>

#include "0_engine/g_globals.hpp"
#include "0_engine/r_window_state.hpp"
#include "0_engine/u_assets.hpp"
#include "0_engine/u_fonts.hpp"
#include "0_engine/u_texture.hpp"

import pce.engine.types;

namespace pce {
static const RelativePath PATH_FONT_NORMAL { "Titillium_Web/TitilliumWeb-Regular.ttf" };
static const RelativePath PATH_FONT_TITILIUM_BOLD { "Titillium_Web/TitilliumWeb-Bold.ttf" };
static const RelativePath PATH_FONT_COURIER_REGULAR { "Courier_Prime/CourierPrime-Regular.ttf" };
static const RelativePath PATH_FONT_COURIER_BOLD { "Courier_Prime/CourierPrime-Bold.ttf" };
struct Window {
    explicit Window(const uint2 size) {
        constexpr u32 window_flags = SDL_WINDOW_RESIZABLE;
        WindowState& window_state = Singleton::Get<WindowState>();
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
        window_state.surface_text_engine = TTF_CreateSurfaceTextEngine();
        window_state.screen_size = size;

        Singleton::Get<ui::FontCollection>().SetFontFile(Asset(PATH_FONT_NORMAL), Asset(PATH_FONT_COURIER_REGULAR));
    }
    ~Window() {
        Singleton::Get<ui::FontCollection>().Clear();
        globalData.Get<Texture>().clear();
        SDL_DestroyRenderer(Singleton::Get<WindowState>().renderer);
        SDL_DestroyWindow(Singleton::Get<WindowState>().window);
        TTF_DestroyRendererTextEngine(Singleton::Get<WindowState>().text_engine);
        TTF_DestroySurfaceTextEngine(Singleton::Get<WindowState>().surface_text_engine);
        TTF_Quit();
        SDL_Quit();
    }
};
} // namespace pce
