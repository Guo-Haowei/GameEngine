#pragma once
#include "core/math/geomath.h"
#include "core/math/plane.h"

namespace my {

class AABB;

class Frustum {
public:
    Frustum() = default;
    Frustum(const mat4& projection_view_matrix);

    Plane& operator[](int i) { return reinterpret_cast<Plane*>(this)[i]; }

    const Plane& operator[](int i) const { return reinterpret_cast<const Plane*>(this)[i]; }

    bool intersects(const AABB& box) const;

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
