#pragma once

#include <vector>
#include <filesystem>

#include "u_table.hpp"
#include "u_collections.hpp"
#include "u_types.hpp"
#include "r_engine.hpp"

#include <SDL3/SDL_render.h>

#include <SDL3_ttf/SDL_ttf.h>

namespace pce::ui {
inline b8 bb_collision(const f32 x, f32 y, const SDL_FRect rect) {
    // todo: make hit collision
    return false;
}

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
    const FontCollection& font_collection;
    List<TextElement::Handle> headers;
    List<TextElement::Handle> text_grid;
    RowInfo header_row_meta;
    List<RowInfo> row_meta;
    List<ColumnInfo> column_meta;
};

class UISystem {
    TTF_TextEngine* text_renderer;

    std::vector<TextElement> text_elements { };
    std::vector<Element> elements { };
    std::vector<ListElement> list_elements { };
    List<TableElement> table_elements { };

public:
    FontCollection font;
    explicit UISystem(SDL_Renderer* renderer) : text_renderer(TTF_CreateRendererTextEngine(renderer)) {
        const RelativePath font_path = "font.ttf";
        if (!font.Load(assets::Asset(font_path))) { SDL_Log("Font not loaded (%s)", SDL_GetError()); }
    }

    void Draw(SDL_Renderer* renderer) {
        for (const TextElement& text : text_elements) { (void)TTF_DrawRendererText(text.text, text.x, text.y); }
        for (const Element& element : elements) {
            (void)SDL_SetRenderDrawColor(renderer, element.color.r, element.color.g, element.color.b, element.color.a);
            (void)SDL_RenderFillRect(renderer, &element.rect);
        }
        for (const ListElement& list : list_elements) {
            (void)SDL_SetRenderDrawColor(renderer, list.color.r, list.color.g, list.color.b, list.color.a);
            (void)SDL_RenderFillRects(renderer, list.rects.data(), static_cast<u32>(list.rects.size()));
        }
    }

    TextElement::Handle CreateText(const String& string, TTF_Font* font, const SDL_Color color, f32 x, const f32 y, const TextAlign alignment, const f32 parent_width) {
        TTF_Text* text = TTF_CreateText(text_renderer, font, string.CString(), string.size());
        i32 text_width;
        (void)TTF_SetTextColor(text, color.r, color.g, color.b, color.a);
        (void)TTF_GetTextSize(text, &text_width, nullptr);
        switch (alignment) {
            case TextAlign::left: break;
            case TextAlign::center: x += (parent_width - static_cast<f32>(text_width)) * 0.5F;
                break;
            case TextAlign::right: x += parent_width - static_cast<f32>(text_width);
                break;
        }
        (void)text_elements.emplace_back(text, x, y);
        return TextElement::Handle { static_cast<u32>(text_elements.size() - 1) };
    }

