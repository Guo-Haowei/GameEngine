#include "camera.h"

#include "engine/core/io/archive.h"
#include "engine/core/math/matrix_transform.h"

namespace my {

void Camera::Update() {
    if (IsDirty()) {
        m_front.x = m_yaw.Cos() * m_pitch.Cos();
        m_front.y = m_pitch.Sin();
        m_front.z = m_yaw.Sin() * m_pitch.Cos();

        m_right = glm::cross(m_front, Vector3f(0, 1, 0));

        m_viewMatrix = glm::lookAt(m_position, m_position + m_front, Vector3f(0, 1, 0));
        m_projectionMatrix = BuildOpenGlPerspectiveRH(m_fovy.GetRadians(), GetAspect(), m_near, m_far);
        m_projectionViewMatrix = m_projectionMatrix * m_viewMatrix;

        SetDirty(false);
    }
}

void Camera::SetDimension(int p_width, int p_height) {
    if (m_width != p_width || m_height != p_height) {
        m_width = p_width;
        m_height = p_height;
        SetDirty();
    }
}

void Camera::Serialize(Archive& p_archive, uint32_t) {
    if (p_archive.IsWriteMode()) {
        p_archive << m_flags;
        p_archive << m_near;
        p_archive << m_far;
        p_archive << m_fovy;
        p_archive << m_width;
        p_archive << m_height;
        p_archive << m_pitch;
        p_archive << m_yaw;
        p_archive << m_position;
    } else {
        p_archive >> m_flags;
        p_archive >> m_near;
        p_archive >> m_far;
        p_archive >> m_fovy;
        p_archive >> m_width;
        p_archive >> m_height;
        p_archive >> m_pitch;
        p_archive >> m_yaw;
        p_archive >> m_position;

        SetDirty();
    }
}

}  // namespace my