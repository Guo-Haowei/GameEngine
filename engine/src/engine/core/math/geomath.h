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

using Matrix4x4f = glm::mat4;

using Quaternion = glm::quat;

template<typename T>
constexpr inline float Saturate(T p_x) { return glm::min(T(1), glm::max(T(0), p_x)); }

static inline void Decompose(const Matrix4x4f& p_matrix, Vector3f& p_scale, Vector4f& p_rotation, Vector3f& p_translation) {
    glm::vec3 scale;
    glm::vec3 translation;
    Quaternion quaternion;
    glm::vec3 _skew;
    glm::vec4 _perspective;

    glm::decompose(p_matrix, scale, quaternion, translation, _skew, _perspective);
    p_rotation.x = quaternion.x;
    p_rotation.y = quaternion.y;
    p_rotation.z = quaternion.z;
    p_rotation.w = quaternion.w;

    p_scale.Set(&scale.x);
    p_translation.Set(&translation.x);
}

}  // namespace my
