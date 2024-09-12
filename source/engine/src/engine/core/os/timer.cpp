#include "timer.h"

namespace my {

void Timer::start() {
    m_start_point = Clock::now();
}

NanoSecond Timer::getDuration() const {
    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(Clock::now() - m_start_point);
    return NanoSecond(static_cast<uint64_t>(duration.count()));
}

std::string Timer::getDurationString() const {
    auto duration = getDuration();

    if (duration.m_value < (SECOND / 10)) {
        return std::format("{:.2f} ms", duration.toMillisecond());
    }

    return std::format("{:.2f} seconds", duration.toSecond());
}

}  // namespace my