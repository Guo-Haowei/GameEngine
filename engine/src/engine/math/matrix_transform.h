#pragma once
#include "angle.h"
#include "geomath.h"
#include "vector.h"

// @TODO: refactor
namespace my {

Matrix4x4f LookAtRh(const Vector3f& p_eye, const Vector3f& p_center, const Vector3f& p_up);

Matrix4x4f LookAtLh(const Vector3f& p_eye, const Vector3f& p_center, const Vector3f& p_up);

Matrix4x4f BuildPerspectiveLH(float p_fovy, float p_aspect, float p_near, float p_far);

Matrix4x4f BuildPerspectiveRH(float p_fovy, float p_aspect, float p_near, float p_far);

Matrix4x4f BuildOpenGlPerspectiveRH(float p_fovy, float p_aspect, float p_near, float p_far);

Matrix4x4f BuildOrthoRH(const float p_left,
                        const float p_right,
                        const float p_bottom,
                        const float p_top,
                        const float p_near,
                        const float p_far);

Matrix4x4f BuildOpenGlOrthoRH(const float p_left,
                              const float p_right,
                              const float p_bottom,
                              const float p_top,
                              const float p_near,
                              const float p_far);

std::array<Matrix4x4f, 6> BuildPointLightCubeMapViewProjectionMatrix(const Vector3f& p_eye, float p_near, float p_far);

std::array<Matrix4x4f, 6> BuildOpenGlPointLightCubeMapViewProjectionMatrix(const Vector3f& p_eye, float p_near, float p_far);

std::array<Matrix4x4f, 6> BuildCubeMapViewProjectionMatrix(const Vector3f& p_eye);

std::array<Matrix4x4f, 6> BuildOpenGlCubeMapViewProjectionMatrix(const Vector3f& p_eye);

static inline Matrix4x4f Translate(const Vector3f& p_vec) {
    return glm::translate(glm::vec3(p_vec.x, p_vec.y, p_vec.z));
}

static inline Matrix4x4f Scale(const Vector3f& p_vec) {
    return glm::scale(glm::vec3(p_vec.x, p_vec.y, p_vec.z));
}

static inline Matrix4x4f Rotate(const Degree& p_degree, const Vector3f& p_axis) {
    return glm::rotate(p_degree.GetRadians(), glm::vec3(p_axis.x, p_axis.y, p_axis.z));
}

static inline Matrix4x4f Rotate(const Radian& p_radians, const Vector3f& p_axis) {
    return glm::rotate(p_radians.GetRad(), glm::vec3(p_axis.x, p_axis.y, p_axis.z));
}

}  // namespace my