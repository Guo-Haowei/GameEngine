#pragma once
#include "geomath.h"

namespace my {

class Radians;
class Degree;

class Degree {
public:
    explicit constexpr Degree() : m_value(0) {}
    explicit constexpr Degree(float degree) : m_value(degree) {}

    Degree& operator=(const Radians& rad);

    Degree& operator+=(Degree v) {
        m_value += v.m_value;
        return *this;
    }
    Degree& operator-=(Degree v) {
        m_value -= v.m_value;
        return *this;
    }
    bool operator==(Degree v) const {
        return m_value == v.m_value;
    }
    bool operator!=(Degree v) const {
        return m_value != v.m_value;
    }
    bool operator<(Degree v) const {
        return m_value < v.m_value;
    }
    bool operator>(Degree v) const {
        return m_value > v.m_value;
    }
    bool operator<=(Degree v) const {
        return m_value <= v.m_value;
    }
    bool operator>=(Degree v) const {
        return m_value >= v.m_value;
    }
    void clamp(float a, float b) { m_value = glm::clamp(m_value, a, b); }
    float to_rad() const { return glm::radians(m_value); }
    float get_degree() const { return m_value; }
    float sin() const { return glm::sin(to_rad()); }
    float cos() const { return glm::cos(to_rad()); }

private:
    float m_value;
};

class Radians {
public:
    explicit constexpr Radians() : m_value(0) {}
    explicit constexpr Radians(float angle) : m_value(angle) {}

    Radians& operator=(const Degree& degree);

    Radians operator*(float v) const {
        return Radians{ m_value * v };
    }
    Radians operator/(float v) const {
        return Radians{ m_value / v };
    }
    Radians& operator*=(float v) {
        m_value *= v;
        return *this;
    }
    Radians& operator/=(float v) {
        m_value /= v;
        return *this;
    }

    Radians& operator+=(Radians v) {
        m_value += v.m_value;
        return *this;
    }
    Radians& operator-=(Radians v) {
        m_value -= v.m_value;
        return *this;
    }
    Radians& operator+=(Degree v) {
        m_value += v.to_rad();
        return *this;
    }
    Radians& operator-=(Degree v) {
        m_value -= v.to_rad();
        return *this;
    }
    void clamp(float a, float b) { m_value = glm::clamp(m_value, a, b); }
    float to_degree() const { return glm::degrees(m_value); }
    float get_rad() const { return m_value; }
    float sin() const { return glm::sin(m_value); }
    float cos() const { return glm::cos(m_value); }

private:
    float m_value;
};

}  // namespace my
