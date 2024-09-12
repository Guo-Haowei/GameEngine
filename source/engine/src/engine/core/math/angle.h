#pragma once
#include "geomath.h"

namespace my {

class Radians;
class Degree;

class Degree {
public:
    explicit constexpr Degree() : m_value(0) {}
    explicit constexpr Degree(float p_degree) : m_value(p_degree) {}

    Degree& operator=(const Radians& p_rad);

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
    bool operator==(Degree p_val) const {
        return m_value == p_val.m_value;
    }
    bool operator!=(Degree p_val) const {
        return m_value != p_val.m_value;
    }
    bool operator<(Degree p_val) const {
        return m_value < p_val.m_value;
    }
    bool operator>(Degree p_val) const {
        return m_value > p_val.m_value;
    }
    bool operator<=(Degree p_val) const {
        return m_value <= p_val.m_value;
    }
    bool operator>=(Degree p_val) const {
        return m_value >= p_val.m_value;
    }
    void clamp(float p_a, float p_b) { m_value = glm::clamp(m_value, p_a, p_b); }
    float toRad() const { return glm::radians(m_value); }
    float getDegree() const { return m_value; }
    float sin() const { return glm::sin(toRad()); }
    float cos() const { return glm::cos(toRad()); }
    float tan() const { return glm::tan(toRad()); }

private:
    float m_value;
};

class Radians {
public:
    explicit constexpr Radians() : m_value(0) {}
    explicit constexpr Radians(float p_angle) : m_value(p_angle) {}

    Radians& operator=(const Degree& p_degree);

    Radians operator*(float p_val) const {
        return Radians{ m_value * p_val };
    }
    Radians operator/(float p_val) const {
        return Radians{ m_value / p_val };
    }
    Radians& operator*=(float p_val) {
        m_value *= p_val;
        return *this;
    }
    Radians& operator/=(float p_val) {
        m_value /= p_val;
        return *this;
    }
    Radians& operator+=(Radians p_val) {
        m_value += p_val.m_value;
        return *this;
    }
    Radians& operator-=(Radians p_val) {
        m_value -= p_val.m_value;
        return *this;
    }
    Radians& operator+=(Degree p_val) {
        m_value += p_val.toRad();
        return *this;
    }
    Radians& operator-=(Degree p_val) {
        m_value -= p_val.toRad();
        return *this;
    }
    void clamp(float p_a, float p_b) { m_value = glm::clamp(m_value, p_a, p_b); }
    float toDegree() const { return glm::degrees(m_value); }
    float getRad() const { return m_value; }
    float sin() const { return glm::sin(m_value); }
    float cos() const { return glm::cos(m_value); }
    float tan() const { return glm::tan(m_value); }

private:
    float m_value;
};

}  // namespace my
