#pragma once

#include <filesystem>
#include <u_ecs.hpp>

#include "r_colors.hpp"
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
inline AbsolutePath Absolute(const RelativePath& relative_path) { return absolute(relative_path); }

inline AbsolutePath Asset(const AssetPath& asset_path) {
    const RelativePath assets_dir = R"(../assets)";
    return absolute(assets_dir / asset_path);
}
}

namespace pce {
class InputSystem {
    b8 left_mouse { false };
    b8 left_mouse_down { false };
    uint2 mouse_position { };

public:
    void Tick() {
        float2 mouse_position_f;
        SDL_MouseButtonFlags state = SDL_GetMouseState(&mouse_position_f.x, &mouse_position_f.y);
        left_mouse_down = state && SDL_BUTTON_LMASK && !left_mouse;
        left_mouse = state && SDL_BUTTON_LMASK;
        mouse_position.x = static_cast<u32>(mouse_position_f.x);
        mouse_position.y = static_cast<u32>(mouse_position_f.y);
    }
    [[nodiscard]] constexpr uint2 MousePosition() const noexcept { return mouse_position; }
    [[nodiscard]] constexpr b8 LeftMouseDown() const noexcept { return left_mouse_down; }
    [[nodiscard]] constexpr b8 LeftMouse() const noexcept { return left_mouse; }
};
}

namespace pce {
using Tick = NamedType<u32, struct TickTag, Arithmetic>;
inline SDL_Color clear_color = colors::dark_dark_brown;

inline void DrawImgui(SDL_Renderer* renderer) {
    ImGui_ImplSDLRenderer3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();

    static bool show_demo_window = true;
    ImGui::ShowDemoWindow(&show_demo_window);

    ImGui::Render();
    ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), renderer);
}

struct Engine {
    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    Tick tick { 0U };
    std::chrono::time_point<std::chrono::high_resolution_clock> last_tick_start;
    f32 tick_time { 1.0f };
    f32 delta_time { 1.0f };

    Engine() = default;

    void MarkTickStart() {
        using namespace std::chrono;
        tick += Tick { 1U };
        delta_time = duration<f32>(high_resolution_clock::now() - last_tick_start).count();
        last_tick_start = high_resolution_clock::now();
    }
    void MarkTickEnd() {
        using namespace std::chrono;
        tick_time = duration<f32>(high_resolution_clock::now() - last_tick_start).count();
    }

    static b8 Load() {
        if (!SDL_Init(SDL_INIT_VIDEO)) {
            SDL_Log("SDL_Init failed (%s)", SDL_GetError());
            return false;
        }
        if (!TTF_Init()) {
            SDL_Log("SDL_ttf failed (%s)", SDL_GetError());
            return false;
        }
        return true;
    }

    b8 InitWindow(const u32 width, const u32 height) {
        constexpr u32 window_flags = SDL_WINDOW_RESIZABLE;
        if (!SDL_CreateWindowAndRenderer("Video Game", static_cast<i32>(width), static_cast<i32>(height), window_flags, &window, &renderer)) {
            SDL_Log("SDL_CreateWindowAndRenderer failed (%s)", SDL_GetError());
            SDL_Quit();
            return false;
        }

        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        TTF_CreateRendererTextEngine(renderer);

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
    void ClearScreen() {
        (void)SDL_SetRenderDrawColor(renderer, clear_color.r, clear_color.g, clear_color.b, clear_color.a);
        (void)SDL_RenderClear(renderer);
    }
    void Present() const { (void)SDL_RenderPresent(renderer); }
    void GetWindowSize(u32* width, u32* height) const { (void)SDL_GetWindowSize(window, reinterpret_cast<i32*>(width), reinterpret_cast<i32*>(height)); }
    ~Engine() {
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
