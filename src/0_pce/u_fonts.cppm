module;

#include <SDL3/SDL_error.h>
#include <SDL3/SDL_log.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <cassert>
export module pce.font;

import pce.std;
import pce.math;
import pce.assets;
import pce.collections;
import pce.logger;

export namespace hex::ui {
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
} // namespace hex::ui

namespace hex::ui {
const Font& FontCollection::GetFontNormal(FontSizes size) const {
    assert(static_cast<FontSize>(size) >= FONT_MIN_SIZE);
    size = math::Max(size, static_cast<FontSizes>(FONT_MIN_SIZE));
    if (!fonts_normal.HasKey(size)) {
        fonts_normal.EmplaceBack(size, font_path_normal, static_cast<FontSize>(size));
        if (fonts_normal[size].FailedLoading()) {
            SDL_Log("ERROR Failed Font not loaded (%s)", SDL_GetError());
            fonts_normal.Erase(size);
        }
    }
    return fonts_normal[size];
}
const Font& FontCollection::GetFontBold(FontSizes size) const {
    assert(static_cast<FontSize>(size) >= FONT_MIN_SIZE);
    size = math::Max(size, static_cast<FontSizes>(FONT_MIN_SIZE));
    if (!fonts_bold.HasKey(size)) {
        fonts_bold.EmplaceBack(size, font_path_bold, static_cast<FontSize>(size));
        assert(!fonts_bold[size].FailedLoading()); // Font failed to load — GetFontBold will return invalid font
    }
    return fonts_bold[size];
}
} // namespace hex::ui
