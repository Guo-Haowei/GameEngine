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
    void Clamp(float p_a, float p_b) { m_value = glm::clamp(m_value, p_a, p_b); }
    float GetRadians() const { return glm::radians(m_value); }
    float GetDegree() const { return m_value; }
    float Sin() const { return glm::sin(GetRadians()); }
    float Cos() const { return glm::cos(GetRadians()); }
    float Tan() const { return glm::tan(GetRadians()); }

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
        m_value += p_val.GetRadians();
        return *this;
    }
    Radians& operator-=(Degree p_val) {
        m_value -= p_val.GetRadians();
        return *this;
    }
    void Clamp(float p_a, float p_b) { m_value = glm::clamp(m_value, p_a, p_b); }
    float ToDegree() const { return glm::degrees(m_value); }
    float GetRad() const { return m_value; }
    float Sin() const { return glm::sin(m_value); }
    float Cos() const { return glm::cos(m_value); }
    float Tan() const { return glm::tan(m_value); }

private:
    float m_value;
};

}  // namespace my
