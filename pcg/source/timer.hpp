#pragma once

#include "logger.hpp"
#include <chrono>
#include <string>

#define PCE_TIMER_ENABLE

#define PCE_TIMER(NAME) const pce::timer::Timer _timer_ = pce::timer::Timer(NAME)
#define PCE_TIMER1(NAME) const pce::timer::Timer _timer_1 = pce::timer::Timer(NAME)
#define PCE_TIMER2(NAME) const pce::timer::Timer _timer_2 = pce::timer::Timer(NAME)

namespace pce {
namespace timer {

    class Timer {
    public:
        Timer(const std::string& name)
        {
            start_ = std::chrono::high_resolution_clock::now();
            name_ = name;
        }
        ~Timer()
        {
#ifdef PCE_TIMER_ENABLE 
            const std::chrono::duration<f32> duration = std::chrono::high_resolution_clock::now() - start_;
            const f32 duration_ms = duration.count() * 1000;
            logger::LogTiming(name_, duration_ms);
#endif
        }

    private:
        std::string name_;
        std::chrono::time_point<std::chrono::steady_clock> start_;
    };
}
}
