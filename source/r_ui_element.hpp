#pragma once

#include "r_engine.hpp"
#include "u_table.hpp"
#include "u_collections.hpp"
#include "u_types.hpp"

#include <SDL3/SDL_render.h>
#include <SDL3_ttf/SDL_ttf.h>

namespace pce::ui {
using FontSize = u8;
enum class Fonts : FontSize { body = 16U, h1 = 34U, h2 = 30U, h3 = 24U, h4 = 20U, h5 = 18U, h6 = 16U, small = 14U, tiny = 12U, title = 52U };
class Font {
    std::unique_ptr<TTF_Font, decltype(&TTF_CloseFont)> font;

public:
    Font(const AbsolutePath& path, const FontSize size) : font(TTF_OpenFont(path.string().c_str(), size), &TTF_CloseFont) { Logger().Log("Loading Font {} {}", size, path.string()); }
    Font(const Font&) = delete;
    Font& operator=(const Font&) = delete;
    Font(Font&&) noexcept = default;
    Font& operator=(Font&&) noexcept = default;

    b8 FailedLoading() const { return font.get() == nullptr; }
    constexpr TTF_Font *ToSDL() const { return font.get(); }
    FontSize GetSize() const { return TTF_GetFontSize(font.get()); }
};
struct FontCollection {
    const AbsolutePath font_path;

    explicit FontCollection(const AbsolutePath& path) : font_path { path } { }

    FlatMap<Fonts, Font> fonts { };
    [[nodiscard]] const Font& GetFont(const Fonts size) {
        if (!fonts.HasKey(size)) {
            fonts.EmplaceBack(size, Font { font_path, static_cast<FontSize>(size) });
            b8 failed = fonts[size].FailedLoading();
            if (failed) { SDL_Log("Font not loaded (%s)", SDL_GetError()); }
        }
        return fonts[size];
    }
};
enum class TextAlign { left, center, right };
struct TextElement {
    struct Handle {
        u32 id;
    };

    TTF_Text* text;
    f32 x;
    f32 y;
};
struct RectangleElement {
    struct Handle {
        u32 id;
    };

    SDL_Color color;
    SDL_FRect rect;

    void (*on_click)();
};
struct ListElement {
    struct Handle {
        u32 id;
    };

    SDL_Color color;
    std::vector<SDL_FRect> rects;
};
struct ColumnInfo {
    f32 x;
    u32 width;
};
struct RowInfo {
    f32 y;
    u32 height;
};
struct TableElement {
    struct Handle {
        u32 id;
    };

    f32 x;
    f32 y;
    SDL_Color text_color;
    List<TextElement::Handle> headers;
    List<TextElement::Handle> text_grid;
    RowInfo header_row_meta;
    List<RowInfo> row_meta;
    List<ColumnInfo> column_meta;
};
}
