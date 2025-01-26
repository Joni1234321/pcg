#include "r_ui.hpp"

namespace pce::ui {
void UISystem::Tick(InputSystem& input_system, NodeTree& tree) {
    Node::Handle previous_hovered_node = hovered_node;

    hovered_node = tree.HitNode(input_system.MousePosition());
    if (previous_hovered_node.id != hovered_node.id) {
        tree.Propagate(previous_hovered_node, &Node::OnHoverOut);
        tree.Propagate(hovered_node, &Node::OnHover);
    }

    if (hovered_node.HasValue() || previous_hovered_node.HasValue()) { tree.MarkDirty(); }
}
void UISystem::RenderTree(SDL_Renderer* renderer, NodeTree& node_tree) {
    const FrameElements& frame_elements = node_tree.GetFrameElements();
    for (const RectangleElement& element : frame_elements.rectangles) {
        (void)SDL_SetRenderDrawColor(renderer, element.color.r, element.color.g, element.color.b, element.color.a);
        (void)SDL_RenderFillRect(renderer, &element.rect);
    }
    for (const TextElement& text : frame_elements.texts) { (void)TTF_DrawRendererText(text.text, text.x, text.y); }
}
void UISystem::LeftClick(NodeTree& tree) { tree.Propagate(hovered_node, &Node::OnClick); }
UISystem::UISystem(Engine& engine): text_renderer(TTF_CreateRendererTextEngine(engine.renderer)) {
    engine.GetWindowSize(&screen_width, &screen_height);
    const RelativePath font_path = "font.ttf";
    if (!font.Load(assets::Asset(font_path))) { SDL_Log("Font not loaded (%s)", SDL_GetError()); }
    const RelativePath font_bold_path = "TitilliumWeb-SemiBold.ttf";
    if (!font_bold.Load(assets::Asset(font_bold_path))) { SDL_Log("Bold font not loaded (%s)", SDL_GetError()); }
}
void UISystem::RenderElements(SDL_Renderer* renderer) {
    for (const RectangleElement& element : rectangle_elements) {
        (void)SDL_SetRenderDrawColor(renderer, element.color.r, element.color.g, element.color.b, element.color.a);
        (void)SDL_RenderFillRect(renderer, &element.rect);
    }
    for (const ListElement& list : list_elements) {
        (void)SDL_SetRenderDrawColor(renderer, list.color.r, list.color.g, list.color.b, list.color.a);
        (void)SDL_RenderFillRects(renderer, list.rects.data(), static_cast<u32>(list.rects.size()));
    }
    for (const TextElement& text : text_elements) { (void)TTF_DrawRendererText(text.text, text.x, text.y); }
}

UISystem::~UISystem() {
    for (const TextElement& text : text_elements) { TTF_DestroyText(text.text); }
    for (const RectangleElement& element : rectangle_elements) { }
    TTF_DestroyRendererTextEngine(text_renderer);
}
TextElement::Handle UISystem::CreateTextAligned(const String& string, TTF_Font* font, const SDL_Color color, f32 x, const f32 y, const TextAlign alignment, const u32 width) {
    TTF_Text* text = TTF_CreateText(text_renderer, font, string.CString(), string.size());
    i32 text_width;
    (void)TTF_SetTextColor(text, color.r, color.g, color.b, color.a);
    (void)TTF_GetTextSize(text, &text_width, nullptr);
    switch (alignment) {
        case TextAlign::left:
            break;
        case TextAlign::center:
            x += (width - static_cast<f32>(text_width)) * 0.5F;
            break;
        case TextAlign::right:
            x += width - static_cast<f32>(text_width);
            break;
    }
    (void)text_elements.emplace_back(text, x, y);
    return TextElement::Handle { static_cast<u32>(text_elements.size() - 1) };
}
TextElement::Handle UISystem::CreateText(const String& string, TTF_Font* font, const SDL_Color color, const f32 x, const f32 y) { return CreateTextAligned(string, font, color, x, y, TextAlign::left, 0U); }
RectangleElement::Handle UISystem::CreateElement(const SDL_FRect rect, const SDL_Color color, void (*on_click)()) {
    (void)rectangle_elements.emplace_back(color, rect, on_click);
    return RectangleElement::Handle { static_cast<u32>(rectangle_elements.size() - 1) };
}
ListElement::Handle UISystem::CreateList(const SDL_Color color, const SDL_FRect rect, const u32 count, const f32 gap, const ListDirection direction) {
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
void UISystem::IncreaseTableSize(TableElement& table_element, const Table& table) {
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
        if (recycle_text) { UpdateText(handle, string, x, y); } else { handle = CreateText(string, font.small, table_element.text_color, x, y); }
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
            if (recycle_text) { UpdateText(text_handle, string, x, y); } else { text_handle = CreateText(string, font.small, table_element.text_color, x, y); }
        }

        const TextElement::Handle first_text_in_row = table_element.text_grid[row * table.ColumnCount()];
        RowInfo& row_info = table_element.row_meta[row];
        GetTextSize(first_text_in_row, nullptr, &row_info.height);
        y += static_cast<f32>(row_info.height);
        row_info.y = y;
    }
}
TableElement::Handle UISystem::CreateTable(const Table& table, const SDL_Color text_color, const f32 x, const f32 y) {
    TableElement table_element { .x = x, .y = y, .text_color = text_color };
    IncreaseTableSize(table_element, table);
    (void)table_elements.EmplaceBack(table_element);
    return TableElement::Handle { table_elements.Size() - 1 };
}
void UISystem::UpdateTable(const TableElement::Handle& handle, const Table& table) {
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
}
