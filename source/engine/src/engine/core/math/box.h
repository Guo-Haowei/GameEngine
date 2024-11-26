#pragma once
#include "core/math/geomath.h"

namespace my {

template<int N>
class Box {
    using vec_type = glm::vec<N, float, glm::defaultp>;
    using self_type = Box<N>;

public:
    static constexpr float BOX_MIN_SIZE = 0.0001f;

    Box() { MakeInvalid(); }

    Box(const vec_type& p_min, const vec_type& p_max) : m_min(p_min), m_max(p_max) {}

    void MakeInvalid() {
        m_min = vec_type(std::numeric_limits<float>::infinity());
        m_max = vec_type(-std::numeric_limits<float>::infinity());
    }

    bool IsValid() const {
        for (int i = 0; i < N; ++i) {
            if (m_min[i] >= m_max[i]) {
                return false;
            }
        }
        return true;
    }

    void MakeValid() {
        const vec_type size = m_max - m_min;
        if (size.x == 0.0f) {
            m_min.x -= BOX_MIN_SIZE;
            m_max.x += BOX_MIN_SIZE;
        }
        if (size.y == 0.0f) {
            m_min.y -= BOX_MIN_SIZE;
            m_max.y += BOX_MIN_SIZE;
        }
        if constexpr (N > 2) {
            if (size.z == 0.0f) {
                m_min.z -= BOX_MIN_SIZE;
                m_max.z += BOX_MIN_SIZE;
            }
        }
    }

    void ExpandPoint(const vec_type& p_point) {
        m_min = glm::min(m_min, p_point);
        m_max = glm::max(m_max, p_point);
    }

    void UnionBox(const self_type& p_other) {
        m_min = glm::min(m_min, p_other.m_min);
        m_max = glm::max(m_max, p_other.m_max);
    }

    void IntersectBox(const self_type& p_other) {
        m_min = glm::max(m_min, p_other.m_min);
        m_max = glm::min(m_max, p_other.m_max);
    }

    float SurfaceArea() const;

    vec_type Center() const { return 0.5f * (m_min + m_max); }
    vec_type Size() const { return m_max - m_min; }

    const vec_type& GetMin() const { return m_min; }
    const vec_type& GetMax() const { return m_max; }

protected:
    vec_type m_min;
    vec_type m_max;
};

using Box2 = Box<2>;
using Box3 = Box<3>;

}  // namespace my
