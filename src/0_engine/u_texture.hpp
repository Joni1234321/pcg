#pragma once

#include "0_engine/g_globals.hpp"
#include "0_engine/r_window_state.hpp"
#include "0_engine/u_assets.hpp"
#include "0_engine/u_logger.hpp"

#include "SDL3_image/SDL_image.h"

namespace pce {
struct Texture : LogLifetimeWithCount<Texture> {
    explicit Texture(const AbsolutePath &path) : texture(IMG_LoadTexture(singleton.Get<WindowState>().renderer, path.string().c_str())) { if (texture.Get()) Logger().Created("Texture {} {}", texture->w, texture->h); else Logger().Created("Texture FAILED to load: {}", path.string()); }
    [[nodiscard]] b8 FailedLoading() const { return texture.Get() == nullptr; }
    [[nodiscard]] constexpr SDL_Texture *ToSDL() const { return texture.Get(); }
private:
    struct CloseTexture {
        void operator()(SDL_Texture* texture) const {
            if (texture) { Logger().Destroyed("SDL_Texture destroyed {} {}", texture->w, texture->h); SDL_DestroyTexture(texture); }
        }
    };
    UniquePointer<SDL_Texture, CloseTexture> texture;
};
struct DestroyText {
    void operator()(TTF_Text* text) const {
        Logger().Destroyed("TTF_Text");
        TTF_DestroyText(text);
    }
};
} // namespace pce