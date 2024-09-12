#pragma once
#include "core/math/geomath.h"

namespace my {

template<int N>
class Box {
    using vec_type = glm::vec<N, float, glm::defaultp>;
    using self_type = Box<N>;

public:
    Box() { makeInvalid(); }

    Box(const vec_type& p_min, const vec_type& p_max) : m_min(p_min), m_max(p_max) {}

    void makeInvalid() {
        m_min = vec_type(std::numeric_limits<float>::infinity());
        m_max = vec_type(-std::numeric_limits<float>::infinity());
    }

    bool isValid() {
        for (int i = 0; i < N; ++i) {
            if (m_min[i] >= m_max[i]) {
                return false;
            }
        }
        return true;
    }

    void makeValid() {
        const vec_type size = glm::abs(m_max - m_min);
        constexpr float delta = 0.0001f;
        if (size.x == 0.0f) {
            m_min.x -= delta;
            m_max.x += delta;
        }
        if (size.y == 0.0f) {
            m_min.y -= delta;
            m_max.y += delta;
        }
        if constexpr (N > 2) {
            if (size.z == 0.0f) {
                m_min.z -= delta;
                m_max.z += delta;
            }
        }
    }

    void expandPoint(const vec_type& p_point) {
        m_min = glm::min(m_min, p_point);
        m_max = glm::max(m_max, p_point);
    }

    void unionBox(const self_type& p_other) {
        m_min = glm::min(m_min, p_other.m_min);
        m_max = glm::max(m_max, p_other.m_max);
    }

    void intersectBox(const self_type& p_other) {
        m_min = glm::max(m_min, p_other.m_min);
        m_max = glm::min(m_max, p_other.m_max);
    }

    vec_type center() const { return 0.5f * (m_min + m_max); }
    vec_type size() const { return m_max - m_min; }

    const vec_type& getMin() const { return m_min; }
    const vec_type& getMax() const { return m_max; }

protected:
    vec_type m_min;
    vec_type m_max;
};

using Box2 = Box<2>;
using Box3 = Box<3>;

}  // namespace my
