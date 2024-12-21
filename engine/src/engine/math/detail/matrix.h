#pragma once
#include "vector4.h"

WARNING_PUSH()
WARNING_DISABLE(4201, "-Wunused-parameter")
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
WARNING_POP()

namespace my::math {

constexpr inline Vector<float, 4> operator*(const glm::mat4& p_lhs, const Vector<float, 4>& p_rhs) {
    glm::vec4 tmp(p_rhs.x, p_rhs.y, p_rhs.z, p_rhs.w);
    tmp = p_lhs * tmp;
    return Vector<float, 4>(tmp.x, tmp.y, tmp.z, tmp.w);
}

}  // namespace my::math
