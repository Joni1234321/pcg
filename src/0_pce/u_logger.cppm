module;

#include <cxxabi.h>
export module pce.logger;

import std;
import pce.math;
import pce.strong;
import pce.collections;
import pce.std;

export namespace pce {
// Demangle a type name. GCC/Clang return the Itanium-mangled form from
// typeid(T).name() (e.g. "N3pce7TextureE"); MSVC already returns a readable
// name. This helper produces a readable name on both.
template <class T> inline const std::string& TypeName() {
    static const std::string cached = [] {
        const char* raw = typeid(T).name();
        int status = 0;
        char* demangled = abi::__cxa_demangle(raw, nullptr, nullptr, &status);
        std::string result = (status == 0 && demangled) ? std::string { demangled } : std::string { raw };
        std::free(demangled);
        return result;
    }();
    return cached;
}
constexpr auto LOGGER_PREFIX_NONE = "            "; // 12 chars to match [DESTROYED] + space
constexpr auto LOGGER_PREFIX_TIMER = "[TIMER    ] ";
constexpr auto LOGGER_PREFIX_LOG = "[LOG      ] ";
constexpr auto LOGGER_PREFIX_WARNING = "[WARNING  ] ";
constexpr auto LOGGER_PREFIX_ERROR = "[ERROR    ] ";

#define LOGGER_ERROR_WRITE(MESSAGE) \
    Logger logger;                  \
    logger.Error("ASSERT FAILED: {}\nFile:{}\nLine:{}", MESSAGE, __FILE__, __LINE__)
#define LOGGER_ERROR_WRITE_RETURN(MESSAGE, RETURN)                                    \
    Logger logger;                                                                    \
    logger.Error("ASSERT FAILED: {}\nFile:{}\nLine:{}", MESSAGE, __FILE__, __LINE__); \
    return RETURN
#define ASSERT_DBG(CONDITION, MESSAGE)                                                    \
    if (!static_cast<b8>(CONDITION)) {                                                    \
        pce::Logger logger;                                                               \
        logger.Error("ASSERT FAILED: {}\nFile:{}\nLine:{}", MESSAGE, __FILE__, __LINE__); \
    }
#define ASSERT_DBG_RETURN(CONDITION, MESSAGE, RETURN)                                     \
    if (!static_cast<b8>(CONDITION)) {                                                    \
        Logger logger;                                                                    \
        logger.Error("ASSERT FAILED: {}\nFile:{}\nLine:{}", MESSAGE, __FILE__, __LINE__); \
        return RETURN;                                                                    \
    }

 auto LoggerColorSet(const u32 color) { return "\033[38;5;" + std::to_string(color) + "m"; }
 constexpr auto LOGGER_COLOR_CLEAR = "\033[m";

constexpr auto LOG_LINE_STRING = "=======================================\n";        // NOLINT(*-err58-cpp)
constexpr auto LOG_SIMPLE_LINE_STRING = "---------------------------------------\n"; // NOLINT(*-err58-cpp)

 constexpr u8 START_COLOR = 172U;
 constexpr u32 DEFAULT_COLUMN_WIDTH = 12U;

struct Logger {
    // NOLINT(*-struct-pack-align)
    // Values are 256-color SGR foreground codes. SetColor emits them with
    // the bold attribute ("\033[1;38;5;<n>m") so output is vivid and bold.
    enum class LOGGER_COLOR : u8 { ORANGE = 130U, YELLOW = 136U, WHITE = 241U, PINK = 127U, RED = 124U, LIGHT_GREEN = 28U, LIGHT_RED = 160U, LIGHT_CYAN = 30U, LIGHT_BLUE = 19U, LIGHT_MAGENTA = 90U, GREY = 102U };
    Logger() = default;
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
    Logger(Logger&&) = delete;
    Logger& operator=(Logger&&) = delete;
    ~Logger() { Print(); }

    void Print() {
        if (string.empty()) { return; }
        ClearColor();
        (void)std::printf("%s", string.c_str());
        string.clear();
    }

