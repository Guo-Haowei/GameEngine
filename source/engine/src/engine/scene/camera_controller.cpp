#include "camera_controller.h"

#include "core/input/input.h"

namespace vct {

void CameraController::move(float dt, CameraComponent& camera, TransformComponent& transform) {
    // @TODO: smooth movement
    float move_speed = 10 * dt;
    float rotate_speed = 150 * dt;

    int dx = input::is_key_down(KEY_D) - input::is_key_down(KEY_A);
    int dy = input::is_key_down(KEY_E) - input::is_key_down(KEY_Q);
    int dz = input::is_key_down(KEY_W) - input::is_key_down(KEY_S);
    bool dirty = false;

    if (dx || dz) {
        vec3 delta = (move_speed * dz) * camera.get_front() + (move_speed * dx) * camera.get_right();
        transform.increase_translation(delta);
        dirty = true;
    }
    if (dy) {
        transform.increase_translation(vec3(0, move_speed * dy, 0));
        dirty = true;
    }

    const mat4& local_matrix = transform.get_local_matrix();
    if (!m_initialized) {
        m_pitch = Radians{ std::asin(-local_matrix[1][0]) };                     // y-axis
        m_roll = Radians{ std::atan2(local_matrix[1][2], local_matrix[2][2]) };  // x-axis
        m_initialized = true;
    }

    if (input::is_button_down(MOUSE_BUTTON_MIDDLE)) {
    }

    int rx = input::is_key_down(KEY_UP) - input::is_key_down(KEY_DOWN);
    int ry = input::is_key_down(KEY_LEFT) - input::is_key_down(KEY_RIGHT);

    if (ry) {
        m_pitch += Degree(ry * rotate_speed);
        dirty = true;
    }
    if (rx) {
        Degree roll{ m_roll.to_degree() + rx * rotate_speed };
        m_roll.clamp(-85.0f, 85.0f);

        m_roll = roll;
        dirty = true;
    }

    if (rx || ry) {
        glm::mat4 rotation_roll = glm::rotate(glm::mat4(1.0f), m_roll.get_rad(), glm::vec3(1.0f, 0.0f, 0.0f));
        glm::mat4 rotation_pitch = glm::rotate(glm::mat4(1.0f), m_pitch.get_rad(), glm::vec3(0.0f, 1.0f, 0.0f));

        glm::mat4 rotation = rotation_pitch * rotation_roll;
        quat quat = glm::quat_cast(rotation);

        transform.set_rotation(vec4{ quat.x, quat.y, quat.z, quat.w });
    }

    if (dirty) {
        camera.set_dirty();
    }
}

}  // namespace vct
