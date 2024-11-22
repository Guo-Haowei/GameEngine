#include "camera_controller.h"

#include "core/framework/input_manager.h"

namespace my {

void CameraController::Move(float p_delta_time, Camera& p_camera) {
    // @TODO: smooth movement
    // @TODO: get rid off the magic numbers
    auto translate_camera = [&]() {
        float move_speed = 10 * p_delta_time;
        float dx = (float)(InputManager::GetSingleton().IsKeyDown(KeyCode::KEY_D) - InputManager::GetSingleton().IsKeyDown(KeyCode::KEY_A));
        float dy = (float)(InputManager::GetSingleton().IsKeyDown(KeyCode::KEY_E) - InputManager::GetSingleton().IsKeyDown(KeyCode::KEY_Q));
        float dz = (float)(InputManager::GetSingleton().IsKeyDown(KeyCode::KEY_W) - InputManager::GetSingleton().IsKeyDown(KeyCode::KEY_S));

        float scroll = InputManager::GetSingleton().GetWheel().y * 3.0f;
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

        if (InputManager::GetSingleton().IsButtonDown(MOUSE_BUTTON_MIDDLE)) {
            vec2 movement = InputManager::GetSingleton().MouseMove();
            movement = 20 * p_delta_time * movement;
            if (glm::abs(movement.x) > glm::abs(movement.y)) {
                rotate_y = movement.x;
            } else {
                rotate_x = movement.y;
            }
        } else {
            // keyboard
            float speed = 200 * p_delta_time;
            rotate_y = speed * (InputManager::GetSingleton().IsKeyDown(KeyCode::KEY_RIGHT) - InputManager::GetSingleton().IsKeyDown(KeyCode::KEY_LEFT));
            rotate_x = speed * (InputManager::GetSingleton().IsKeyDown(KeyCode::KEY_UP) - InputManager::GetSingleton().IsKeyDown(KeyCode::KEY_DOWN));
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

    if (translate_camera() || rotate_camera()) {
        p_camera.SetDirty();
    }
}

}  // namespace my
