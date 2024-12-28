#pragma once
#include "types.hpp"

#include <SDL3/SDL_init.h>
#include <SDL3/SDL_log.h>
#include <SDL3/SDL_render.h>

#include <SDL3_ttf/SDL_ttf.h>

namespace pce {
TTF_Font *font = nullptr;
TTF_TextEngine *textRenderer = nullptr;
TTF_Text *text = nullptr;
SDL_Window *window = nullptr;
SDL_Renderer *renderer = nullptr;
SDL_Surface *textSurface = nullptr;
SDL_Texture *textTexture = nullptr;

b8 InitEngine() {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("SDL_Init failed (%s)", SDL_GetError());
        return false;
    }
    if (!TTF_Init()) {
        SDL_Log("SDL_ttf failed (%s)", SDL_GetError());
        return false;
    }
    font = TTF_OpenFont("../resources/font.ttf", 22);
    if (!font) {
        SDL_Log("Font not loaded (%s)", SDL_GetError());
        return false;
    }

    return true;
}

b8 SetWindow(const u32 width, const u32 height) {
    if (!SDL_CreateWindowAndRenderer("PCG", static_cast<i32>(width), static_cast<i32>(height), 0, &window, &renderer)) {
        SDL_Log("SDL_CreateWindowAndRenderer failed (%s)", SDL_GetError());
        SDL_Quit();
        return false;
    }

    std::string welcome = "Welcome to the Engine!\n FISH";

    const SDL_Color color{0, 0, 0, 255};
    textSurface = TTF_RenderText_Solid_Wrapped(font, welcome.c_str(), welcome.length(), color, width);
    textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
    if (!textTexture) SDL_Log("SDL_CreateTextureFromSurface() failed (%s)", SDL_GetError());


    textRenderer = TTF_CreateRendererTextEngine(renderer);
    text = TTF_CreateText(textRenderer, font, welcome.c_str(), welcome.size());
    TTF_SetTextColor(text, 0, 0, 255, 255);

    return true;
}

void DestroyEngine() {
    TTF_CloseFont(font);
    TTF_DestroyRendererTextEngine(textRenderer);

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);

    TTF_Quit();
    SDL_Quit();
}

void TestDraw(u32 i) {
    // Background
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 100);
    SDL_RenderClear(renderer);

    // Square
    SDL_SetRenderDrawColor(renderer, i / 3, i++ / 10, i / 67, 100);
    constexpr SDL_FRect rect{.x = 400, .y = 200, .w = 30, .h = 30};
    SDL_RenderFillRect(renderer, &rect);

    // Text
    SDL_FRect dstRect = { 30.0f, 30.0f };
    SDL_GetTextureSize(textTexture, &dstRect.w, &dstRect.h);
    SDL_RenderTexture(renderer, textTexture, nullptr, &dstRect);

    const std::string str = std::format("Tick {:6}", i);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderDebugText(renderer, 0, 0, str.c_str());


    TTF_DrawRendererText(text, 100.0f, 300.0f);

    // Present
    SDL_RenderPresent(renderer);
}
}