    template <typename... Args> constexpr void Log(const char* text, Args... args) {
        SetColor(static_cast<u8>(LOGGER_COLOR::LIGHT_CYAN));
        string += LOGGER_PREFIX_LOG;
        string += std::vformat(text, std::make_format_args(args...));
        string += "\n";
        ClearColor();
    }

    template <typename... Args> constexpr void Warning(const char* text, Args... args) {
        SetColor(static_cast<u8>(LOGGER_COLOR::YELLOW));
        string += LOGGER_PREFIX_WARNING;
        string += std::vformat(text, std::make_format_args(args...));
        string += "\n";
        ClearColor();
    }

    template <typename... Args> constexpr void Error(const char* text, Args... args) {
        SetColor(static_cast<u8>(LOGGER_COLOR::RED));
        string += LOGGER_PREFIX_ERROR;
        string += std::vformat(text, std::make_format_args(args...));
        string += "\n";
        ClearColor();
    }
    template <typename... Args> constexpr void TaggedColored(const u8 color, const char* tag, const char* text, Args... args) {
        SetColor(color);
        string += std::format("[{:<9}] ", tag);
        string += std::vformat(text, std::make_format_args(args...));
        string += "\n";
        ClearColor();
    }
    template <typename... Args> constexpr void Tagged(const char* tag, const char* text, Args... args) { TaggedColored(static_cast<u8>(LOGGER_COLOR::WHITE), tag, text, args...); }
    template <typename... Args> constexpr void ColoredLog(const u8 color, const char* text, Args... args) {
        SetColor(color);
        string += LOGGER_PREFIX_LOG;
        string += std::vformat(text, std::make_format_args(args...));
        string += "\n";
        ClearColor();
    }
    template <typename... Args> constexpr void LogTaggedColored(const u8 color, const char* tag, const char* text, Args... args) {
        SetColor(color);
        string += LOGGER_PREFIX_LOG;
        string += std::format("[{:<9}] ", tag);
        string += std::vformat(text, std::make_format_args(args...));
        string += "\n";
        ClearColor();
    }
    template <typename... Args> constexpr void Destroyed(const char* text, Args... args) { LogTaggedColored(static_cast<u8>(LOGGER_COLOR::GREY), "DESTROYED", text, args...); }
    template <typename... Args> constexpr void Created(const char* text, Args... args) { LogTaggedColored(static_cast<u8>(LOGGER_COLOR::ORANGE), "CREATED", text, args...); }
    template <typename... Args> constexpr void Moved(const char* text, Args... args) { ColoredLog(static_cast<u8>(LOGGER_COLOR::LIGHT_BLUE), text, args...); }
    template <typename... Args> constexpr void Copied(const char* text, Args... args) { ColoredLog(static_cast<u8>(LOGGER_COLOR::LIGHT_MAGENTA), text, args...); }

    template <typename... Args> constexpr void ErrorWithFile(const char* text, Args... args) {
        SetColor(static_cast<u8>(LOGGER_COLOR::RED));
        string += LOGGER_PREFIX_ERROR;
        string += std::vformat(text, std::make_format_args(args...));
        string += std::format("Line[{}] FILE: [{}]", __LINE__, __FILE__);
        string += "\n";
        ClearColor();
    }

    template <typename... Args> constexpr void Write(const char* text, Args... args) {
        string += LOGGER_PREFIX_NONE;
        string += std::vformat(text, std::make_format_args(args...));
        string += "\n";
    }

    constexpr void LogLine() { string.Add(LOG_SIMPLE_LINE_STRING); }
    constexpr void LogComplexLine() { string.Add(LOG_LINE_STRING); }

    void SetColor(u8 color) { string.Add(std::format("\033[1;38;5;{}m", color)); }
    void RotateColor(const u32 index) { SetColor(static_cast<u8>(START_COLOR + index * 3U)); }
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

