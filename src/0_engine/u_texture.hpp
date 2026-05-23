#pragma once

#include "0_engine/g_globals.hpp"
#include "0_engine/r_window_state.hpp"
#include "0_engine/u_assets.hpp"
#include "0_engine/u_logger.hpp"

#include "SDL3/SDL_rect.h"
#include "SDL3_image/SDL_image.h"

namespace pce {
struct Texture : LogLifetimeWithCount<Texture> {
    explicit Texture(const AbsolutePath& path) : texture(IMG_LoadTexture(Singleton::Get<WindowState>().renderer, path.string().c_str())) {
        if (texture.Get()) {
            Logger().Created("Texture {} {}", texture->w, texture->h);
        } else {
            Logger().Created("Texture FAILED to load: {}", path.string());
        }
    }
    [[nodiscard]] b8 FailedLoading() const { return texture.Get() == nullptr; }
    [[nodiscard]] constexpr SDL_Texture* ToSDL() const { return texture.Get(); }

private:
    struct CloseTexture {
        void operator()(SDL_Texture* texture) const {
            if (texture) {
                Logger().Destroyed("SDL_Texture destroyed {} {}", texture->w, texture->h);
                SDL_DestroyTexture(texture);
            }
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
struct Label {
    UniquePointer<TTF_Text, DestroyText> ttf_text { TTF_CreateText(Singleton::Get<WindowState>().text_engine, nullptr, "", 0) };
    operator TTF_Text*() const { return ttf_text.Get(); }
};
struct AABBF {
    float2 point;
    float2 size;

    [[nodiscard]] static constexpr AABBF FromPoint(const float2 point, const float2 size) { return AABBF { .point = point, .size = size }; }
    [[nodiscard]] static constexpr AABBF FromCenter(const float2 center, const float2 size) { return AABBF { .point = center - size * float2 { 0.5F } , .size = size }; }
    [[nodiscard]] constexpr operator const SDL_FRect* () const { return reinterpret_cast<const SDL_FRect*>(this); }
};
} // namespace pce
