#include "aabb.h"

#include "detail/matrix.h"

namespace my::math {

#if 0
 *        E__________________ H
 *       /|                 /|
 *      / |                / |
 *     /  |               /  |
 *   A/___|______________/D  |
 *    |   |              |   |
 *    |   |              |   |
 *    |   |              |   |
 *    |  F|______________|___|G
 *    |  /               |  /
 *    | /                | /
 *   B|/_________________|C
 *
 * A, B, B, C, C, D, D, A, E, F, F, G, G, H, H, E, A, E, B, F, D, H, C, G
#endif
#if 0
Vector3f AABB::Corner(int p_index) const {
    // clang-format off
    switch (p_index)
    {
        case 0: return Vector3f(m_min.x, m_max.y, m_max.z); // A
        case 1: return Vector3f(m_min.x, m_min.y, m_max.z); // B
        case 2: return Vector3f(m_max.x, m_min.y, m_max.z); // C
        case 3: return Vector3f(m_max.x, m_max.y, m_max.z); // D
        case 4: return Vector3f(m_min.x, m_max.y, m_min.z); // E
        case 5: return Vector3f(m_min.x, m_min.y, m_min.z); // F
        case 6: return Vector3f(m_max.x, m_min.y, m_min.z); // G
        case 7: return Vector3f(m_max.x, m_max.y, m_min.z); // H
    }
    // clang-format on
    DEV_ASSERT(0);
    return Vector3f(0);
}
#endif

void AABB::ApplyMatrix(const Matrix4x4f& p_mat4) {
    const Vector4f points[] = { Vector4f(m_min.x, m_min.y, m_min.z, 1.0f), Vector4f(m_min.x, m_min.y, m_max.z, 1.0f),
                                Vector4f(m_min.x, m_max.y, m_min.z, 1.0f), Vector4f(m_min.x, m_max.y, m_max.z, 1.0f),
                                Vector4f(m_max.x, m_min.y, m_min.z, 1.0f), Vector4f(m_max.x, m_min.y, m_max.z, 1.0f),
                                Vector4f(m_max.x, m_max.y, m_min.z, 1.0f), Vector4f(m_max.x, m_max.y, m_max.z, 1.0f) };
    static_assert(array_length(points) == 8);

    AABB new_box;
    for (size_t i = 0; i < array_length(points); ++i) {
        auto point = p_mat4 * points[i];
        new_box.ExpandPoint(Vector3f(point.x, point.y, point.z));
    }

    m_min = new_box.m_min;
    m_max = new_box.m_max;
}

AABB AABB::FromCenterSize(const Vector3f& p_center, const Vector3f& p_size) {
    AABB box;
    Vector3f center;
    Vector3f half_size;
    center.Set(&p_center.x);
    half_size.Set(&p_size.x);
    half_size *= 0.5f;

    box.m_min = center - half_size;
    box.m_max = center + half_size;
    return box;
}

}  // namespace my::math
