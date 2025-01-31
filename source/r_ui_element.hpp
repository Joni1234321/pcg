#pragma once

#include "r_engine.hpp"
#include "u_table.hpp"
#include "u_collections.hpp"
#include "u_types.hpp"

#include <SDL3/SDL_render.h>
#include <SDL3_ttf/SDL_ttf.h>

namespace pce::ui {
using FontSize = u8;
class Font {
    TTF_Font *font;
public:
    [[nodiscard]] Font (const AbsolutePath& path, FontSize size) : font(TTF_OpenFont(path.string().c_str(), size)) { }
    Font(const Font&) = delete;

    b8 FailedLoading () const { return font == nullptr; }
    ~Font() { if (font != nullptr) { TTF_CloseFont(font); } }
    constexpr TTF_Font* ToSDL() const { return font; }
};
struct FontCollection {
    enum FontSizes : FontSize { small = 14, normal = 22, large = 36, h1 = 72 };
    FlatMap<FontSize, Font> fonts { };
    const AbsolutePath font_path;

    explicit FontCollection(const AbsolutePath& path) : font_path { path } { }

    [[nodiscard]] constexpr const Font& GetFont(const u8 size) {
        if (!fonts.HasKey(size)) {
            fonts.PushBack(size, Font(font_path, size));
            b8 failed = fonts[size].FailedLoading();
            if (failed) { SDL_Log("Font not loaded (%s)", SDL_GetError());}
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
