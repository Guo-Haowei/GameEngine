#include "ray.h"

namespace my {

Ray Ray::Inverse(const Matrix4x4f& p_inverse_matrix) const {
    Vector3f inversed_start = Vector3f(p_inverse_matrix * Vector4f(m_start, 1.0f));
    Vector3f inversed_end = Vector3f(p_inverse_matrix * Vector4f(m_end, 1.0f));
    Ray inversed_ray(inversed_start, inversed_end);
    inversed_ray.m_dist = m_dist;
    return inversed_ray;
}

}  // namespace my
