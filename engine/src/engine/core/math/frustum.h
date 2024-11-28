#pragma once
#include "engine/core/math/geomath.h"
#include "engine/core/math/plane.h"

namespace my {

class AABB;

class Frustum {
public:
    Frustum() = default;
    Frustum(const Matrix4x4f& p_projection_view_matrix);

    Plane& operator[](int p_index) { return reinterpret_cast<Plane*>(this)[p_index]; }

    const Plane& operator[](int p_index) const { return reinterpret_cast<const Plane*>(this)[p_index]; }

    bool Intersects(const AABB& p_box) const;

private:
    Plane m_left;
    Plane m_right;
    Plane m_top;
    Plane m_bottom;
    Plane m_near;
    Plane m_far;

    friend class IntersectionTest;
};

}  // namespace my
