#include "plane.h"

namespace my {

float Plane::Distance(const Vector3f& p_point) const {
    return math::Dot(p_point, normal) + dist;
}

}  // namespace my
