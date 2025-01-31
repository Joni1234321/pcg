#pragma once

#include "r_engine.hpp"
#include "u_table.hpp"
#include "u_collections.hpp"
#include "u_types.hpp"

#include <SDL3/SDL_render.h>
#include <SDL3_ttf/SDL_ttf.h>

namespace pce::ui {
struct FontCollection {
    enum FontSizes : u8 { small = 14, normal = 22, large = 36, h1 = 72 };
    FlatMap<u8, TTF_Font*> fonts { };
    const String font_path;

    explicit FontCollection(const AbsolutePath& path) : font_path { path.string() } { }

    [[nodiscard]] constexpr TTF_Font *GetFont(const u8 size) {
        if (!fonts.HasKey(size)) {
            fonts.PushBack(size, TTF_OpenFont(font_path, size));
            if (fonts[size] == nullptr) { SDL_Log("Font not loaded (%s)", SDL_GetError());}
        }
        return fonts[size];
    }

    ~FontCollection() { for (TTF_Font* font : fonts.Values()) { TTF_CloseFont(font); } }
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
