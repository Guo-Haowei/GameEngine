#pragma once

WARNING_PUSH()
WARNING_DISABLE(4201, "-Wunused-parameter")
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtx/vector_angle.hpp>
WARNING_POP()

#include "vector_math.h"

namespace my {

using Vector2f = Vector<float, 2>;
using Vector3f = glm::vec3;
using Vector4f = glm::vec4;

using Vector2i = Vector<int32_t, 2>;
using Vector3i = Vector<int32_t, 3>;
using Vector4i = Vector<int32_t, 4>;

using Vector2u = Vector<uint32_t, 2>;
using Vector3u = Vector<uint32_t, 3>;
using Vector4u = Vector<uint32_t, 4>;

using Matrix3x3f = glm::mat3;
using Matrix4x4f = glm::mat4;

using Quaternion = glm::quat;

template<typename T>
constexpr inline float Saturate(T p_x) { return glm::min(T(1), glm::max(T(0), p_x)); }

static inline void Decompose(const Matrix4x4f& p_matrix, Vector3f& p_scale, Vector4f& p_rotation, Vector3f& p_translation) {
    Vector3f _skew;
    Vector4f _perspective;
    Quaternion quaternion;
    glm::decompose(p_matrix, p_scale, quaternion, p_translation, _skew, _perspective);
    p_rotation.x = quaternion.x;
    p_rotation.y = quaternion.y;
    p_rotation.z = quaternion.z;
    p_rotation.w = quaternion.w;
}

}  // namespace my
