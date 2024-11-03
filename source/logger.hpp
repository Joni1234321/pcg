#pragma once

#include <algorithm>
#include <cstdio>
#include <format>
#include <numeric>
#include <string>
#include <vector>

#include "collections.hpp"
#include "types.hpp"

namespace pce {
#define DISABLE_PREFIX 1 // NOLINT(*-macro-usage)
#if DISABLE_PREFIX
constexpr auto LOGGER_PREFIX_NONE = "        | ";
constexpr auto LOGGER_PREFIX_TIMER = "TIMER   | ";
constexpr auto LOGGER_PREFIX_LOG = "LOG     | ";
constexpr auto LOGGER_PREFIX_WARNING = "WARNING | ";
constexpr auto LOGGER_PREFIX_ERROR = "ERROR   | ";
#else
constexpr auto LOGGER_PREFIX_NONE = "";
constexpr auto LOGGER_PREFIX_TIMER = "";
constexpr auto LOGGER_PREFIX_LOG = "";
constexpr auto LOGGER_PREFIX_WARNING = "";
constexpr auto LOGGER_PREFIX_ERROR = "";
#endif

static auto LOGGER_COLOR_SET(const u32 color) { return "\033[38;5;" + std::to_string(color) + "m"; }
static constexpr auto LOGGER_COLOR_CLEAR = "\033[m";

#define LOGGER_COLOR_SET_ORANGE LOGGER_COLOR_SET(202)
#define LOGGER_COLOR_SET_YELLOW LOGGER_COLOR_SET(220)
#define LOGGER_COLOR_SET_WHITE LOGGER_COLOR_SET(250)
#define LOGGER_COLOR_SET_PINK LOGGER_COLOR_SET(189)

constexpr auto LOG_LINE_STRING = "=======================================\n";        // NOLINT(*-err58-cpp)
constexpr auto LOG_SIMPLE_LINE_STRING = "---------------------------------------\n"; // NOLINT(*-err58-cpp)

static constexpr u8 START_COLOR = 172U;
static constexpr u32 DEFAULT_COLUMN_WIDTH = 12U;

struct Logger { // NOLINT(*-struct-pack-align)
    Logger() = default;

    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    Logger(Logger&&) = delete;
    Logger& operator=(Logger&&) = delete;

    ~Logger() { Print(); }

    void Print() {
        if (string.Empty()) { return; }
        ClearColor();
        (void)std::printf(string.CString()); // NOLINT(*-vararg)
        string.Clear();
    }

    template <typename... Args> constexpr void Log(const char* text, Args... args) {
        string += LOGGER_PREFIX_LOG;
        string += std::vformat(text, std::make_format_args(args...));
        string += "\n";
    }
    template <typename... Args> constexpr void Write(const char* text, Args... args) {
        string += LOGGER_PREFIX_NONE;
        string += std::vformat(text, std::make_format_args(args...));
        string += "\n";
    }

    constexpr void LogLine() { string.Add(LOG_SIMPLE_LINE_STRING); }
    constexpr void LogComplexLine() { string.Add(LOG_LINE_STRING); }

    void SetColor(u8 color) { string.Add(std::format("\033[38;5;{}m", color)); }
    void RotateColor(u32 index) { SetColor(static_cast<u8>(START_COLOR + (index * 3U))); }
    constexpr void ClearColor() { string.Add(LOGGER_COLOR_CLEAR); }

    template <typename T> constexpr void LogList(const String& label, const String& value_label, const Span<T> span) {
        for (u32 i = 0U; i < span.size(); ++i) {
            RotateColor(i);
            string += std::format("{}{} [{}]\t{} {}\n", LOGGER_PREFIX_NONE, label, i, value_label, span[i]);
        }
        ClearColor();
    }
    template <typename T> constexpr void LogList(const String& label, const String& value_label, const List<T>& span) {
        for (u32 i = 0; i < span.size(); ++i) {
            RotateColor(i);
            string += std::format("{}{} [{}]\t{} {}\n", LOGGER_PREFIX_NONE, label, i, value_label, span[i]);
        }
        ClearColor();
    }
    template <typename T> constexpr void LogListOneLine(const String& label, const String& value_label, const Span<T> span) {
        string.Add(std::format("{}{} {}\t", LOGGER_PREFIX_NONE, label, value_label));
        for (u32 i = 0; i < span.size(); ++i) {
            RotateColor(i);
            string += std::format("[{:2}] {:10}\t", i, span[i]);
        }
        ClearColor();
        string.Add("\n");
    }

private:
    String string;
};

struct Table { // NOLINT(*-struct-pack-align)
    enum LOGGER_COLOR : bool { COLOR_DISABLED, COLOR_ENABLED };
    Table(const String& name, const u32 row_count) : rows(row_count + 1U) {
        List<u32> idx(row_count);
        std::iota(idx.begin(), idx.end(), 0U);
        AddColumn(name, Span<u32>(idx));
    }
    template <typename T> void AddColumn(String title, Span<T> values) { AddColumnFixed(title, values, title.size() + 1U); }
    template <typename T> void AddColumn(String title, List<T> values) { AddColumnFixed(title, Span<T>(values), title.size() + 1U); }
    void Print(Logger& logger, const LOGGER_COLOR coloring) const {
        const String line('-', rows[0U].size() + 1U);
        logger.Write("{}", line);
        logger.Write("|{}", rows[0U]);
        logger.Write("{}", line);
        for (u32 i = 1U; i < rows.Size(); i++) {
            if (coloring == COLOR_ENABLED) { logger.RotateColor(i - 1U); }
            logger.Write("|{}", rows[i]);
        }
        logger.ClearColor();
        logger.Write("{}", line);
        logger.Print();
    }

private:
    List<String> rows;

    template <typename T> void AddColumnFixed(String title, Span<T> values, u32 width) {
        rows[0U] += std::format("{:>{}} |", title, width);
        for (u32 i = 0U; i < values.size(); i++) { rows[i + 1U] += std::format("{:>{}} |", values[i], width); }
    }
};

template <typename T> static void PrintListStats(Logger& logger, const List<T>& list) {
    const u32 len = list.Size();
    if (len == 0U) { return; }
    const T max = *std::ranges::max_element(list, std::less(), { });
    const T min = *std::ranges::min_element(list, std::less(), { });
    const f64 tot = std::accumulate(list.begin(), list.end(), 0.0);
    const f32 avg = tot / static_cast<T>(len);
    Table table("List", 1U);
    table.AddColumn("Length", List { { len } });
    table.AddColumn("Total", List { { tot } });
    table.AddColumn("Average", List { { avg } });
    table.AddColumn("Max", List<T> { { max } });
    table.AddColumn("Min", List<T> { { min } });
    table.Print(logger, Table::COLOR_DISABLED);
}
} // namespace pce
