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
        if (string.empty()) { return; }
        ClearColor();
        (void)std::printf(string.c_str()); // NOLINT(*-vararg)
        string.clear();
    }

    template <typename... Args> constexpr void Log(const std::string& text, Args... args) { string += std::vformat(LOGGER_PREFIX_LOG + text + "\n", std::make_format_args(args...)); }
    template <typename... Args> constexpr void Log(const char* text, Args... args) { string += std::vformat(LOGGER_PREFIX_LOG + std::string(text, std::allocator<std::string>()) + "\n", std::make_format_args(args...)); }
    template <typename... Args> constexpr void Write(const std::string& text, Args... args) { string += std::vformat(LOGGER_PREFIX_NONE + text + "\n", std::make_format_args(args...)); }
    template <typename... Args> constexpr void Write(const char* text, Args... args) {
        string += std::vformat(LOGGER_PREFIX_NONE + std::string(text, std::allocator<std::string>()) + "\n", std::make_format_args(args...));
    }

    constexpr void LogLine() {
        string += LOG_SIMPLE_LINE_STRING;;
    }
    constexpr void LogComplexLine() { string += LOG_LINE_STRING; }

    void SetColor(u8 color) { string += std::format("\033[38;5;{}m", color); }
    void RotateColor(u32 index) { SetColor(static_cast<u8>(START_COLOR + (index * 3U))); }
    constexpr void ClearColor() { string += LOGGER_COLOR_CLEAR; }

    template <typename T> constexpr void LogList(const std::string& label, const std::string& value_label, const Span<T> span) {
        for (u32 i = 0U; i < span.size(); ++i) {
            RotateColor(i);
            string += std::format("{}{} [{}]\t{} {}\n", LOGGER_PREFIX_NONE, label, i, value_label, span[i]);
        }
        ClearColor();
    }
    template <typename T> constexpr void LogList(const std::string& label, const std::string& value_label, const List<T>& span) {
        for (u32 i = 0; i < span.size(); ++i) {
            RotateColor(i);
            string += std::format("{}{} [{}]\t{} {}\n", LOGGER_PREFIX_NONE, label, i, value_label, span[i]);
        }
        ClearColor();
    }
    template <typename T> constexpr void LogListOneLine(const std::string& label, const std::string& value_label, const Span<T> span) {
        string += std::format("{}{} {}\t", LOGGER_PREFIX_NONE, label, value_label);
        for (u32 i = 0; i < span.size(); ++i) {
            RotateColor(i);
            string += std::format("[{:2}] {:10}\t", i, span[i]);
        }
        ClearColor();
        string += "\n";
    }

    constexpr void LogVectorStats(const List<f32>& list) {
        const u32 len = list.size();
        if (len == 0) { return; }
        const f32 max = *std::ranges::max_element(list, std::less(), { });
        const f32 min = *std::ranges::min_element(list, std::less(), { });
        const f64 tot = std::accumulate(list.begin(), list.end(), 0.0);
        const f64 avg = tot / static_cast<f32>(len);
        LogLine();
        Log("LEN {:{}}", len, DEFAULT_COLUMN_WIDTH);
        Log("TOT {:{}.0f}", tot, DEFAULT_COLUMN_WIDTH);
        Log("MAX {:{}.0f}", max, DEFAULT_COLUMN_WIDTH);
        Log("MIN {:{}.0f}", min, DEFAULT_COLUMN_WIDTH);
        Log("AVG {:{}.0f}", avg, DEFAULT_COLUMN_WIDTH);
        LogLine();
    }

private:
    std::string string;
};

enum LOGGER_COLOR : bool { COLOR_DISABLED, COLOR_ENABLED };

struct Table { // NOLINT(*-struct-pack-align)
    Table(const std::string& name, const u32 row_count) : rows(row_count + 1U) {
        std::vector<u32> idx(row_count, std::allocator<u32>());
        std::iota(idx.begin(), idx.end(), 0U);
        AddColumn(name, Span<u32>(idx));
    }

    template <typename T> void AddColumnFixed(std::string title, Span<T> values, u32 width) {
        rows[0U] += std::format("{:>{}} |", title, width);
        for (u32 i = 0U; i < values.size(); i++) { rows[i + 1U] += std::format("{:>{}} |", values[i], width); }
    }
    template <typename T> void AddColumn(std::string title, Span<T> values) { AddColumnFixed(title, values, static_cast<u32>(title.size()) + 1U); }
    template <typename T> void AddColumn(std::string title, std::vector<T> values) { AddColumnFixed(title, Span<T>(values), title.size() + 1U); }
    template <typename T> void AddColumn(std::string title, List<T> values) { AddColumnFixed(title, Span<T>(values), title.size() + 1U); }

    void Print(Logger& logger, const LOGGER_COLOR coloring) const {
        std::string line = std::string(rows[0].size() + 1, '-', std::allocator<std::string>());
        logger.Write("{}", line);
        logger.Write("|{}", rows[0]);
        logger.Write("{}", line);
        for (u32 i = 1U; i < rows.size(); i++) {
            if (coloring == COLOR_ENABLED) { logger.RotateColor(i - 1U); }
            logger.Write("|{}", rows[i]);
        }
        logger.ClearColor();
        logger.Write("{}", line);
        logger.Print();
    }

private:
    List<std::string> rows;
};

namespace logger {
// enum PREFIX { NONE, LOG, WARNING, ERROR };
// template <typename... Args>
// constexpr void Log(std::string &s, Args... args) {
//  std::printf(LOGGER_COLOR_SET_WHITE LOGGER_PREFIX_LOG + s,
//std::forward(args)...);
// }
// template <typename... Args>
// constexpr void Print(const std::string &s, Args... args) {
//  std::printf((s + LOGGER_COLOR_CLEAR "\n").c_str(), args...);
// }
// template <typename... Args>
// constexpr void Log(const std::string &format, Args... args) {
//  Print(LOGGER_COLOR_SET_WHITE LOGGER_PREFIX_LOG + format,
//std::forward<Args>(args)...);
// }
// template <typename... Args>
// constexpr void LogWarning(const std::string &format, Args... args) {
//  Print(LOGGER_COLOR_SET_YELLOW LOGGER_PREFIX_WARNING + format,
//std::forward<Args>(args)...);
// }
// template <typename... Args>
// constexpr void LogError(const std::string &format, Args... args) {
//  Print(LOGGER_COLOR_SET_ORANGE LOGGER_PREFIX_ERROR + format,
//std::forward<Args>(args)...);
// }
// void LogTiming(const std::string &name, f32 ms) {
//  Print(LOGGER_COLOR_SET_PINK LOGGER_PREFIX_TIMER "%.2f ms \t%s", ms,
//name.c_str());
// }
//  used to debug colors
//void dbg_print_256_colours_txt() {
//  for (u32 i = 0; i < 256; i++) {
//    if (i % 16 == 0 && i != 0) std::printf("\n");
//    std::printf("\033[38;5;%dm %3d\033[m", i, i);
//  }
//}
} // namespace logger
} // namespace pce
