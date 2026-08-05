module;

#include <algorithm>
#include <any>
#include <cassert>
#include <format>
#include <functional>
#include <initializer_list>
#include <numeric>
#include <variant>
export module pce.table;

import pce.collections;
import pce.logger;
import pce.std;

export namespace hex {
struct Table {
    using Cell = String;
    using Column = List<Cell>;
    String name;
    List<String> headers { };
    List<Column> columns { };
    explicit Table(String&& name) : name(std::move(name)) { }
    void AddColumn(String&& title, const List<String>& values) {
        assert(ColumnCount() == 0 || values.size() == RowCount()); //, "Received different amount of values", );
        headers.EmplaceBack(std::move(title));
        columns.EmplaceBack(values);
    }
    template <typename T> void AddColumn(String&& title, const List<T>& values) {
        headers.EmplaceBack(std::move(title));
        List<String> strings { values.size() };
        for (const auto& value : values) { strings.EmplaceBack(FormatValue(value)); }
        columns.EmplaceBack(strings);
    }

    [[nodiscard]] constexpr u32 RowCount() const { return columns[0].size(); }
    [[nodiscard]] constexpr u32 ColumnCount() const { return columns.size(); }
    [[nodiscard]] constexpr u32 Size() const { return RowCount() * ColumnCount(); }
};

struct TableU32 {
    using Cell = u32;
    using Column = List<Cell>;
    String name;
    List<String> headers { };
    List<Column> columns { };
    explicit TableU32(String&& name) : name(std::move(name)) { }
    void AddColumn(String&& title, const List<u32>& values) {
        assert(ColumnCount() == 0 || values.size() == RowCount()); // , "Received different amount of values", );
        headers.EmplaceBack(std::move(title));
        columns.EmplaceBack(values);
    }
    [[nodiscard]] constexpr u32 RowCount() const { return columns[0].size(); }
    [[nodiscard]] constexpr u32 ColumnCount() const { return columns.size(); }
};

inline String TableToString(const TableU32& table) {
    String header_string = "|";
    for (const String& header : table.headers) { header_string += std::format(" {} |", header); }
    String line('-', header_string.size());
    String result = std::format("{}\n{}\n{}\n", line, header_string, line);
    for (u32 row = 0U; row < table.RowCount(); row++) {
        result += "|";
        for (u32 column = 0U; column < table.ColumnCount(); column++) {
            std::variant<unsigned, float> val = table.columns[column][row];
            String text = std::visit([](auto& x) { return std::to_string(x); }, val);
            result += std::format(" {:>{}} |", text, table.headers[column].size());
        }
        result += "\n";
    }
    return result;
}

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
        assert(values.size() < rows.size()); // , "Received more values than rows in table", );
        rows[0U] += std::format("{:>{}} |", title, width);
        for (u32 i = 0U; i < values.size(); i++) { rows[i + 1U] += std::format("{:>{}} |", FormatValue(values[i]), width); }
        for (u32 i = static_cast<u32>(values.size()) + 1U; i < rows.size(); ++i) { rows[i] += std::format("{:>{}} |", "XXXX", width); }
    }
    template <typename T> void AddColumn(const String& title, const List<T>& values) { AddColumnFixed(title, values, title.size() + 1U); }
    String& WriteToLogger(Logger& logger, const LOGGER_COLOR coloring) const {
        const String line('-', rows[0U].size() + 1U);
        logger.Write("{}", line);
        logger.Write("|{}", rows[0U]);
        logger.Write("{}", line);
        if (rows.size() > 1U) {
            for (u32 i = 1U; i < rows.size(); i++) {
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

template <typename T> void PrintListStats(Logger& logger, const List<T>& list) {
    const u32 len = list.size();
    if (len == 0U) { return; }
    const T max = *std::ranges::max_element(list, std::less(), { });
    const T min = *std::ranges::min_element(list, std::less(), { });
    const f64 tot = std::accumulate(list.begin(), list.end(), 0.0);
    const f32 avg = tot / static_cast<T>(len);
    LoggerTable table("List", 1U);
    table.AddColumn("Length", List { std::initializer_list { len } });
    table.AddColumn("Total", List { std::initializer_list { tot } });
    table.AddColumn("Average", List { std::initializer_list { avg } });
    table.AddColumn("Max", List<T> { { max } });
    table.AddColumn("Min", List<T> { { min } });
    table.Print(logger, LoggerTable::COLOR_DISABLED);
}
} // namespace hex
