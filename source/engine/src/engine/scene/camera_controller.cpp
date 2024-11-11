#include "camera_controller.h"

#include "core/input/input.h"

namespace my {

void CameraController::move(float p_delta_time, Camera& p_camera) {
    // @TODO: smooth movement
    // @TODO: get rid off the magic numbers
    auto translate_camera = [&]() {
        float move_speed = 10 * p_delta_time;
        float dx = (float)(input::IsKeyDown(KEY_D) - input::IsKeyDown(KEY_A));
        float dy = (float)(input::IsKeyDown(KEY_E) - input::IsKeyDown(KEY_Q));
        float dz = (float)(input::IsKeyDown(KEY_W) - input::IsKeyDown(KEY_S));

        float scroll = input::GetWheel().y * 3.0f;
        if (glm::abs(scroll) > glm::abs(dz)) {
            dz = scroll;
        }
        if (dx || dz) {
            vec3 delta = (move_speed * dz) * p_camera.GetFront() + (move_speed * dx) * p_camera.GetRight();
            p_camera.m_position += delta;
        }
        if (dy) {
            p_camera.m_position += vec3(0, move_speed * dy, 0);
        }
        return dx || dy || dz;
    };

    auto rotate_camera = [&]() {
        float rotate_x = 0.0f;
        float rotate_y = 0.0f;

        if (input::IsButtonDown(MOUSE_BUTTON_MIDDLE)) {
            vec2 movement = input::MouseMove();
            movement = 20 * p_delta_time * movement;
            if (glm::abs(movement.x) > glm::abs(movement.y)) {
                rotate_y = movement.x;
            } else {
                rotate_x = movement.y;
            }
        } else {
            // keyboard
            float speed = 200 * p_delta_time;
            rotate_y = speed * (input::IsKeyDown(KEY_RIGHT) - input::IsKeyDown(KEY_LEFT));
            rotate_x = speed * (input::IsKeyDown(KEY_UP) - input::IsKeyDown(KEY_DOWN));
        }

        // @TODO: DPI

        if (rotate_y) {
            p_camera.m_yaw += Degree(rotate_y);
        }

        if (rotate_x) {
            p_camera.m_pitch += Degree(rotate_x);
            p_camera.m_pitch.Clamp(-85.0f, 85.0f);
        }

        return rotate_x || rotate_y;
    };

    bool dirty = translate_camera() | rotate_camera();
    if (dirty) {
        p_camera.SetDirty();
    }
}

}  // namespace my
