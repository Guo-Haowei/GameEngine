#pragma once

WARNING_PUSH()
WARNING_DISABLE(4201, "")
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtx/vector_angle.hpp>
WARNING_POP()

using glm::vec2;
using glm::vec3;
using glm::vec4;

using glm::ivec2;
using glm::ivec3;
using glm::ivec4;

using glm::uvec2;
using glm::uvec3;
using glm::uvec4;

using glm::mat3;
using glm::mat4;

using glm::quat;

template<typename T>
constexpr inline T Lerp(const T& p_a, const T& p_b, const T& p_f) {
    return p_a + p_f * (p_b - p_a);
}

template<typename T>
constexpr inline float Saturate(T p_x) { return glm::min(T(1), glm::max(T(0), p_x)); }

static inline void Decompose(const mat4& p_matrix, vec3& p_scale, vec4& p_rotation, vec3& p_translation) {
    vec3 _skew;
    vec4 _perspective;
    quat quaternion;
    glm::decompose(p_matrix, p_scale, quaternion, p_translation, _skew, _perspective);
    p_rotation.x = quaternion.x;
    p_rotation.y = quaternion.y;
    p_rotation.z = quaternion.z;
    p_rotation.w = quaternion.w;
}
