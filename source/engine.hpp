#pragma once

#include <engine.hpp>

#include "types.hpp"

#include "imgui/imgui.h"
#include "imgui/imgui_impl_sdl3.h"
#include "imgui/imgui_impl_sdlrenderer3.h"

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

struct TextElement {
    struct Handle {
        u32 id;
    };
    TTF_Text* text;
    f32 x;
    f32 y;
};
struct Element {
    struct Handle {
        u32 id;
    };
    SDL_FColor color;
    SDL_FRect rect;
};
inline FontCollection font;

inline std::vector<TextElement> textElements = { };
inline std::vector<Element> elements = { };
inline TextElement::Handle CreateText(const String& string, TTF_Font* font, const SDL_FColor color, const f32 x, const f32 y) {
    TTF_Text* text = TTF_CreateText(textRenderer, font, string.CString(), string.size());
    (void)TTF_SetTextColorFloat(text, color.r, color.g, color.b, color.a);
    textElements.emplace_back(text, x, y);
    //TTF_SetTextWrapWidth(text, 680U);
    return TextElement::Handle { static_cast<u32>(textElements.size() - 1) };
}
inline Element::Handle CreateElement(const SDL_FRect rect, const SDL_FColor color) {
    elements.emplace_back(color, rect);
    return Element::Handle { static_cast<u32>(elements.size() - 1) };
}
}

namespace pce {
inline SDL_Window* window = nullptr;
inline SDL_Renderer* renderer = nullptr;

inline void DrawImgui(SDL_Renderer* renderer) {
    ImGui_ImplSDLRenderer3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();

    static bool show_demo_window = true;
    ImGui::ShowDemoWindow(&show_demo_window);

    ImGui::Render();
    ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), renderer);
}

inline void TestDraw() {
    (void)SDL_SetRenderDrawColor(renderer, 255, 255, 255, 100);
    (void)SDL_RenderClear(renderer);

    for (const ui::TextElement& text : ui::textElements) { TTF_DrawRendererText(text.text, text.x, text.y); }
    for (const ui::Element& element : ui::elements) {
        SDL_SetRenderDrawColorFloat(renderer, element.color.r, element.color.g, element.color.b, element.color.a);
        SDL_RenderFillRect(renderer, &element.rect);
    }

    DrawImgui(renderer);

    (void)SDL_RenderPresent(renderer);
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

inline b8 InitWindow(const u32 width, const u32 height) {
    constexpr u32 window_flags = SDL_WINDOW_RESIZABLE;
    if (!SDL_CreateWindowAndRenderer("PCG", static_cast<i32>(width), static_cast<i32>(height), window_flags, &window, &renderer)) {
        SDL_Log("SDL_CreateWindowAndRenderer failed (%s)", SDL_GetError());
        SDL_Quit();
        return false;
    }

    ui::textRenderer = TTF_CreateRendererTextEngine(renderer);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO* io = &ImGui::GetIO();
    io->ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
    io->ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;  // Enable Gamepad Controls
    if (!ImGui_ImplSDL3_InitForSDLRenderer(window, renderer)) {
        SDL_Log("Failed to initialize ImGui SDL3 backend.");
        return false;
    }
    if (!ImGui_ImplSDLRenderer3_Init(renderer)) {
        SDL_Log("Failed to initialize ImGui SDL Renderer3 backend.");
        return false;
    }
    ImGui::StyleColorsLight(&ImGui::GetStyle());
    return true;
}

inline void DestroyEngine() {
    ImGui_ImplSDLRenderer3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext(ImGui::GetCurrentContext());

    TTF_DestroyRendererTextEngine(ui::textRenderer);

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);

    TTF_Quit();
    SDL_Quit();
}
}
