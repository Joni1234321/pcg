#pragma once

#include <SDL3_ttf/SDL_ttf.h>

#include "0_engine/u_assets.hpp"
#include "0_engine/u_collections.hpp"
#include "0_engine/u_logger.hpp"

namespace pce::ui {
using FontSize = u8;
enum class FontSizes : FontSize {
    body    = 16U, h1 = 34U, h2 = 30U, h3 = 24U, h4 = 20U, h5 = 18U, h6 = 16U, small = 14U, tiny = 12U, title = 52U,
    massive = 72U
};
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
    [[nodiscard]] constexpr TTF_Font *ToSDL() const { return font.Get(); }
    [[nodiscard]] FontSize GetSize() const { return static_cast<FontSize>(TTF_GetFontSize(font.Get())); }
};
class FontCollection {
    AbsolutePath font_path { };
    FlatMap<FontSizes, Font> fonts { 16U };

public:
    explicit FontCollection() { }
    void SetFontFile(const AbsolutePath& path) {
        font_path = path;
        Clear();
    }
    [[nodiscard]] const Font& GetFont(FontSizes size);
    void Clear() { fonts.Clear(); }
    ~FontCollection() { Clear(); }
};
} //namespace ui
