#pragma once

#include <vector>
#include <filesystem>
#include <functional>

#include "r_colors.hpp"
#include "u_table.hpp"
#include "u_collections.hpp"
#include "u_types.hpp"

#include "imgui/imgui.h"
#include "imgui/imgui_impl_sdl3.h"
#include "imgui/imgui_impl_sdlrenderer3.h"

#include <SDL3/SDL_init.h>
#include <SDL3/SDL_log.h>
#include <SDL3/SDL_render.h>

#include <SDL3_ttf/SDL_ttf.h>

namespace pce::ui {
inline b8 bb_collision(const f32 x, f32 y, const SDL_FRect rect) {
    // todo: make hit collision
    return false;
}

struct FontCollection {
    TTF_Font *small = nullptr;
    TTF_Font *normal = nullptr;
    TTF_Font *large = nullptr;
    TTF_Font *h1 = nullptr;

    b8 Load(const AbsolutePath &path) {
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

struct TextElement {
    struct Handle {
        u32 id;
    };

    TTF_Text *text;
    f32 x;
    f32 y;
};

struct Element {
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

class UISystem {
    TTF_TextEngine *text_renderer = nullptr;
    std::vector<TextElement> text_elements;
    std::vector<Element> elements;
    std::vector<ListElement> list_elements;

public:
    UISystem(SDL_Renderer *renderer) { text_renderer = TTF_CreateRendererTextEngine(renderer); }

    void Draw(SDL_Renderer *renderer) {
        for (const TextElement &text: text_elements) { (void) TTF_DrawRendererText(text.text, text.x, text.y); }
        for (const Element &element: elements) {
            (void) SDL_SetRenderDrawColor(renderer, element.color.r, element.color.g, element.color.b, element.color.a);
            (void) SDL_RenderFillRect(renderer, &element.rect);
        }
        for (const ListElement &list: list_elements) {
            (void) SDL_SetRenderDrawColor(renderer, list.color.r, list.color.g, list.color.b, list.color.a);
            (void) SDL_RenderFillRects(renderer, list.rects.data(), static_cast<u32>(list.rects.size()));
        }
    }

    TextElement::Handle CreateText(const String &string, TTF_Font *font, const SDL_Color color, const f32 x,
                                   const f32 y) {
        TTF_Text *text = TTF_CreateText(text_renderer, font, string.CString(), string.size());
        (void) TTF_SetTextColor(text, color.r, color.g, color.b, color.a);
        (void) text_elements.emplace_back(text, x, y);
        //TTF_SetTextWrapWidth(text, 680U);
        return TextElement::Handle{static_cast<u32>(text_elements.size() - 1)};
    }

    Element::Handle CreateElement(const SDL_FRect rect, const SDL_Color color, void (*on_click)()) {
        (void) elements.emplace_back(color, rect, on_click);
        return Element::Handle{static_cast<u32>(elements.size() - 1)};
    }

    enum class ListDirection { horizontal, vertical };
    ListElement::Handle CreateList(const SDL_Color color, const SDL_FRect rect, const u32 count, const f32 gap, const ListDirection direction) {
        std::vector<SDL_FRect> rects;
        rects.reserve(count);
        if (direction == ListDirection::horizontal) {
            for (u32 i = 0; i < count; i++) {
                const f32 x = rect.x + (rect.w + gap) * static_cast<f32>(i);
                rects.emplace_back(x, rect.y, rect.w, rect.h);
            }
        }
        else if (direction == ListDirection::vertical) {
            for (u32 i = 0; i < count; i++) {
                const f32 y = rect.y + (rect.h + gap) * static_cast<f32>(i);
                rects.emplace_back(rect.x, y, rect.w, rect.h);
            }
        }
        (void)list_elements.emplace_back(color, std::move(rects));
        return ListElement::Handle{static_cast<u32>(list_elements.size() - 1)};
    }

    [[nodiscard]] Element operator [](const Element::Handle handle) { return elements[handle.id]; }
    [[nodiscard]] TextElement operator [](const TextElement::Handle handle) { return text_elements[handle.id]; }
    [[nodiscard]] ListElement operator [](const ListElement::Handle handle) { return list_elements[handle.id]; }

    ~UISystem() {
        for (const TextElement &text: text_elements) { TTF_DestroyText(text.text); }
        for (const Element &element: elements) {
        }
        TTF_DestroyRendererTextEngine(text_renderer);
    }
};


inline void DrawTable (UISystem& ui_system, const Table& table, const pce::ui::FontCollection& font_collection) {
    static bool update_layout = true;
    static List<pce::ui::TextElement::Handle> headers;
    static List<pce::ui::TextElement::Handle> text_grid;

    if (update_layout) {
        update_layout = false;

        headers.Reserve(table.ColumnCount());
        text_grid.Reserve(table.RowCount() * table.ColumnCount());

        const SDL_Color text_color = colors::light_sky_blue;
        constexpr f32 x_start = 500.0F;
        constexpr f32 y_start = 200.0F;
        f32 x = x_start;
        f32 y = y_start;

        struct ColumnInfo {
            f32 x;
            u32 width;
        };
        struct RowInfo {
            f32 y;
            u32 height;
        };
        List<ColumnInfo> column_meta {table.ColumnCount()};
        List<RowInfo> row_meta {table.RowCount()};

        // Create header
        for (const String& title : table.headers) {
            ColumnInfo column_info { x, 0U};
            RowInfo header_row_info { y, 0U };

            const pce::ui::TextElement::Handle handle = ui_system.CreateText(title, font_collection.normal, text_color, x, y);
            (void)TTF_GetTextSize(ui_system[handle].text, reinterpret_cast<i32*>(&column_info.width), reinterpret_cast<i32*>(&header_row_info.height));

            (void)column_meta.EmplaceBack(column_info);
            (void)row_meta.EmplaceBack(header_row_info);
            (void)headers.EmplaceBack(handle);

            x += column_info.width;
        }

        // Create row
        for (u32 row = 0; row < table.RowCount(); row++) {
            x = x_start;
            y += row_meta.Back().height;
            RowInfo row_info { y, 0U };

            for (u32 column = 0; column < table.ColumnCount(); column++) {
                const u32& value = table.columns[column][row];
                const String string = std::to_string(value);
                const pce::ui::TextElement::Handle handle = ui_system.CreateText(string, font_collection.small, text_color, x, y);
                (void)text_grid.EmplaceBack(handle);

                x += column_meta[column].width;
            }

            (void)TTF_GetTextSize(ui_system[text_grid.Back()].text, nullptr, reinterpret_cast<i32*>(&row_info.height));
            (void)row_meta.EmplaceBack(row_info);
        }
    }

    for (u32 row = 0; row < table.RowCount(); row++) {
        for (u32 column = 0; column < table.ColumnCount(); column++) {
            TTF_Text* text = ui_system[text_grid[column + row * table.ColumnCount()]].text;
            const u32& data = table.columns[column][row];
            const String string = std::to_string(data);
            TTF_SetTextString(text, string.CString(), string.size());
        }
    }
}

}


