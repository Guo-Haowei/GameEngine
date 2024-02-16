#include "camera_controller.h"

#include "core/input/input.h"

namespace my {

void CameraController::move(float dt, Camera& camera) {
    // @TODO: smooth movement
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
            camera.m_position += delta;
        }
        if (dy) {
            camera.m_position += vec3(0, move_speed * dy, 0);
        }
        return dx || dy || dz;
    };

    auto rotate_camera = [&]() {
        float rotate_x = 0.0f;
        float rotate_y = 0.0f;

        if (input::is_button_down(MOUSE_BUTTON_MIDDLE)) {
            vec2 movement = input::mouse_move();
            movement = 20 * dt * movement;
            if (glm::abs(movement.x) > glm::abs(movement.y)) {
                rotate_y = movement.x;
            } else {
                rotate_x = movement.y;
            }
        } else {
            // keyboard
            float speed = 200 * dt;
            rotate_y = speed * (input::is_key_down(KEY_RIGHT) - input::is_key_down(KEY_LEFT));
            rotate_x = speed * (input::is_key_down(KEY_UP) - input::is_key_down(KEY_DOWN));
        }

        // @TODO: DPI

        if (rotate_y) {
            camera.m_yaw += Degree(rotate_y);
        }

        if (rotate_x) {
            camera.m_pitch += Degree(rotate_x);
            camera.m_pitch.clamp(-85.0f, 85.0f);
        }

        return rotate_x || rotate_y;
    };

    bool dirty = translate_camera() | rotate_camera();
    if (dirty) {
        camera.set_dirty();
    }
}

}  // namespace my
