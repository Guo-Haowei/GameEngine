#include "frustum.h"

#include "aabb.h"

namespace my::math {

// https://stackoverflow.com/questions/12836967/extracting-view-frustum-planes-hartmann-gribbs-method
Frustum::Frustum(const Matrix4x4f& p_project_view_matrix) {
    m_left.normal.x = p_project_view_matrix[0][3] + p_project_view_matrix[0][0];
    m_left.normal.y = p_project_view_matrix[1][3] + p_project_view_matrix[1][0];
    m_left.normal.z = p_project_view_matrix[2][3] + p_project_view_matrix[2][0];
    m_left.dist = p_project_view_matrix[3][3] + p_project_view_matrix[3][0];

    m_right.normal.x = p_project_view_matrix[0][3] - p_project_view_matrix[0][0];
    m_right.normal.y = p_project_view_matrix[1][3] - p_project_view_matrix[1][0];
    m_right.normal.z = p_project_view_matrix[2][3] - p_project_view_matrix[2][0];
    m_right.dist = p_project_view_matrix[3][3] - p_project_view_matrix[3][0];

    m_top.normal.x = p_project_view_matrix[0][3] - p_project_view_matrix[0][1];
    m_top.normal.y = p_project_view_matrix[1][3] - p_project_view_matrix[1][1];
    m_top.normal.z = p_project_view_matrix[2][3] - p_project_view_matrix[2][1];
    m_top.dist = p_project_view_matrix[3][3] - p_project_view_matrix[3][1];

    m_bottom.normal.x = p_project_view_matrix[0][3] + p_project_view_matrix[0][1];
    m_bottom.normal.y = p_project_view_matrix[1][3] + p_project_view_matrix[1][1];
    m_bottom.normal.z = p_project_view_matrix[2][3] + p_project_view_matrix[2][1];
    m_bottom.dist = p_project_view_matrix[3][3] + p_project_view_matrix[3][1];

    m_near.normal.x = p_project_view_matrix[0][3] + p_project_view_matrix[0][2];
    m_near.normal.y = p_project_view_matrix[1][3] + p_project_view_matrix[1][2];
    m_near.normal.z = p_project_view_matrix[2][3] + p_project_view_matrix[2][2];
    m_near.dist = p_project_view_matrix[3][3] + p_project_view_matrix[3][2];

    m_far.normal.x = p_project_view_matrix[0][3] - p_project_view_matrix[0][2];
    m_far.normal.y = p_project_view_matrix[1][3] - p_project_view_matrix[1][2];
    m_far.normal.z = p_project_view_matrix[2][3] - p_project_view_matrix[2][2];
    m_far.dist = p_project_view_matrix[3][3] - p_project_view_matrix[3][2];
}

bool Frustum::Intersects(const AABB& p_box) const {
    const auto& box_min = p_box.GetMin();
    const auto& box_max = p_box.GetMax();
    for (int i = 0; i < 6; ++i) {
        const Plane& plane = this->operator[](i);
        Vector3f p;
        p.x = plane.normal.x > 0.0f ? box_max.x : box_min.x;
        p.y = plane.normal.y > 0.0f ? box_max.y : box_min.y;
        p.z = plane.normal.z > 0.0f ? box_max.z : box_min.z;

        if (plane.Distance(p) < 0.0f) {
            return false;
        }
    }

    return true;
}

}  // namespace my::math
