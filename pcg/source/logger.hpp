#pragma once

#include <cstdio>
#include <format>
#include <numeric>
#include <string>
#include <vector>

#include "collections.hpp"
#include "types.hpp"

#define LOG(s) pce::logger::Log(s)
constexpr auto LOG_LINE_STRING = "=======================================";
#define LOG_LINE pce::logger::Log(LOG_LINE_STRING)

#define DISABLE_PREFIX 1
#if DISABLE_PREFIX == 1
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

constexpr auto LOGGER_COLOR_SET(u32 COLOR) {
  return "\033[38;5;" + std::to_string(COLOR) + "m";
}
constexpr auto LOGGER_COLOR_CLEAR = "\033[m";

#define LOGGER_COLOR_SET_ORANGE LOGGER_COLOR_SET(202)
#define LOGGER_COLOR_SET_YELLOW LOGGER_COLOR_SET(220)
#define LOGGER_COLOR_SET_WHITE LOGGER_COLOR_SET(250)
#define LOGGER_COLOR_SET_PINK LOGGER_COLOR_SET(189)

namespace pce {
namespace logger {
enum PREFIX { NONE, LOG, WARNING, ERROR };
// template <typename... Args>
// constexpr void Log(std::string &s, Args... args) {
//	std::printf(LOGGER_COLOR_SET_WHITE LOGGER_PREFIX_LOG + s,
//std::forward(args)...);
// }
// template <typename... Args>
// constexpr void Print(const std::string &s, Args... args) {
//	std::printf((s + LOGGER_COLOR_CLEAR "\n").c_str(), args...);
// }
// template <typename... Args>
// constexpr void Log(const std::string &format, Args... args) {
//	Print(LOGGER_COLOR_SET_WHITE LOGGER_PREFIX_LOG + format,
//std::forward<Args>(args)...);
// }
// template <typename... Args>
// constexpr void LogWarning(const std::string &format, Args... args) {
//	Print(LOGGER_COLOR_SET_YELLOW LOGGER_PREFIX_WARNING + format,
//std::forward<Args>(args)...);
// }
// template <typename... Args>
// constexpr void LogError(const std::string &format, Args... args) {
//	Print(LOGGER_COLOR_SET_ORANGE LOGGER_PREFIX_ERROR + format,
//std::forward<Args>(args)...);
// }
// void LogTiming(const std::string &name, f32 ms) {
//	Print(LOGGER_COLOR_SET_PINK LOGGER_PREFIX_TIMER "%.2f ms \t%s", ms,
//name.c_str());
// }
//  used to debug colors
//void dbg_print_256_colours_txt() {
//  for (u32 i = 0; i < 256; i++) {
//    if (i % 16 == 0 && i != 0) std::printf("\n");
//    std::printf("\033[38;5;%dm %3d\033[m", i, i);
//  }
//}
}  // namespace logger
struct Logger {
 public:
  void Print() {
    string.pop_back();
    std::printf(string.c_str());
    string.clear();
  }
  ~Logger() {
    if (string.length() > 0) Print();
  }

  template <typename... Args>
  constexpr void Log(const std::string &s, Args... args) {
    string += std::vformat(LOGGER_PREFIX_LOG + s + "\n",
                           std::make_format_args(args...));
  }

  constexpr void LogLine() {
    string += LOG_LINE_STRING;
    string += "\n";
  }

  constexpr void SetColor(u8 i) { string += std::format("\033[38;5;{}m", i); }
  constexpr void ClearColor() { string += LOGGER_COLOR_CLEAR; }

  template <typename T>
  constexpr void LogList(const std::string &label,
                         const std::string &value_label, const Span<T> span) {
    for (u32 i = 0; i < span.size(); ++i) {
      SetColor(172 + i * 3);
      string += std::format("{}{} [{}]\t{} {}\n", LOGGER_PREFIX_NONE, label, i,
                            value_label, span[i]);
    }
    ClearColor();
  }
  template <typename T>
  constexpr void LogListOneLine(const std::string &label,
                                const std::string &value_label,
                                const Span<T> span) {
    string += std::format("{}{} {}\t", LOGGER_PREFIX_NONE, label, value_label);
    for (u32 i = 0; i < span.size(); ++i) {
      SetColor(172 + i * 3);
      // Add("[{:2}] {:10}\t", i, span[i]);
      string += std::format("[{:2}] {:10}\t", i, span[i]);
    }
    ClearColor();
    string += "\n";
  }

  constexpr void LogVectorStats(const List<f32> &v) {
    auto len = v.size();
    if (len == 0) return;
    auto max = *std::max_element(v.begin(), v.end());
    auto min = *std::min_element(v.begin(), v.end());
    auto tot = std::accumulate(v.begin(), v.end(), 0.0);
    auto avg = tot / len;
    const int w = 10;
    LogLine();
    Log("TOT {:{}.0f}", tot, w);
    Log("LEN {:{}}", len, w);
    Log("MAX {:{}.0f}", max, w);
    Log("MIN {:{}.0f}", min, w);
    Log("AVG {:{}.0f}", avg, w);
    LogLine();
  }

 private:
  std::string string;
};
};  // namespace pce
