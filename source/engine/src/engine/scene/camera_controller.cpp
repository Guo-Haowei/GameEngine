#include "camera_controller.h"

#include "core/input/input.h"

namespace vct {

void CameraController::move(float dt, CameraComponent& camera, TransformComponent& transform) {
    // @TODO: smooth movement
    const mat4& local_matrix = transform.get_local_matrix();
    if (!m_initialized) {
        m_pitch = Radians{ std::asin(-local_matrix[1][0]) };                     // y-axis
        m_roll = Radians{ std::atan2(local_matrix[1][2], local_matrix[2][2]) };  // x-axis
        m_initialized = true;
    }

    // @TODO: get rid off the magic numbers
    auto translate_camera = [&]() {
        float move_speed = 10 * dt;
        float dx = (float)(input::is_key_down(KEY_D) - input::is_key_down(KEY_A));
        float dy = (float)(input::is_key_down(KEY_E) - input::is_key_down(KEY_Q));
        float dz = (float)(input::is_key_down(KEY_W) - input::is_key_down(KEY_S));

        float scroll = input::get_wheel().y * 3.0f;
        if (glm::abs(scroll) > glm::abs(dz)) {
            dz = scroll;
        }
        if (dx || dz) {
            vec3 delta = (move_speed * dz) * camera.get_front() + (move_speed * dx) * camera.get_right();
            transform.increase_translation(delta);
        }
        if (dy) {
            transform.increase_translation(vec3(0, move_speed * dy, 0));
        }
        return dx || dy || dz;
    };

    auto rotate_camera = [&]() {
        float delta_roll = 0.0f;
        float delta_pitch = 0.0f;

        if (input::is_button_down(MOUSE_BUTTON_MIDDLE)) {
            vec2 movement = input::mouse_move();
            movement = 20 * dt * movement;
            if (glm::abs(movement.x) > glm::abs(movement.y)) {
                delta_pitch = movement.x;
            } else {
                delta_roll = movement.y;
            }
        } else {
            // keyboard
            float speed = 200 * dt;
            delta_pitch = speed * -(input::is_key_down(KEY_RIGHT) - input::is_key_down(KEY_LEFT));
            delta_roll = speed * (input::is_key_down(KEY_UP) - input::is_key_down(KEY_DOWN));
        }

        // @TODO: DPI

        if (delta_pitch) {
            m_pitch += Degree(delta_pitch);
        }
        if (delta_roll) {
            Degree roll{ m_roll.to_degree() + delta_roll };
            m_roll.clamp(-85.0f, 85.0f);

            m_roll = roll;
        }

        if (delta_roll || delta_pitch) {
            glm::mat4 rotation_roll = glm::rotate(glm::mat4(1.0f), m_roll.get_rad(), glm::vec3(1.0f, 0.0f, 0.0f));
            glm::mat4 rotation_pitch = glm::rotate(glm::mat4(1.0f), m_pitch.get_rad(), glm::vec3(0.0f, 1.0f, 0.0f));

            glm::mat4 rotation = rotation_pitch * rotation_roll;
            quat quat = glm::quat_cast(rotation);

            transform.set_rotation(vec4{ quat.x, quat.y, quat.z, quat.w });
            return true;
        }

        return false;
    };

    bool dirty = false;
    dirty = dirty || translate_camera();
    dirty = dirty || rotate_camera();

    if (dirty) {
        camera.set_dirty();
    }
}

}  // namespace vct
