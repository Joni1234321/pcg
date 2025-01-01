#pragma once

#include "u_types.hpp"
#include "u_collections.hpp"

namespace pce {
class Table {
    List<String> headers;
    List<List<u32>> rows;
public:
    Table(String&& name, const u32 row_count) : rows(row_count, List<u32> { } ) { }
    void AddColumn(String&& title, const List<u32>& values) {
        ASSERT_DBG_RETURN(values.Size() < rows.Size(), "Received more values than rows in table", );
        headers.EmplaceBack(std::move(title));
        for (u32 i = 0U; i < values.Size(); i++) { rows[i].EmplaceBack(values[i]); }
        for (u32 i = values.Size(); i < rows.Size(); i++) { rows[i].EmplaceBack(0); }
    }
    [[nodiscard]] u32 ColumnCount () const { return rows[0U].Size(); }
    [[nodiscard]] const List<String>& Headers () const { return headers; }
};
#if false
inline String& WriteToLogger (Logger& logger, const Table& table) {
    for (const String& header : table.Headers) logger.Write(header);
    logger.Write("|{}", rows[0U]);
    const String line('-', table.ColumnCount());
    logger.Write("{}", line);
    logger.Write("{}", line);
    if (rows.Size() > 1U) {
        for (u32 i = 1U; i < rows.Size(); i++) {
            if (coloring == COLOR_ENABLED) { logger.RotateColor(i - 1U); }
            logger.Write("|{}", rows[i]);
        }
        logger.ClearColor();
        logger.Write("{}", line);
    }
    return logger.GetString();
}
inline void Print(Logger& logger,  const LOGGER_COLOR coloring) const {
    (void)WriteToLogger(logger, coloring);
    logger.Print();
}
#endif

class LoggerTable { // NOLINT(*-struct-pack-align)
    List<String> rows;
public:
    enum LOGGER_COLOR : b8 { COLOR_DISABLED, COLOR_ENABLED };
    LoggerTable(const String& name, const u32 row_count) : rows(row_count + 1U, "") {
        List<u32> idx(row_count);
        std::iota(idx.begin(), idx.end(), 0U);
        AddColumn(name, idx);
    }
    template <typename T> void AddColumnFixed(const String& title, const List<T>& values, const u32 width) {
        ASSERT_DBG_RETURN(values.Size() < rows.Size(), "Received more values than rows in table", );
        rows[0U] += std::format("{:>{}} |", title, width);
        for (u32 i = 0U; i < values.Size(); i++) { rows[i + 1U] += std::format("{:>{}} |", FormatValue(values[i]), width); }
        for (u32 i = static_cast<u32>(values.Size()) + 1U; i < rows.Size(); i++) { rows[i] += std::format("{:>{}} |", "XXXX", width); }
    }
    template <typename T> void AddColumn(const String& title, const List<T>& values) { AddColumnFixed(title, values, title.size() + 1U); }
    String& WriteToLogger (Logger& logger, const LOGGER_COLOR coloring) const {
        const String line('-', rows[0U].size() + 1U);
        logger.Write("{}", line);
        logger.Write("|{}", rows[0U]);
        logger.Write("{}", line);
        if (rows.Size() > 1U) {
            for (u32 i = 1U; i < rows.Size(); i++) {
                if (coloring == COLOR_ENABLED) { logger.RotateColor(i - 1U); }
                logger.Write("|{}", rows[i]);
            }
            logger.ClearColor();
            logger.Write("{}", line);
        }
        return logger.GetString();
    }
    void Print(Logger& logger, const LOGGER_COLOR coloring) const {
        (void)WriteToLogger(logger, coloring);
        logger.Print();
    }
};

template <typename T> static void PrintListStats(Logger& logger, const List<T>& list) {
    const u32 len = list.Size();
    if (len == 0U) { return; }
    const T max = *std::ranges::max_element(list, std::less(), { });
    const T min = *std::ranges::min_element(list, std::less(), { });
    const f64 tot = std::accumulate(list.begin(), list.end(), 0.0);
    const f32 avg = tot / static_cast<T>(len);
    LoggerTable table("List", 1U);
    table.AddColumn("Length", List { { len } });
    table.AddColumn("Total", List { { tot } });
    table.AddColumn("Average", List { { avg } });
    table.AddColumn("Max", List<T> { { max } });
    table.AddColumn("Min", List<T> { { min } });
    table.Print(logger, LoggerTable::COLOR_DISABLED);
}
}