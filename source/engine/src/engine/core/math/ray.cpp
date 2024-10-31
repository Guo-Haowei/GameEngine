#include "ray.h"

namespace my {

Ray Ray::Inverse(const mat4& p_inverse_matrix) const {
    vec3 inversed_start = vec3(p_inverse_matrix * vec4(m_start, 1.0f));
    vec3 inversed_end = vec3(p_inverse_matrix * vec4(m_end, 1.0f));
    Ray inversed_ray(inversed_start, inversed_end);
    inversed_ray.m_dist = m_dist;
    return inversed_ray;
}

}  // namespace my
