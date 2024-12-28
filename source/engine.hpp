#pragma once
#include "types.hpp"

#include <SDL3/SDL_init.h>
#include <SDL3/SDL_log.h>
#include <SDL3/SDL_render.h>

namespace pce {
    SDL_Window *window = nullptr;
    SDL_Renderer *renderer = nullptr;

    u32 InitEngine () {
        if (!SDL_Init(SDL_INIT_VIDEO)) {
            SDL_Log("SDL_Init failed (%s)", SDL_GetError());
            return 0U;
        }
        // if (!TTF_Init()) {
        //     SDL_Log("SDL_ttf failed (%s)", SDL_GetError());
        //     return 0U;
        // }
        return 1U;
    }
    u32 SetWindow (const u32 width, const u32 height) {
        if (!SDL_CreateWindowAndRenderer("PCG", static_cast<i32>(width), static_cast<i32>(height), 0, &window, &renderer)) {
            SDL_Log("SDL_CreateWindowAndRenderer failed (%s)", SDL_GetError());
            SDL_Quit();
            return 0U;
        }

        return 1U;
    }
    void DestroyEngine () {
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);

        // TTF_Quit();
        // SDL_Quit();
    }

void TestDraw (u32 i) {
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 100);
        SDL_RenderClear(renderer);

        SDL_SetRenderDrawColor(renderer, i / 3, i++ / 10, i / 67, 100);
        constexpr SDL_FRect rect { .x = 10, .y = 10, .w = 200, .h = 300};
        SDL_RenderFillRect(renderer, &rect);

        const std::string str = std::format("Tick {:6}", i);
        SDL_SetRenderDrawColor(renderer, 0,0,0, 255);
        SDL_RenderDebugText(renderer, 0, 0, str.c_str());


        SDL_RenderPresent(renderer);
    }

}