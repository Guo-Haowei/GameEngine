#include "camera.h"

#include "core/io/archive.h"
#include "rendering/rendering_dvars.h"

namespace my {

Camera::Camera() {
    ivec2 frame_size = DVAR_GET_IVEC2(resolution);
    m_width = frame_size.x;
    m_height = frame_size.y;
}

void Camera::update() {
    if (is_dirty()) {
        m_front.x = m_yaw.cos() * m_pitch.cos();
        m_front.y = m_pitch.sin();
        m_front.z = m_yaw.sin() * m_pitch.cos();

        m_right = glm::cross(m_front, vec3(0, 1, 0));

        // @TODO: use transpose
        m_view_matrix = glm::lookAt(m_position, m_position + m_front, vec3(0, 1, 0));

        m_projection_matrix = glm::perspective(m_fovy.to_rad(), get_aspect(), m_near, m_far);
        m_projection_view_matrix = m_projection_matrix * m_view_matrix;
        set_dirty(false);
    }
}

void Camera::set_dimension(int width, int height) {
    if (m_width != width || m_height != height) {
        m_width = width;
        m_height = height;
        set_dirty();
    }
}

void Camera::serialize(Archive& archive, uint32_t) {
    if (archive.is_write_mode()) {
        archive << m_flags;
        archive << m_near;
        archive << m_far;
        archive << m_fovy;
        archive << m_width;
        archive << m_height;
        archive << m_pitch;
        archive << m_yaw;
        archive << m_position;
    } else {
        archive >> m_flags;
        archive >> m_near;
        archive >> m_far;
        archive >> m_fovy;
        archive >> m_width;
        archive >> m_height;
        archive >> m_pitch;
        archive >> m_yaw;
        archive >> m_position;

        // @TODO: fix this
        ivec2 frame_size = DVAR_GET_IVEC2(resolution);
        m_width = frame_size.x;
        m_height = frame_size.y;
        set_dirty();
    }
}

}  // namespace my