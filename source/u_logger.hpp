#pragma once

#include <algorithm>
#include <cstdio>
#include <format>
#include <numeric>
#include <string>
#include <array>

#include "u_collections.hpp"
#include "u_types.hpp"
#include "u_ecs.hpp"

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

#define LOGGER_ERROR_WRITE(MESSAGE) Logger logger; logger.Error("ASSERT FAILED: {}\nFile:{}\nLine:{}", MESSAGE, __FILE__, __LINE__)
#define LOGGER_ERROR_WRITE_RETURN(MESSAGE, RETURN) Logger logger; logger.Error("ASSERT FAILED: {}\nFile:{}\nLine:{}", MESSAGE, __FILE__, __LINE__); return RETURN
#define FAILED(MESSAGE) Logger logger; logger.Error("ASSERT FAILED: {}\nFile:{}\nLine:{}", MESSAGE, __FILE__, __LINE__);
#define ASSERT_DBG(CONDITION, MESSAGE) if (CONDITION) { Logger logger; logger.Error("ASSERT FAILED: {}\nFile:{}\nLine:{}", MESSAGE, __FILE__, __LINE__); }
#define ASSERT_DBG_RETURN(CONDITION, MESSAGE, RETURN) if (!static_cast<b8>(CONDITION)) { Logger logger; logger.Error("ASSERT FAILED: {}\nFile:{}\nLine:{}", MESSAGE, __FILE__, __LINE__); return RETURN; }

static auto LoggerColorSet(const u32 color) { return "\033[38;5;" + std::to_string(color) + "m"; }
static constexpr auto LOGGER_COLOR_CLEAR = "\033[m";

constexpr auto LOG_LINE_STRING = "=======================================\n"; // NOLINT(*-err58-cpp)
constexpr auto LOG_SIMPLE_LINE_STRING = "---------------------------------------\n"; // NOLINT(*-err58-cpp)

static constexpr u8 START_COLOR = 172U;
static constexpr u32 DEFAULT_COLUMN_WIDTH = 12U;

struct Logger {
    // NOLINT(*-struct-pack-align)
    enum class LOGGER_COLOR : u8 { ORANGE = 202U, YELLOW = 220U, WHITE = 250U, PINK = 189U, RED = 196U };

    Logger() = default;

    Logger(const Logger &) = delete;

    Logger &operator=(const Logger &) = delete;

    Logger(Logger &&) = delete;

    Logger &operator=(Logger &&) = delete;

    ~Logger() { Print(); }

    void Print() {
        if (string.Empty()) { return; }
        ClearColor();
        (void) std::printf(string.CString()); // NOLINT(*-vararg)
        string.Clear();
    }

    template<typename... Args>
    constexpr void Log(const char *text, Args... args) {
        string += LOGGER_PREFIX_LOG;
        string += std::vformat(text, std::make_format_args(args...));
        string += "\n";
    }

    template<typename... Args>
    constexpr void Error(const char *text, Args... args) {
        SetColor(static_cast<u8>(LOGGER_COLOR::RED));
        string += LOGGER_PREFIX_ERROR;
        string += std::vformat(text, std::make_format_args(args...));
        string += "\n";
        ClearColor();
    }
    template<typename... Args>
    constexpr void ErrorWithFile(const char *text, Args... args) {
        SetColor(static_cast<u8>(LOGGER_COLOR::RED));
        string += LOGGER_PREFIX_ERROR;
        string += std::vformat(text, std::make_format_args(args...));
        string += std::format("Line[{}] FILE: [{}]", __LINE__, __FILE__);
        string += "\n";
        ClearColor();
    }

    template<typename... Args>
    constexpr void Write(const char *text, Args... args) {
        string += LOGGER_PREFIX_NONE;
        string += std::vformat(text, std::make_format_args(args...));
        string += "\n";
    }

    constexpr void LogLine() { string.Add(LOG_SIMPLE_LINE_STRING); }
    constexpr void LogComplexLine() { string.Add(LOG_LINE_STRING); }

    void SetColor(u8 color) { string.Add(std::format("\033[38;5;{}m", color)); }
    void RotateColor(const u32 index) { SetColor(static_cast<u8>(START_COLOR + (index * 3U))); }
    constexpr void ClearColor() { string.Add(LOGGER_COLOR_CLEAR); }

    template<typename T>
    constexpr void LogList(const String &label, const String &value_label, const Span<T> span) {
        for (u32 i = 0U; i < span.size(); ++i) {
            RotateColor(i);
            string += std::format("{}{} [{}]\t{} {}\n", LOGGER_PREFIX_NONE, label, i, value_label, span[i]);
        }
        ClearColor();
    }

    template<typename T>
    constexpr void LogList(const String &label, const String &value_label, const List<T> &span) {
        for (u32 i = 0; i < span.size(); ++i) {
            RotateColor(i);
            string += std::format("{}{} [{}]\t{} {}\n", LOGGER_PREFIX_NONE, label, i, value_label, span[i]);
        }
        ClearColor();
    }

    template<typename T>
    constexpr void LogListOneLine(const String &label, const String &value_label, const Span<T> span) {
        string.Add(std::format("{}{} {}\t", LOGGER_PREFIX_NONE, label, value_label));
        for (u32 i = 0; i < span.size(); ++i) {
            RotateColor(i);
            string += std::format("[{:2}] {:10}\t", i, span[i]);
        }
        ClearColor();
        string.Add("\n");
    }

    String &GetString() { return string; }

private:
    String string;
};

template<typename T>
String FormatValue(const T value) { return std::format("{} ", value); }

template<typename T>concept NamedTypeArithmetic = requires(T value)
{
    { static_cast<f32>(value) }; // Checks if T can be cast to f32
} && (HasASkill<T, pcg::FormatLongNumber>);

template<NamedTypeArithmetic T>
String FormatValue(const T value) {
    const f32 number = static_cast<f32>(value);
    f32 abs_number = math::Abs(number);
    static constexpr std::array LONG_NUMBER_SUFFIX = {'K', 'M', 'B', 'T', 'Q', 'P', 'S'};
    char suffix{' '};
    for (const char current_suffix: LONG_NUMBER_SUFFIX) {
        constexpr f32 one_thousand = 1'000.0F;
        if (abs_number < one_thousand) { break; }
        abs_number /= one_thousand;
        suffix = current_suffix;
    }
    const char prefix{number < 0.0F ? '-' : ' '};
    return std::format("{}{:.2f}{}", prefix, abs_number, suffix);
}

template<>
inline String FormatValue<Entity>(const Entity value) {
    return value == Entity::NONE ? String{"NONE"} : std::format("{} ", value);
}
} // namespace pce
