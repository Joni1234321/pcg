#pragma once

#include <chrono>
#include <string>

#include "types.hpp"

#define PCE_TIMER_ENABLE

#define PCE_TIMER(NAME) const pce::Timer _timer_ = pce::timer::Timer(NAME)
#define PCE_TIMER1(NAME) const pce::Timer _timer_1 = pce::timer::Timer(NAME)
#define PCE_TIMER2(NAME) const pce::Timer _timer_2 = pce::timer::Timer(NAME)

namespace pce {
class Timer {
 public:
    explicit Timer(const std::string& name) {
    start_ = std::chrono::high_resolution_clock::now();
    name_ = name;
  }
  ~Timer() {
#ifdef PCE_TIMER_ENABLE
    const std::chrono::duration<f32> duration = std::chrono::high_resolution_clock::now() - start_;
    const f32 duration_ms = duration.count() * 1000.0F;
    // logger::LogTiming(name_, duration_ms);
#endif
  }
 private:
  std::string name_;
  std::chrono::time_point<std::chrono::steady_clock> start_;
};
}
