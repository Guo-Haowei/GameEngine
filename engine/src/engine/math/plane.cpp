#include "plane.h"

namespace my {

float Plane::Distance(const Vector3f& p_point) const {
    return dot(p_point, normal) + dist;
}

}  // namespace my