    Element::Handle CreateElement(const SDL_FRect rect, const SDL_Color color, void (*on_click)()) {
        (void)elements.emplace_back(color, rect, on_click);
        return Element::Handle { static_cast<u32>(elements.size() - 1) };
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
        } else if (direction == ListDirection::vertical) {
            for (u32 i = 0; i < count; i++) {
                const f32 y = rect.y + (rect.h + gap) * static_cast<f32>(i);
                rects.emplace_back(rect.x, y, rect.w, rect.h);
            }
        }
        (void)list_elements.emplace_back(color, std::move(rects));
        return ListElement::Handle { static_cast<u32>(list_elements.size() - 1) };
    }

    void GetTextSize(const TextElement::Handle handle, u32* width, u32* height) const { (void)TTF_GetTextSize(text_elements[handle.id].text, reinterpret_cast<i32*>(width), reinterpret_cast<i32*>(height)); }

    void UpdateText(const TextElement::Handle handle, const String& string) { (void)TTF_SetTextString(text_elements[handle.id].text, string.CString(), string.size()); }
    void UpdateText(const TextElement::Handle handle, const String& string, const f32 x, const f32 y) {
        TextElement& text_element = text_elements[handle.id];
        (void)TTF_SetTextString(text_element.text, string.CString(), string.size());
        text_element.x = x;
        text_element.y = y;
    }

    [[nodiscard]] Element operator [](const Element::Handle handle) { return elements[handle.id]; }
    [[nodiscard]] TextElement operator [](const TextElement::Handle handle) { return text_elements[handle.id]; }
    [[nodiscard]] ListElement operator [](const ListElement::Handle handle) { return list_elements[handle.id]; }
    [[nodiscard]] TableElement operator [](const TableElement::Handle handle) { return table_elements[handle.id]; }

    void IncreaseTableSize(TableElement& table_element, const Table& table) {
        ASSERT_DBG_RETURN(table.Size() > table_element.text_grid.Size(), "New table layout is not bigger than previous",)
        const u32 old_columns = table_element.column_meta.Size();
        const u32 old_size = table_element.text_grid.Size();

        f32 x = table_element.x;
        f32 y = table_element.y;
        table_element.header_row_meta.y = y;
        table_element.headers.Resize(table.ColumnCount());
        table_element.text_grid.Resize(table.Size());
        table_element.row_meta.Resize(table.RowCount());
        table_element.column_meta.Resize(table.ColumnCount());

        // Create header
        for (u32 column = 0; column < table.headers.Size(); column++) {
            const b8 recycle_text = column < old_columns;

            const String& string = table.headers[column];
            ColumnInfo& column_info = table_element.column_meta[column];
            TextElement::Handle& handle = table_element.headers[column];
            if (recycle_text) { UpdateText(handle, string, x, y); } else {
                handle = CreateText(string, table_element.font_collection.small, table_element.text_color, x, y, TextAlign::left, table_element.column_meta[column].width);
            }
            GetTextSize(handle, &column_info.width, &table_element.header_row_meta.height);
            column_info.x = x;
            x += static_cast<f32>(column_info.width);
        }
        y += static_cast<f32>(table_element.header_row_meta.height);

        // Create row
        for (u32 row = 0; row < table.RowCount(); row++) {
            for (u32 column = 0; column < table.ColumnCount(); column++) {
                x = table_element.column_meta[column].x;

                const u32 i = row * table.ColumnCount() + column;
                const b8 recycle_text = i < old_size;

                const String& string = table.columns[column][row];
                TextElement::Handle& text_handle = table_element.text_grid[i];
                if (recycle_text) { UpdateText(text_handle, string, x, y); } else {
                    text_handle = CreateText(string, table_element.font_collection.small, table_element.text_color, x, y, TextAlign::left, table_element.column_meta[column].width);
                }
            }

            const TextElement::Handle first_text_in_row = table_element.text_grid[row * table.ColumnCount()];
            RowInfo& row_info = table_element.row_meta[row];
            GetTextSize(first_text_in_row, nullptr, &row_info.height);
            y += static_cast<f32>(row_info.height);
            row_info.y = y;
        }
    }

    TableElement::Handle CreateTable(const Table& table, const FontCollection& font_collection, SDL_Color text_color, const f32 x, const f32 y) {
        TableElement table_element { .x = x, .y = y, .text_color = text_color, .font_collection = font_collection };
        IncreaseTableSize(table_element, table);
        (void)table_elements.EmplaceBack(table_element);
        return TableElement::Handle { table_elements.Size() - 1 };
    }

    void UpdateTable(const TableElement::Handle& handle, const Table& table) {
        TableElement& table_element = table_elements[handle.id];
        if (table.Size() > table_element.text_grid.Size()) { IncreaseTableSize(table_element, table); }
        for (u32 row = 0; row < table.RowCount(); row++) {
            for (u32 column = 0; column < table.ColumnCount(); column++) {
                const u32 i = row * table.ColumnCount() + column;
                const TextElement::Handle text_handle = table_element.text_grid[i];
                const String& string = table.columns[column][row];
                UpdateText(text_handle, string);
            }
        }
    }

    ~UISystem() {
        font.~FontCollection();

        for (const TextElement& text : text_elements) { TTF_DestroyText(text.text); }
        for (const Element& element : elements) { }
        TTF_DestroyRendererTextEngine(text_renderer);
    }
};
}
