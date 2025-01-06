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
    List<TextElement::Handle> headers;
    List<TextElement::Handle> text_grid;
    RowInfo header_row_meta;
    List<RowInfo> row_meta;
    List<ColumnInfo> column_meta;
};


class Color {
public:
    u8 r;
    u8 g;
    u8 b;
    u8 a;
    Color(u8 r, u8 g, u8 b) : r(r), g(g), b(b), a(255) { }
    Color(u8 r, u8 g, u8 b, u8 a) : r(r), g(g), b(b), a(a) { }
    //Color(f32 r, f32 g, f32 b) : r(r * 255U), g(g * 255U), b(b * 255U), a(255) { }
    //Color(f32 r, f32 g, f32 b, f32 a) : r(r * 255U), g(g * 255U), b(b * 255U), a(a * 255U) { }
    explicit Color (const SDL_Color color) : Color(color.r, color.g, color.b, color.a) { }
    explicit Color (const SDL_FColor color) : Color(color.r, color.g, color.b, color.a) { }
};
struct ResolvedLayout {
    SDL_FRect layout;
};
struct LayoutLength {
    enum LayoutType { fixed, hug, fill };
    u32 resolved;
    LayoutType layout_type;
};
class Node {
    String name;
    Color background_color;
    float2 position;
    float2 padding;
    uint2 size;

    LayoutLength width;
    LayoutLength height;
    Node& parent;
    List<Node> children;

public:
    Node();
    Node(const Node&) = delete;
    Node(Node&&) = default;
    void AddChild (Node&& node);

    const String& GetName() const;

    const LayoutLength& GetWidth() const { return width; }
    void RecalculateResolvedWidth();
    void SetWidth(LayoutLength new_width);


    List<Node>& GetChildren();
    Color GetResolvedBackgroundColor();
    float2 GetResolvedPosition();
    SDL_FRect GetLayout();
};
struct TextNode : Node {
    void SetText(String& string);
    String& GetText();
private:
    Color color;
    String text;
};
struct NodeBuilder {
    NodeBuilder();
    NodeBuilder& WithColor(Color color);
    NodeBuilder& WithPosition(float2 position);
    Node Build ();
private:
    Node node;
};
class NodeRenderer {
    void RenderNode(Node& node);
};

class UISystem {
    SDL_Renderer* renderer;
    TTF_TextEngine* text_renderer;

    std::vector<TextElement> text_elements { };
    std::vector<Element> elements { };
    std::vector<ListElement> list_elements { };
    List<TableElement> table_elements { };

public:
    enum class ListDirection { horizontal, vertical };
    u32 screen_width;
    u32 screen_height;
    FontCollection font;
    FontCollection font_bold;
    explicit UISystem(Engine& engine);
    ~UISystem();

    void SetColor(Color color);

    void DrawElements(SDL_Renderer* renderer);
    void UpdateNodeLayout(Node& node);

    TextElement::Handle CreateTextAligned(const String& string, TTF_Font* font, const SDL_Color color, f32 x, const f32 y, const TextAlign alignment, const u32 width);
    TextElement::Handle CreateText(const String& string, TTF_Font* font, const SDL_Color color, f32 x, const f32 y);
    Element::Handle CreateElement(const SDL_FRect rect, const SDL_Color color, void (*on_click)());
    ListElement::Handle CreateList(const SDL_Color color, const SDL_FRect rect, const u32 count, const f32 gap, const ListDirection direction);
    void GetTextSize(const TextElement::Handle handle, u32* width, u32* height) const { (void)TTF_GetTextSize(text_elements[handle.id].text, reinterpret_cast<i32*>(width), reinterpret_cast<i32*>(height)); }
    void UpdateText(const TextElement::Handle handle, const String& string) { (void)TTF_SetTextString(text_elements[handle.id].text, string.CString(), string.size()); }
    void UpdateText(const TextElement::Handle handle, const String& string, const f32 x, const f32 y) {
        TextElement& text_element = text_elements[handle.id];
        (void)TTF_SetTextString(text_element.text, string.CString(), string.size());
        text_element.x = x;
        text_element.y = y;
    }

    [[nodiscard]] Element& operator [](const Element::Handle handle) { return elements[handle.id]; }
    [[nodiscard]] TextElement& operator [](const TextElement::Handle handle) { return text_elements[handle.id]; }
    [[nodiscard]] ListElement& operator [](const ListElement::Handle handle) { return list_elements[handle.id]; }
    [[nodiscard]] TableElement& operator [](const TableElement::Handle handle) { return table_elements[handle.id]; }

    void IncreaseTableSize(TableElement& table_element, const Table& table);
    TableElement::Handle CreateTable(const Table& table, SDL_Color text_color, const f32 x, const f32 y);
    void UpdateTable(const TableElement::Handle& handle, const Table& table);
};
}
