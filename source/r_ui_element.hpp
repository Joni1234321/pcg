#pragma once

#include "r_engine.hpp"
#include "u_table.hpp"
#include "u_collections.hpp"
#include "u_types.hpp"

#include <SDL3/SDL_render.h>
#include <SDL3_ttf/SDL_ttf.h>

namespace pce::ui {
struct FontCollection {
    TTF_Font* small = nullptr;
    TTF_Font* normal = nullptr;
    TTF_Font* large = nullptr;
    TTF_Font* h1 = nullptr;

    b8 Load(const AbsolutePath& path) {
        const String font = path.string();
        small = TTF_OpenFont(font, 14.0F);
        normal = TTF_OpenFont(font, 22.0F);
        large = TTF_OpenFont(font, 36.0F);
        h1 = TTF_OpenFont(font, 72.0F);

        return small && normal && large && h1;
    }

    ~FontCollection() {
        if (small != nullptr) {
            TTF_CloseFont(small);
            small = nullptr;
        }
        if (normal != nullptr) {
            TTF_CloseFont(normal);
            normal = nullptr;
        }
        if (large != nullptr) {
            TTF_CloseFont(large);
            large = nullptr;
        }
        if (h1 != nullptr) {
            TTF_CloseFont(h1);
            h1 = nullptr;
        }
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