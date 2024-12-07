#pragma once
#include "geomath.h"

namespace my {

inline Matrix4x4f BuildPerspectiveLH(float p_fovy, float p_aspect, float p_near, float p_far) {
    const float tan_half_fovy = glm::tan(0.5f * p_fovy);
    Matrix4x4f result(0.0f);
    result[0][0] = 1.0f / (p_aspect * tan_half_fovy);
    result[1][1] = 1.0f / tan_half_fovy;
    result[2][2] = p_far / (p_far - p_near);
    result[2][3] = 1.0f;
    result[3][2] = -(p_far * p_near) / (p_far - p_near);
    return result;
}

inline Matrix4x4f BuildPerspectiveRH(float p_fovy, float p_aspect, float p_near, float p_far) {
    const float tan_half_fovy = glm::tan(0.5f * p_fovy);
    Matrix4x4f result(0.0f);
    result[0][0] = 1.0f / (p_aspect * tan_half_fovy);
    result[1][1] = 1.0f / tan_half_fovy;
    result[2][2] = -p_far / (p_far - p_near);
    result[2][3] = -1.0f;
    result[3][2] = -(p_far * p_near) / (p_far - p_near);
    return result;
}

inline Matrix4x4f BuildOpenGlPerspectiveRH(float p_fovy, float p_aspect, float p_near, float p_far) {
    const float tan_half_fovy = glm::tan(0.5f * p_fovy);
    Matrix4x4f Result(0.0f);
    Result[0][0] = 1.0f / (p_aspect * tan_half_fovy);
    Result[1][1] = 1.0f / tan_half_fovy;
    Result[2][2] = -(p_far + p_near) / (p_far - p_near);
    Result[2][3] = -1.0f;
    Result[3][2] = -(2.0f * p_far * p_near) / (p_far - p_near);
    return Result;
}

inline Matrix4x4f BuildOrthoRH(const float p_left,
                               const float p_right,
                               const float p_bottom,
                               const float p_top,
                               const float p_near,
                               const float p_far) {

    const float reciprocal_width = 1.0f / (p_right - p_left);
    const float reciprocal_height = 1.0f / (p_top - p_bottom);
    const float reciprocal_depth = 1.0f / (p_far - p_near);

    Matrix4x4f result(1.0f);
    result[0][0] = 2.0f * reciprocal_width;
    result[1][1] = 2.0f * reciprocal_height;
    result[2][2] = -1.0f * reciprocal_depth;
    result[3][0] = -(p_right + p_left) * reciprocal_width;
    result[3][1] = -(p_top + p_bottom) * reciprocal_height;
    result[3][2] = -p_near * reciprocal_depth;
    return result;
}

inline Matrix4x4f BuildOpenGlOrthoRH(const float p_left,
                                     const float p_right,
                                     const float p_bottom,
                                     const float p_top,
                                     const float p_near,
                                     const float p_far) {

    const float reciprocal_width = 1.0f / (p_right - p_left);
    const float reciprocal_height = 1.0f / (p_top - p_bottom);
    const float reciprocal_depth = 1.0f / (p_far - p_near);

    Matrix4x4f result(1.0f);
    result[0][0] = 2.0f * reciprocal_width;
    result[1][1] = 2.0f * reciprocal_height;
    result[2][2] = -2.0f * reciprocal_depth;
    result[3][0] = -(p_right + p_left) * reciprocal_width;
    result[3][1] = -(p_top + p_bottom) * reciprocal_height;
    result[3][2] = -(p_far + p_near) * reciprocal_depth;
    return result;
}

inline std::array<Matrix4x4f, 6> BuildPointLightCubeMapViewProjectionMatrix(const Vector3f& p_eye, float p_near, float p_far) {
    auto P = BuildPerspectiveLH(glm::radians(90.0f), 1.0f, p_near, p_far);

    std::array<Matrix4x4f, 6> matrices = {
        P * glm::lookAtLH(p_eye, p_eye + Vector3f(+1, +0, +0), Vector3f(0, +1, +0)),
        P * glm::lookAtLH(p_eye, p_eye + Vector3f(-1, +0, +0), Vector3f(0, +1, +0)),
        P * glm::lookAtLH(p_eye, p_eye + Vector3f(+0, +1, +0), Vector3f(0, +0, -1)),
        P * glm::lookAtLH(p_eye, p_eye + Vector3f(+0, -1, +0), Vector3f(0, +0, +1)),
        P * glm::lookAtLH(p_eye, p_eye + Vector3f(+0, +0, +1), Vector3f(0, +1, +0)),
        P * glm::lookAtLH(p_eye, p_eye + Vector3f(+0, +0, -1), Vector3f(0, +1, +0)),
    };

    return matrices;
}

inline std::array<Matrix4x4f, 6> BuildOpenGlPointLightCubeMapViewProjectionMatrix(const Vector3f& p_eye, float p_near, float p_far) {
    auto P = BuildOpenGlPerspectiveRH(glm::radians(90.0f), 1.0f, p_near, p_far);

    std::array<Matrix4x4f, 6> matrices = {
        P * glm::lookAtRH(p_eye, p_eye + Vector3f(+1, +0, +0), Vector3f(0, -1, +0)),
        P * glm::lookAtRH(p_eye, p_eye + Vector3f(-1, +0, +0), Vector3f(0, -1, +0)),
        P * glm::lookAtRH(p_eye, p_eye + Vector3f(+0, +1, +0), Vector3f(0, +0, +1)),
        P * glm::lookAtRH(p_eye, p_eye + Vector3f(+0, -1, +0), Vector3f(0, +0, -1)),
        P * glm::lookAtRH(p_eye, p_eye + Vector3f(+0, +0, +1), Vector3f(0, -1, +0)),
        P * glm::lookAtRH(p_eye, p_eye + Vector3f(+0, +0, -1), Vector3f(0, -1, +0)),
    };

    return matrices;
}

inline std::array<Matrix4x4f, 6> BuildCubeMapViewProjectionMatrix(const Vector3f& p_eye) {
    auto P = BuildPerspectiveRH(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);

    std::array<Matrix4x4f, 6> matrices = {
        P * glm::lookAt(p_eye, p_eye + Vector3f(+1, +0, +0), Vector3f(0, -1, +0)),
        P * glm::lookAt(p_eye, p_eye + Vector3f(-1, +0, +0), Vector3f(0, -1, +0)),
        P * glm::lookAt(p_eye, p_eye + Vector3f(+0, -1, +0), Vector3f(0, +0, -1)),
        P * glm::lookAt(p_eye, p_eye + Vector3f(+0, +1, +0), Vector3f(0, +0, +1)),
        P * glm::lookAt(p_eye, p_eye + Vector3f(+0, +0, +1), Vector3f(0, -1, +0)),
        P * glm::lookAt(p_eye, p_eye + Vector3f(+0, +0, -1), Vector3f(0, -1, +0)),
    };

    return matrices;
}

inline std::array<Matrix4x4f, 6> BuildOpenGlCubeMapViewProjectionMatrix(const Vector3f& p_eye) {
    auto P = BuildOpenGlPerspectiveRH(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);

    std::array<Matrix4x4f, 6> matrices = {
        P * glm::lookAtRH(p_eye, p_eye + Vector3f(+1, +0, +0), Vector3f(0, -1, +0)),
        P * glm::lookAtRH(p_eye, p_eye + Vector3f(-1, +0, +0), Vector3f(0, -1, +0)),
        P * glm::lookAtRH(p_eye, p_eye + Vector3f(+0, +1, +0), Vector3f(0, +0, +1)),
        P * glm::lookAtRH(p_eye, p_eye + Vector3f(+0, -1, +0), Vector3f(0, +0, -1)),
        P * glm::lookAtRH(p_eye, p_eye + Vector3f(+0, +0, +1), Vector3f(0, -1, +0)),
        P * glm::lookAtRH(p_eye, p_eye + Vector3f(+0, +0, -1), Vector3f(0, -1, +0)),
    };

    return matrices;
}

#if 0
inline std::array<Matrix4x4f, 6> BuildCubeMapViewMatrices(const Vector3f& p_eye) {
    std::array<Matrix4x4f, 6> matrices;

#if 0
    matrices[0] = glm::lookAtLH(p_eye, p_eye + Vector3f(+1, +0, +0), Vector3f(0, +1, +0));
    matrices[1] = glm::lookAtLH(p_eye, p_eye + Vector3f(-1, +0, +0), Vector3f(0, +1, +0));
    matrices[2] = glm::lookAtLH(p_eye, p_eye + Vector3f(+0, +1, +0), Vector3f(0, +0, -1));
    matrices[3] = glm::lookAtLH(p_eye, p_eye + Vector3f(+0, -1, +0), Vector3f(0, +0, +1));
    matrices[4] = glm::lookAtLH(p_eye, p_eye + Vector3f(+0, +0, +1), Vector3f(0, +1, +0));
    matrices[5] = glm::lookAtLH(p_eye, p_eye + Vector3f(+0, +0, -1), Vector3f(0, +1, +0));
#else
    matrices[0] = glm::lookAt(p_eye, p_eye + Vector3f(+1, +0, +0), Vector3f(0, -1, +0));
    matrices[1] = glm::lookAt(p_eye, p_eye + Vector3f(-1, +0, +0), Vector3f(0, -1, +0));
    matrices[2] = glm::lookAt(p_eye, p_eye + Vector3f(+0, -1, +0), Vector3f(0, +0, -1));
    matrices[3] = glm::lookAt(p_eye, p_eye + Vector3f(+0, +1, +0), Vector3f(0, +0, +1));
    matrices[4] = glm::lookAt(p_eye, p_eye + Vector3f(+0, +0, +1), Vector3f(0, -1, +0));
    matrices[5] = glm::lookAt(p_eye, p_eye + Vector3f(+0, +0, -1), Vector3f(0, -1, +0));
#endif
    return matrices;
}

inline std::array<Matrix4x4f, 6> BuildOpenGlCubeMapViewMatrices(const Vector3f& p_eye) {
    std::array<Matrix4x4f, 6> matrices;
    matrices[0] = glm::lookAtRH(p_eye, p_eye + Vector3f(+1, +0, +0), Vector3f(0, -1, +0));
    matrices[1] = glm::lookAtRH(p_eye, p_eye + Vector3f(-1, +0, +0), Vector3f(0, -1, +0));
    matrices[2] = glm::lookAtRH(p_eye, p_eye + Vector3f(+0, +1, +0), Vector3f(0, +0, +1));
    matrices[3] = glm::lookAtRH(p_eye, p_eye + Vector3f(+0, -1, +0), Vector3f(0, +0, -1));
    matrices[4] = glm::lookAtRH(p_eye, p_eye + Vector3f(+0, +0, +1), Vector3f(0, -1, +0));
    matrices[5] = glm::lookAtRH(p_eye, p_eye + Vector3f(+0, +0, -1), Vector3f(0, -1, +0));
    return matrices;
}
#endif

}  // namespace my