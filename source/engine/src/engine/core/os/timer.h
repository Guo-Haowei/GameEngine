#pragma once

namespace my {

inline constexpr uint64_t MILLISECOND = 1000000;
inline constexpr uint64_t SECOND = MILLISECOND * 1000;

struct NanoSecond {
    NanoSecond(const uint64_t p_value) : m_value(p_value) {}

    double toMillisecond() const {
        constexpr double factor = 1.0 / MILLISECOND;
        return factor * m_value;
    }

    double toSecond() const {
        constexpr double factor = 1.0 / SECOND;
        return factor * m_value;
    }

    uint64_t m_value;
};

class Timer {
    using Clock = std::chrono::high_resolution_clock;
    using TimePoint = std::chrono::time_point<Clock>;

public:
    Timer() { start(); }

    void start();

    NanoSecond getDuration() const;
    std::string getDurationString() const;

protected:
    TimePoint m_start_point{};
};

}  // namespace my
