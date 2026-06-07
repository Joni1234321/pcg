#pragma once

#include <SDL3_ttf/SDL_ttf.h>

#include "0_engine/u_assets.hpp"
import pce.collections;
#include "0_engine/u_logger.hpp"

namespace pce::ui {
using FontSize = u16;
enum class FontSizes : FontSize { body = 16U, h1 = 34U, h2 = 30U, h3 = 24U, h4 = 20U, h5 = 18U, small = 14U, tiny = 12U, title = 52U, massive = 72U };
constexpr FontSize FONT_MIN_SIZE = 5;
class Font : LogLifetimeWithCount<Font> {
    struct CloseFont {
        void operator()(TTF_Font* font) const {
            Logger().Destroyed("TTF_Font with size {}", TTF_GetFontSize(font));
            TTF_CloseFont(font);
        }
    };
    UniquePointer<TTF_Font, CloseFont> font;

public:
    Font(const AbsolutePath& path, const FontSize size) : font(TTF_OpenFont(path.string().c_str(), size)) { Logger().Created("Font {} {}", size, path.string()); }

    [[nodiscard]] b8 FailedLoading() const { return font.Get() == nullptr; }
    [[nodiscard]] constexpr operator TTF_Font*() const { return font.Get(); }
    [[nodiscard]] FontSize GetSize() const { return static_cast<FontSize>(TTF_GetFontSize(font.Get())); }
};
class FontCollection {
    AbsolutePath font_path_normal { };
    AbsolutePath font_path_bold { };
    mutable FlatMap<FontSizes, Font> fonts_normal { 16U };
    mutable FlatMap<FontSizes, Font> fonts_bold { 16U };

public:
    explicit FontCollection() { }
    void SetFontFile(const AbsolutePath& normal, const AbsolutePath& bold) {
        font_path_normal = normal;
        font_path_bold = bold;
        Clear();
    }
    [[nodiscard]] const Font& GetFontNormal(FontSizes size) const;
    [[nodiscard]] const Font& GetFontBold(FontSizes size) const;
    void Clear() {
        fonts_normal.Clear();
        fonts_bold.Clear();
    }
    ~FontCollection() { Clear(); }
};
} // namespace pce::ui
