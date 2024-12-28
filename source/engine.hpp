#pragma once

#include <engine.hpp>

#include "types.hpp"

#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_sdlrenderer3.h"

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

void DrawImgui ();

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
    constexpr u32 window_flags = SDL_WINDOW_RESIZABLE;
    if (!SDL_CreateWindowAndRenderer("PCG", static_cast<i32>(width), static_cast<i32>(height), window_flags, &window, &renderer)) {
        SDL_Log("SDL_CreateWindowAndRenderer failed (%s)", SDL_GetError());
        SDL_Quit();
        return false;
    }
    ui::textRenderer = TTF_CreateRendererTextEngine(renderer);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsLight();

    // Setup Platform/Renderer backends
    ImGui_ImplSDL3_InitForSDLRenderer(window, renderer);
    ImGui_ImplSDLRenderer3_Init(renderer);


    String welcome = "Welcome to the Engine!\n FISH";
    constexpr SDL_FColor textColor { 0.0F, 0.0F, 1.0F, 1.0F };
    tick_handle = ui::CreateFixedText(welcome, ui::font.small, textColor, 10.0F, 0.0F);
    ui::CreateFixedText(welcome, ui::font.normal, textColor, 10.0F, 10.0F);

    return true;
}

inline void DrawImgui() {
            // Start the Dear ImGui frame
        ImGui_ImplSDLRenderer3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();

        // 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
        if (show_demo_window)
            ImGui::ShowDemoWindow(&show_demo_window);

        // 2. Show a simple window that we create ourselves. We use a Begin/End pair to create a named window.
        {
            static float f = 0.0f;
            static int counter = 0;

            ImGui::Begin("Hello, world!");                          // Create a window called "Hello, world!" and append into it.

            ImGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)
            ImGui::Checkbox("Demo Window", &show_demo_window);      // Edit bools storing our window open/close state
            ImGui::Checkbox("Another Window", &show_another_window);

            ImGui::SliderFloat("float", &f, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f
            ImGui::ColorEdit3("clear color", (float*)&clear_color); // Edit 3 floats representing a color

            if (ImGui::Button("Button"))                            // Buttons return true when clicked (most widgets return true when edited/activated)
                counter++;
            ImGui::SameLine();
            ImGui::Text("counter = %d", counter);

            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
            ImGui::End();
        }

        // 3. Show another simple window.
        if (show_another_window)
        {
            ImGui::Begin("Another Window", &show_another_window);   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
            ImGui::Text("Hello from another window!");
            if (ImGui::Button("Close Me"))
                show_another_window = false;
            ImGui::End();
        }

        // Rendering
        ImGui::Render();

}

inline void DestroyEngine() {
    ImGui_ImplSDLRenderer3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();

    TTF_DestroyRendererTextEngine(ui::textRenderer);

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);

    TTF_Quit();
    SDL_Quit();
}
}
