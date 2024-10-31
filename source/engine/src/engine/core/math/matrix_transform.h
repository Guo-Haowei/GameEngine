#pragma once
#include "geomath.h"

namespace my {

inline mat4 BuildPerspectiveLH(float p_fovy, float p_aspect, float p_near, float p_far) {
    const float tan_half_fovy = glm::tan(0.5f * p_fovy);
    mat4 result(0.0f);
    result[0][0] = 1.0f / (p_aspect * tan_half_fovy);
    result[1][1] = 1.0f / tan_half_fovy;
    result[2][2] = p_far / (p_far - p_near);
    result[2][3] = 1.0f;
    result[3][2] = -(p_far * p_near) / (p_far - p_near);
    return result;
}

inline mat4 BuildPerspectiveRH(float p_fovy, float p_aspect, float p_near, float p_far) {
    const float tan_half_fovy = glm::tan(0.5f * p_fovy);
    mat4 result(0.0f);
    result[0][0] = 1.0f / (p_aspect * tan_half_fovy);
    result[1][1] = 1.0f / tan_half_fovy;
    result[2][2] = -p_far / (p_far - p_near);
    result[2][3] = -1.0f;
    result[3][2] = -(p_far * p_near) / (p_far - p_near);
    return result;
}

inline mat4 BuildOpenGLPerspectiveRH(float p_fovy, float p_aspect, float p_near, float p_far) {
    const float tan_half_fovy = glm::tan(0.5f * p_fovy);
    mat4 Result(0.0f);
    Result[0][0] = 1.0f / (p_aspect * tan_half_fovy);
    Result[1][1] = 1.0f / tan_half_fovy;
    Result[2][2] = -(p_far + p_near) / (p_far - p_near);
    Result[2][3] = -1.0f;
    Result[3][2] = -(2.0f * p_far * p_near) / (p_far - p_near);
    return Result;
}

inline mat4 BuildOrthoRH(const float p_left,
                         const float p_right,
                         const float p_bottom,
                         const float p_top,
                         const float p_near,
                         const float p_far) {

    const float reciprocal_width = 1.0f / (p_right - p_left);
    const float reciprocal_height = 1.0f / (p_top - p_bottom);
    const float reciprocal_depth = 1.0f / (p_far - p_near);

    mat4 result(1.0f);
    result[0][0] = 2.0f * reciprocal_width;
    result[1][1] = 2.0f * reciprocal_height;
    result[2][2] = -1.0f * reciprocal_depth;
    result[3][0] = -(p_right + p_left) * reciprocal_width;
    result[3][1] = -(p_top + p_bottom) * reciprocal_height;
    result[3][2] = -p_near * reciprocal_depth;
    return result;
}

inline mat4 BuildOpenGLOrthoRH(const float p_left,
                               const float p_right,
                               const float p_bottom,
                               const float p_top,
                               const float p_near,
                               const float p_far) {

    const float reciprocal_width = 1.0f / (p_right - p_left);
    const float reciprocal_height = 1.0f / (p_top - p_bottom);
    const float reciprocal_depth = 1.0f / (p_far - p_near);

    mat4 result(1.0f);
    result[0][0] = 2.0f * reciprocal_width;
    result[1][1] = 2.0f * reciprocal_height;
    result[2][2] = -2.0f * reciprocal_depth;
    result[3][0] = -(p_right + p_left) * reciprocal_width;
    result[3][1] = -(p_top + p_bottom) * reciprocal_height;
    result[3][2] = -(p_far + p_near) * reciprocal_depth;
    return result;
}

inline std::array<mat4, 6> BuildCubeMapViewMatrices(const vec3& p_eye) {
    std::array<mat4, 6> matrices;

    matrices[0] = glm::lookAtLH(p_eye, p_eye + glm::vec3(+1, +0, +0), glm::vec3(0, +1, +0));
    matrices[1] = glm::lookAtLH(p_eye, p_eye + glm::vec3(-1, +0, +0), glm::vec3(0, +1, +0));
    matrices[2] = glm::lookAtLH(p_eye, p_eye + glm::vec3(+0, +1, +0), glm::vec3(0, +0, -1));
    matrices[3] = glm::lookAtLH(p_eye, p_eye + glm::vec3(+0, -1, +0), glm::vec3(0, +0, +1));
    matrices[4] = glm::lookAtLH(p_eye, p_eye + glm::vec3(+0, +0, +1), glm::vec3(0, +1, +0));
    matrices[5] = glm::lookAtLH(p_eye, p_eye + glm::vec3(+0, +0, -1), glm::vec3(0, +1, +0));
    return matrices;
}

inline std::array<mat4, 6> BuildOpenGLCubeMapViewMatrices(const vec3& p_eye) {
    std::array<mat4, 6> matrices;
    matrices[0] = glm::lookAtRH(p_eye, p_eye + glm::vec3(+1, +0, +0), glm::vec3(0, -1, +0));
    matrices[1] = glm::lookAtRH(p_eye, p_eye + glm::vec3(-1, +0, +0), glm::vec3(0, -1, +0));
    matrices[2] = glm::lookAtRH(p_eye, p_eye + glm::vec3(+0, +1, +0), glm::vec3(0, +0, +1));
    matrices[3] = glm::lookAtRH(p_eye, p_eye + glm::vec3(+0, -1, +0), glm::vec3(0, +0, -1));
    matrices[4] = glm::lookAtRH(p_eye, p_eye + glm::vec3(+0, +0, +1), glm::vec3(0, -1, +0));
    matrices[5] = glm::lookAtRH(p_eye, p_eye + glm::vec3(+0, +0, -1), glm::vec3(0, -1, +0));
    return matrices;
}

}  // namespace my