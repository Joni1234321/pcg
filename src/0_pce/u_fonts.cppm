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
constexpr FontSize FONT_MIN_SIZE = 8;
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
    AbsolutePath font_path_normal_courier { };
    AbsolutePath font_path_bold_courier { };
    AbsolutePath font_path_bold_compact { };
    mutable FlatMap<FontSizes, Font> fonts_normal_courier { 256U };
    mutable FlatMap<FontSizes, Font> fonts_bold_courier { 256U };
    mutable FlatMap<FontSizes, Font> fonts_bold_compact { 256U };

    [[nodiscard]] static const Font& GetFont(FlatMap<FontSizes, Font>& fonts, const AbsolutePath& font_path, FontSizes font_size) {
        assert(static_cast<FontSize>(font_size) >= FONT_MIN_SIZE);
        font_size = math::Max(font_size, static_cast<FontSizes>(FONT_MIN_SIZE));
        if (!fonts.HasKey(font_size)) {
            fonts.EmplaceBack(font_size, font_path, static_cast<FontSize>(font_size));
            if (fonts[font_size].FailedLoading()) { SDL_Log("ERROR Failed Font not loaded, size %u (%s)", static_cast<u32>(font_size), SDL_GetError()); }
        }
        return fonts[font_size];
    }

public:
    explicit FontCollection() { }
    void SetFontFile(const AbsolutePath& normal_courier, const AbsolutePath& bold_courier, const AbsolutePath& bold_compact) {
        font_path_normal_courier = normal_courier;
        font_path_bold_courier = bold_courier;
        font_path_bold_compact = bold_compact;
        Clear();
    }
    [[nodiscard]] const Font& GetFontNormalCourier(const FontSizes size) const { return GetFont(fonts_normal_courier, font_path_normal_courier, size); }
    [[nodiscard]] const Font& GetFontBoldCourier(const FontSizes size) const { return GetFont(fonts_bold_courier, font_path_bold_courier, size); }
    [[nodiscard]] const Font& GetFontBoldCompact(const FontSizes size) const { return GetFont(fonts_bold_compact, font_path_bold_compact, size); }
    void Clear() {
        fonts_normal_courier.Clear();
        fonts_bold_courier.Clear();
        fonts_bold_compact.Clear();
    }
    ~FontCollection() { Clear(); }
};
} // namespace hex::ui

namespace hex::ui {

} // namespace hex::ui
