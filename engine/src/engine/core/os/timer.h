#pragma once

namespace my {

constexpr inline uint64_t MILLISECOND = 1000000;
constexpr inline uint64_t SECOND = MILLISECOND * 1000;

struct NanoSecond {
    NanoSecond(const uint64_t p_value) : m_value(p_value) {}

    double ToMillisecond() const {
        constexpr double factor = 1.0 / MILLISECOND;
        return factor * m_value;
    }

    double ToSecond() const {
        constexpr double factor = 1.0 / SECOND;
        return factor * m_value;
    }

    uint64_t m_value;
};

class Timer {
    using Clock = std::chrono::high_resolution_clock;
    using TimePoint = std::chrono::time_point<Clock>;

public:
    Timer() { Start(); }

    void Start();

    NanoSecond GetDuration() const;
    std::string GetDurationString() const;

protected:
    TimePoint m_startPoint{};
};

}  // namespace my
