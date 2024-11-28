#include "timer.h"

namespace my {

void Timer::Start() {
    m_startPoint = Clock::now();
}

NanoSecond Timer::GetDuration() const {
    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(Clock::now() - m_startPoint);
    return NanoSecond(static_cast<uint64_t>(duration.count()));
}

std::string Timer::GetDurationString() const {
    auto duration = GetDuration();

    if (duration.m_value < (SECOND / 10)) {
        return std::format("{:.2f} ms", duration.ToMillisecond());
    }

    return std::format("{:.2f} seconds", duration.ToSecond());
}

}  // namespace my