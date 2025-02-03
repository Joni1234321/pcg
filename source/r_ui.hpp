#pragma once

#include <vector>

#include "r_engine.hpp"
#include "r_ui_node.hpp"
#include "r_ui_element.hpp"
#include "u_table.hpp"
#include "u_collections.hpp"
#include "u_types.hpp"

#include <SDL3/SDL_render.h>
#include <SDL3_ttf/SDL_ttf.h>

namespace pce::ui {
class NodeRenderSystem {
    std::vector<TextElement> text_elements { };
    std::vector<RectangleElement> rectangle_elements { };
    std::vector<ListElement> list_elements { };
    List<TableElement> table_elements { };

public:
    using HoveredType = std::optional<NodeReference>;
    HoveredType hovered { };
    enum class ListDirection { horizontal, vertical };
    TTF_TextEngine* text_engine;
    FontCollection font;
    FontCollection font_bold;
    List<std::reference_wrapper<NodeTree>> node_trees { };
    auto GetNodeTrees() const { return node_trees | std::views::transform(&std::reference_wrapper<NodeTree>::get); }
    explicit NodeRenderSystem(RenderSystem& engine);
    ~NodeRenderSystem();

    HoveredType GetHovered(uint2 mouse_position) const;
    void HoverClickEvents(const InputSystem& input_system);
    void RenderTrees(SDL_Renderer* renderer);
    void LeftClickHoveredItem();
    void SetColor(Color color);

    void RenderElements(SDL_Renderer* renderer);

    TextElement::Handle CreateTextAligned(const String& string, TTF_Font* font, const SDL_Color color, f32 x, const f32 y, const TextAlign alignment, const u32 width);
    TextElement::Handle CreateText(const String& string, TTF_Font* font, const SDL_Color color, f32 x, const f32 y);
    RectangleElement::Handle CreateElement(const SDL_FRect rect, const SDL_Color color, void (*on_click)());
    ListElement::Handle CreateList(const SDL_Color color, const SDL_FRect rect, const u32 count, const f32 gap, const ListDirection direction);
    void GetTextSize(const TextElement::Handle handle, u32* width, u32* height) const { (void)TTF_GetTextSize(text_elements[handle.id].text, reinterpret_cast<i32*>(width), reinterpret_cast<i32*>(height)); }
    void UpdateText(const TextElement::Handle handle, const String& string) { (void)TTF_SetTextString(text_elements[handle.id].text, string.CString(), string.size()); }
    void UpdateText(const TextElement::Handle handle, const String& string, const f32 x, const f32 y) {
        TextElement& text_element = text_elements[handle.id];
        (void)TTF_SetTextString(text_element.text, string.CString(), string.size());
        text_element.x = x;
        text_element.y = y;
    }

    [[nodiscard]] RectangleElement& operator [](const RectangleElement::Handle handle) { return rectangle_elements[handle.id]; }
    [[nodiscard]] TextElement& operator [](const TextElement::Handle handle) { return text_elements[handle.id]; }
    [[nodiscard]] ListElement& operator [](const ListElement::Handle handle) { return list_elements[handle.id]; }
    [[nodiscard]] TableElement& operator [](const TableElement::Handle handle) { return table_elements[handle.id]; }

    void IncreaseTableSize(TableElement& table_element, const Table& table);
    TableElement::Handle CreateTable(const Table& table, SDL_Color text_color, const f32 x, const f32 y);
    void UpdateTable(const TableElement::Handle& handle, const Table& table);
};
}