    String& GetString() { return string; }

private:
    String string;
};

template <typename T> struct LogLifetime {
    LogLifetime() { Logger().Created("{}", TypeName<T>()); }
    LogLifetime(const LogLifetime&) { Logger().Copied("{}", TypeName<T>()); }
    LogLifetime(LogLifetime&&) noexcept { Logger().Moved("{}", TypeName<T>()); }
    ~LogLifetime() { Logger().Destroyed("{}", TypeName<T>()); }
};
template <typename T> struct LogLifetimeWithCount {
    LogLifetimeWithCount() { Logger().Created("{} {}", TypeName<T>(), log_id); }
    LogLifetimeWithCount(const LogLifetimeWithCount&) noexcept { Logger().Copied("{} {}", TypeName<T>(), log_id); }
    LogLifetimeWithCount(LogLifetimeWithCount&& other) noexcept { Logger().Moved("{} {} -> {}", TypeName<T>(), other.log_id, log_id); }
    ~LogLifetimeWithCount() { Logger().Destroyed("{} {}", TypeName<T>(), log_id); }
    LogLifetimeWithCount& operator=(const LogLifetimeWithCount& other) {
        if (this == &other) { return *this; }
        log_id = other.log_id;
        return *this;
    }
    LogLifetimeWithCount& operator=(LogLifetimeWithCount&& other) noexcept {
        if (this == &other) { return *this; }
        log_id = other.log_id;
        return *this;
    }
    static u32 log_counter;
    u32 log_id { log_counter++ };
};
template <typename T> u32 LogLifetimeWithCount<T>::log_counter = 0U;

#if defined(__cpp_lib_stacktrace)
template <typename T> struct LogLifetimeWithStack {
    LogLifetimeWithStack() { Logger().Created("{} {}\n", TypeName<T>(), std::stacktrace::current()); }
    LogLifetimeWithStack(const LogLifetimeWithStack&) { Logger().Copied("{} {}\n", TypeName<T>(), std::stacktrace::current()); }
    LogLifetimeWithStack(LogLifetimeWithStack&&) noexcept { Logger().Moved("{} {}\n", TypeName<T>(), std::stacktrace::current()); }
    ~LogLifetimeWithStack() { Logger().Destroyed("{} {}\n", TypeName<T>(), std::stacktrace::current()); }
};
#endif
template <typename T> struct LogDestroy {
    ~LogDestroy() { Logger().Destroyed("{}", TypeName<T>()); }
};
#if defined(__cpp_lib_stacktrace)
template <typename T> struct LogDestroyWithStack {
    ~LogDestroyWithStack() {
        auto filtered_frames = std::stacktrace::current() | std::views::filter([](const std::stacktrace_entry& frame) -> bool { return frame.description().contains("pcg") || frame.description().contains("pce"); });
        String result;
        for (const std::stacktrace_entry& frame : filtered_frames) { result += frame.description() + "\n"; }
        Logger().Destroyed("{} {}\n", TypeName<T>(), result);
    }
};
#endif
template <typename T> struct LogDestroyWithCount {
    ~LogDestroyWithCount() {
        static u32 count;
        Logger().Destroyed("{} {}", TypeName<T>(), ++count);
    }
};

template <typename T> concept LongNumberFormattable = requires(T value) {
    { static_cast<f32>(value.value) }; // Checks if T can be cast to f32
} && HasASkill<T, FormatLongNumber>;

template <LongNumberFormattable T> String FormatValue(const T value) {
    const f32 number { static_cast<f32>(value.value) };
    const char prefix { number < 0.0F ? '-' : ' ' };
    f32 abs_number = math::Abs(number);
    if (abs_number < 1000.0F) { return std::format("{}{}", prefix, number); }
    static constexpr Array LONG_NUMBER_SUFFIX = { 'K', 'M', 'B', 'T', 'Q', 'P', 'S' };
    char suffix = ' ';
    for (const char current_suffix : LONG_NUMBER_SUFFIX) {
        constexpr f32 one_thousand = 1'000.0F;
        if (abs_number < one_thousand) { break; }
        abs_number /= one_thousand;
        suffix = current_suffix;
    }
    return std::format("{}{:.2f}{}", prefix, abs_number, suffix);
}
template <typename T> String FormatValue(const T value) { return std::format("{} ", value); }
template <> inline String FormatValue<Entity>(const Entity value) { return value == Entity::NONE ? String { "NONE" } : std::format("{} ", value); }
} // namespace pce
