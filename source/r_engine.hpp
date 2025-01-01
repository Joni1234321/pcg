#pragma once

#include <vector>
#include <filesystem>
#include <functional>

#include "r_ui.hpp"
#include "u_collections.hpp"
#include "u_types.hpp"

#include "imgui/imgui.h"
#include "imgui/imgui_impl_sdl3.h"
#include "imgui/imgui_impl_sdlrenderer3.h"

#include <SDL3/SDL_init.h>
#include <SDL3/SDL_log.h>
#include <SDL3/SDL_render.h>

#include <SDL3_ttf/SDL_ttf.h>



namespace pce::assets {
inline AbsolutePath Absolute(const RelativePath &relative_path) { return absolute(relative_path); }

inline AbsolutePath Asset(const AssetPath &asset_path) {
    const RelativePath assets_dir = R"(../assets)";
    return absolute(assets_dir / asset_path);
}
}

namespace pce {
inline SDL_Color clear_color = colors::dark_dark_brown;

inline void DrawImgui(SDL_Renderer *renderer) {
    ImGui_ImplSDLRenderer3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();

    static bool show_demo_window = true;
    ImGui::ShowDemoWindow(&show_demo_window);

    ImGui::Render();
    ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), renderer);
}

struct Engine {
    SDL_Window *window = nullptr;
    SDL_Renderer *renderer = nullptr;

    ui::FontCollection font;

    Engine() = default;

    b8 Load() {
        if (!SDL_Init(SDL_INIT_VIDEO)) {
            SDL_Log("SDL_Init failed (%s)", SDL_GetError());
            return false;
        }
        if (!TTF_Init()) {
            SDL_Log("SDL_ttf failed (%s)", SDL_GetError());
            return false;
        }
        const RelativePath font_path = "font.ttf";
        if (!font.Load(assets::Asset(font_path))) {
            SDL_Log("Font not loaded (%s)", SDL_GetError());
            return false;
        }
        return true;
    }

    b8 InitWindow(const u32 width, const u32 height) {
        constexpr u32 window_flags = SDL_WINDOW_RESIZABLE;
        if (!SDL_CreateWindowAndRenderer("Video Game", static_cast<i32>(width), static_cast<i32>(height), window_flags,
                                         &window, &renderer)) {
            SDL_Log("SDL_CreateWindowAndRenderer failed (%s)", SDL_GetError());
            SDL_Quit();
            return false;
        }

        TTF_CreateRendererTextEngine(renderer);

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO *io = &ImGui::GetIO();
        io->ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
        io->ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad; // Enable Gamepad Controls
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

    void ClearScreen() {
        (void) SDL_SetRenderDrawColor(renderer, clear_color.r, clear_color.g, clear_color.b, clear_color.a);
        (void) SDL_RenderClear(renderer);
    }

    void Present() { (void) SDL_RenderPresent(renderer); }

    ~Engine() {
        font.~FontCollection();

        ImGui_ImplSDLRenderer3_Shutdown();
        ImGui_ImplSDL3_Shutdown();
        ImGui::DestroyContext(ImGui::GetCurrentContext());

        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);

        TTF_Quit();
        SDL_Quit();
    }
};
}
