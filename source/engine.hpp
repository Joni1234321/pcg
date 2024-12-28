#pragma once

#include <engine.hpp>

#include "types.hpp"

#include <SDL3/SDL_init.h>
#include <SDL3/SDL_log.h>
#include <SDL3/SDL_render.h>

#include <SDL3_ttf/SDL_ttf.h>

namespace pce::ui {
inline TTF_TextEngine* textRenderer = nullptr;

struct FontCollection {
    TTF_Font* small = nullptr;
    TTF_Font* normal = nullptr;
    TTF_Font* large = nullptr;
    FontCollection() { }
    b8 Load(const std::string& path) {
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

struct TextElementFixed {
    struct Handle {
        u32 id;
    };
    TTF_Text* text;
    f32 x;
    f32 y;
};

inline FontCollection font;
inline std::vector<TextElementFixed> textElements = { };
inline TextElementFixed::Handle CreateFixedText(const String& string, TTF_Font* font, const SDL_FColor color, const f32 x, const f32 y) {
    TTF_Text* text = TTF_CreateText(textRenderer, font, string.CString(), string.size());
    (void)TTF_SetTextColorFloat(text, color.r, color.g, color.b, color.a);
    textElements.emplace_back(text, x, y);
    //TTF_SetFontWrapAlignment()
    TTF_SetTextWrapWidth(text, 680);
    return TextElementFixed::Handle { static_cast<u32>(textElements.size() - 1) };
}
}

namespace pce {
inline ui::TextElementFixed::Handle tick_handle;
inline SDL_Window* window = nullptr;
inline SDL_Renderer* renderer = nullptr;


inline void TestDraw(u32 i) {
    // Background
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 100);
    SDL_RenderClear(renderer);

    // Square
    SDL_SetRenderDrawColor(renderer, i / 3, i++ / 10, i / 67, 100);
    constexpr SDL_FRect rect { .x = 400, .y = 200, .w = 30, .h = 30 };
    SDL_RenderFillRect(renderer, &rect);

    const std::string str = std::format("Tick {:6}", i);
    TTF_SetTextString(ui::textElements[tick_handle.id].text, str.c_str(), str.length());
    for (const ui::TextElementFixed& text : ui::textElements) { TTF_DrawRendererText(text.text, text.x, text.y); }

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
    if (!SDL_CreateWindowAndRenderer("PCG", static_cast<i32>(width), static_cast<i32>(height), SDL_WINDOW_RESIZABLE, &window, &renderer)) {
        SDL_Log("SDL_CreateWindowAndRenderer failed (%s)", SDL_GetError());
        SDL_Quit();
        return false;
    }
    ui::textRenderer = TTF_CreateRendererTextEngine(renderer);

    String welcome = "Welcome to the Engine!\n FISH";
    constexpr SDL_FColor textColor { 0.0F, 0.0F, 1.0F, 1.0F };
    tick_handle = ui::CreateFixedText(welcome, ui::font.small, textColor, 10.0F, 0.0F);
    ui::CreateFixedText(welcome, ui::font.normal, textColor, 10.0F, 10.0F);
    return true;
}

inline void DestroyEngine() {
    TTF_DestroyRendererTextEngine(ui::textRenderer);

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);

    TTF_Quit();
    SDL_Quit();
}
}
