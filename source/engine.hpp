#pragma once

#include <vector>

#include "collections.hpp"
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
    TTF_Font* h1 = nullptr;
    FontCollection() { }
    b8 Load(const String& path) {
        small = TTF_OpenFont(path.CString(), 14.0F);
        normal = TTF_OpenFont(path.CString(), 22.0F);
        large = TTF_OpenFont(path.CString(), 36.0F);
        h1 = TTF_OpenFont(path.CString(), 72.0F);
        return small && normal && large && h1;
    }
    ~FontCollection() {
        TTF_CloseFont(small);
        TTF_CloseFont(normal);
        TTF_CloseFont(large);
        TTF_CloseFont(h1);
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
    SDL_Color color;
    SDL_FRect rect;
};
inline FontCollection font;

inline std::vector<TextElement> textElements = { };
inline std::vector<Element> elements = { };
inline TextElement::Handle CreateText(const String& string, TTF_Font* font, const SDL_Color color, const f32 x, const f32 y) {
    TTF_Text* text = TTF_CreateText(textRenderer, font, string.CString(), string.size());
    (void)TTF_SetTextColor(text, color.r, color.g, color.b, color.a);
    textElements.emplace_back(text, x, y);
    //TTF_SetTextWrapWidth(text, 680U);
    return TextElement::Handle { static_cast<u32>(textElements.size() - 1) };
}
inline Element::Handle CreateElement(const SDL_FRect rect, const SDL_Color color) {
    elements.emplace_back(color, rect);
    return Element::Handle { static_cast<u32>(elements.size() - 1) };
}
}

namespace pce::ui::colors {
constexpr SDL_Color black { 0, 0, 0, 255U };
constexpr SDL_Color white { 255U, 255U, 255U, 255U };

// Color palette
constexpr SDL_Color light_sky_blue { 135U, 206U, 250U, 255U };
constexpr SDL_Color dark_dark_brown { 62U, 68U, 43U, 255U };

constexpr SDL_Color red { 255U, 0, 0, 255U };
constexpr SDL_Color green { 0, 255U, 0, 255U };
constexpr SDL_Color blue { 0, 0, 255U, 255U };
constexpr SDL_Color yellow { 255U, 255U, 0, 255U };
constexpr SDL_Color cyan { 0, 255U, 255U, 255U };
constexpr SDL_Color magenta { 255U, 0, 255U, 255U };

constexpr SDL_Color gray { 128U, 128U, 128U, 255U };
constexpr SDL_Color dark_gray { 64U, 64U, 64U, 255U };
constexpr SDL_Color light_gray { 192U, 192U, 192U, 255U };

constexpr SDL_Color orange { 255U, 165U, 0, 255U };
constexpr SDL_Color pink { 255U, 192U, 203U, 255U };
constexpr SDL_Color purple { 128U, 0, 128U, 255U };
constexpr SDL_Color brown { 165U, 42U, 42U, 255U };
constexpr SDL_Color gold { 255U, 215U, 0, 255U };

constexpr SDL_Color navy { 0, 0, 128U, 255U };
constexpr SDL_Color teal { 0, 128U, 128U, 255U };
constexpr SDL_Color olive { 128U, 128U, 0, 255U };
constexpr SDL_Color maroon { 128U, 0, 0, 255U };
constexpr SDL_Color lime { 50U, 205U, 50U, 255U };

constexpr SDL_Color sky_blue { 135U, 206U, 235U, 255U };
constexpr SDL_Color deep_sky_blue { 0, 191U, 255U, 255U };
constexpr SDL_Color royal_blue { 65U, 105U, 225U, 255U };
constexpr SDL_Color forest_green { 34U, 139U, 34U, 255U };
constexpr SDL_Color sea_green { 46U, 139U, 87U, 255U };

constexpr SDL_Color indigo { 75U, 0, 130U, 255U };
constexpr SDL_Color violet { 238U, 130U, 238U, 255U };
constexpr SDL_Color lavender { 230U, 230U, 250U, 255U };
constexpr SDL_Color beige { 245U, 245U, 220U, 255U };
constexpr SDL_Color ivory { 255U, 255U, 240U, 255U };

constexpr SDL_Color chocolate { 210U, 105U, 30U, 255U };
constexpr SDL_Color coral { 255U, 127U, 80U, 255U };
constexpr SDL_Color salmon { 250U, 128U, 114U, 255U };
constexpr SDL_Color khaki { 240U, 230U, 140U, 255U };
}

namespace pce {
inline SDL_Window* window = nullptr;
inline SDL_Renderer* renderer = nullptr;
inline SDL_Color clear_color = ui::colors::dark_dark_brown;

inline void DrawImgui(SDL_Renderer* renderer) {
    ImGui_ImplSDLRenderer3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();

    static bool show_demo_window = true;
    ImGui::ShowDemoWindow(&show_demo_window);

    ImGui::Render();
    ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), renderer);
}

inline void Draw() {
    (void)SDL_SetRenderDrawColor(renderer, clear_color.r, clear_color.g, clear_color.b, clear_color.a);
    (void)SDL_RenderClear(renderer);

    for (const ui::TextElement& text : ui::textElements) { TTF_DrawRendererText(text.text, text.x, text.y); }
    for (const ui::Element& element : ui::elements) {
        SDL_SetRenderDrawColor(renderer, element.color.r, element.color.g, element.color.b, element.color.a);
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
    constexpr const char* font_path_absolute = "C:\\Active\\CPP\\pcg\\resources\\font.ttf";
    if (!ui::font.Load(font_path_absolute)) {
        SDL_Log("Font not loaded (%s)", SDL_GetError());
        return false;
    }
    return true;
}

inline b8 InitWindow(const u32 width, const u32 height) {
    constexpr u32 window_flags = SDL_WINDOW_RESIZABLE;
    if (!SDL_CreateWindowAndRenderer("Video Game", static_cast<i32>(width), static_cast<i32>(height), window_flags, &window, &renderer)) {
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
