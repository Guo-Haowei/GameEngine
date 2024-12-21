#pragma once
// @TODO: refactor
#include "vector_math.h"

namespace my {

class Radian;
class Degree;

class Degree {
public:
    explicit constexpr Degree() : m_value(0) {}
    explicit constexpr Degree(float p_degree) : m_value(p_degree) {}
    explicit constexpr Degree(double p_degree) : m_value(static_cast<float>(p_degree)) {}

    Degree& operator=(const Radian& p_rad);

    Degree operator*(float p_val) const {
        return Degree{ m_value * p_val };
    }
    Degree operator/(float p_val) const {
        return Degree{ m_value / p_val };
    }
    Degree& operator*=(float p_val) {
        m_value *= p_val;
        return *this;
    }
    Degree& operator/=(float p_val) {
        m_value /= p_val;
        return *this;
    }
    Degree& operator+=(Degree p_val) {
        m_value += p_val.m_value;
        return *this;
    }
    Degree& operator-=(Degree p_val) {
        m_value -= p_val.m_value;
        return *this;
    }

    constexpr bool operator==(Degree p_val) const {
        return m_value == p_val.m_value;
    }
    constexpr bool operator!=(Degree p_val) const {
        return m_value != p_val.m_value;
    }
    constexpr bool operator<(Degree p_val) const {
        return m_value < p_val.m_value;
    }
    constexpr bool operator>(Degree p_val) const {
        return m_value > p_val.m_value;
    }
    constexpr bool operator<=(Degree p_val) const {
        return m_value <= p_val.m_value;
    }
    constexpr bool operator>=(Degree p_val) const {
        return m_value >= p_val.m_value;
    }
    constexpr Degree operator-() {
        return Degree(-m_value);
    }

    void Clamp(float p_a, float p_b) { m_value = math::Clamp(m_value, p_a, p_b); }
    constexpr float GetRadians() const { return math::Radians(m_value); }
    constexpr float GetDegree() const { return m_value; }
    float Sin() const { return std::sin(GetRadians()); }
    float Cos() const { return std::cos(GetRadians()); }
    float Tan() const { return std::tan(GetRadians()); }

private:
    float m_value;
};

class Radian {
public:
    explicit constexpr Radian() : m_value(0) {}
    explicit constexpr Radian(float p_angle) : m_value(p_angle) {}

    Radian& operator=(const Degree& p_degree);

    Radian operator*(float p_val) const {
        return Radian{ m_value * p_val };
    }
    Radian operator/(float p_val) const {
        return Radian{ m_value / p_val };
    }
    Radian& operator*=(float p_val) {
        m_value *= p_val;
        return *this;
    }
    Radian& operator/=(float p_val) {
        m_value /= p_val;
        return *this;
    }
    Radian& operator+=(Radian p_val) {
        m_value += p_val.m_value;
        return *this;
    }
    Radian& operator-=(Radian p_val) {
        m_value -= p_val.m_value;
        return *this;
    }
    Radian& operator+=(Degree p_val) {
        m_value += p_val.GetRadians();
        return *this;
    }
    Radian& operator-=(Degree p_val) {
        m_value -= p_val.GetRadians();
        return *this;
    }

    void Clamp(float p_a, float p_b) { m_value = math::Clamp(m_value, p_a, p_b); }
    // @TODO: rename for consistancy
    float ToDegree() const { return math::Degrees(m_value); }
    float GetRad() const { return m_value; }
    float Sin() const { return std::sin(m_value); }
    float Cos() const { return std::cos(m_value); }
    float Tan() const { return std::tan(m_value); }

private:
    float m_value;
};

}  // namespace my
