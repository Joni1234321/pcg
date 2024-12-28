#pragma once

#include <array>
#include <engine.hpp>

#include "types.hpp"

#include <SDL3/SDL_init.h>
#include <SDL3/SDL_log.h>
#include <SDL3/SDL_render.h>

#include <SDL3_ttf/SDL_ttf.h>

namespace pce::ui {
struct FontCollection {
    TTF_Font* small = nullptr;
    TTF_Font* normal = nullptr;
    TTF_Font* large = nullptr;
    FontCollection() { }
    bool Load(const std::string& path) {
        small = TTF_OpenFont(path.c_str(), 14.0F);
        normal = TTF_OpenFont(path.c_str(), 22.0F);
        large = TTF_OpenFont(path.c_str(), 36.0F);
        return small && normal && large;
    }
    constexpr ~FontCollection() {
        TTF_CloseFont(small);
        TTF_CloseFont(normal);
        TTF_CloseFont(large);
    }
};
inline FontCollection font;
}

namespace pce {
inline TTF_TextEngine* textRenderer = nullptr;
inline TTF_Text* text = nullptr;
inline SDL_Window* window = nullptr;
inline SDL_Renderer* renderer = nullptr;
inline SDL_Surface* textSurface = nullptr;
inline SDL_Texture* textTexture = nullptr;

inline void TestDraw(u32 i) {
    // Background
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 100);
    SDL_RenderClear(renderer);

    // Square
    SDL_SetRenderDrawColor(renderer, i / 3, i++ / 10, i / 67, 100);
    constexpr SDL_FRect rect { .x = 400, .y = 200, .w = 30, .h = 30 };
    SDL_RenderFillRect(renderer, &rect);

    // Text
    SDL_FRect dstRect = { 30.0F, 30.0F };
    SDL_GetTextureSize(textTexture, &dstRect.w, &dstRect.h);
    SDL_RenderTexture(renderer, textTexture, nullptr, &dstRect);

    const std::string str = std::format("Tick {:6}", i);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    //SDL_RenderDebugText(renderer, 0, 0, str.c_str());

    TTF_SetTextString(text, str.c_str(), str.length());
    u8 j = i / 10 % 3;
    TTF_SetTextFont(text, j % 3 == 0 ? ui::font.small : j % 3 == 1 ? ui::font.normal : ui::font.large);
    TTF_DrawRendererText(text, 10.0F, 0.0F);

    // Present
    SDL_RenderPresent(renderer);
}

inline b8 InitEngine() {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("SDL_Init failed (%s)", SDL_GetError());
        return false;
    }
    if (!TTF_Init()) {
        SDL_Log("SDL_ttf failed (%s)", SDL_GetError());
        return false;
    }
    constexpr const char* font_path = "../resources/font.ttf";
    if (!ui::font.Load(font_path)) {
        SDL_Log("Font not loaded (%s)", SDL_GetError());
        return false;
    }
    return true;
}

inline b8 SetWindow(const u32 width, const u32 height) {
    if (!SDL_CreateWindowAndRenderer("PCG", static_cast<i32>(width), static_cast<i32>(height), 0, &window, &renderer)) {
        SDL_Log("SDL_CreateWindowAndRenderer failed (%s)", SDL_GetError());
        SDL_Quit();
        return false;
    }

    std::string welcome = "Welcome to the Engine!\n FISH";

    constexpr SDL_Color color { 0, 0, 0, 255 };
    textSurface = TTF_RenderText_Solid_Wrapped(ui::font.normal, welcome.c_str(), welcome.length(), color, width);
    textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
    if (!textTexture) { SDL_Log("SDL_CreateTextureFromSurface() failed (%s)", SDL_GetError()); }

    textRenderer = TTF_CreateRendererTextEngine(renderer);
    text = TTF_CreateText(textRenderer, ui::font.small, welcome.c_str(), welcome.size());
    TTF_SetTextColor(text, 0, 0, 255, 255);

    return true;
}

inline void DestroyEngine() {
    TTF_DestroyRendererTextEngine(textRenderer);

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);

    TTF_Quit();
    SDL_Quit();
}
}
