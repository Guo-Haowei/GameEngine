#include "camera_controller.h"

#include "engine/core/framework/input_manager.h"
#include "engine/core/math/angle.h"
#include "engine/scene/scene_component.h"

namespace my {

void CameraController::Move(float p_delta_time, PerspectiveCameraComponent& p_camera, Vector3i& p_move, float p_scroll) {
    // @TODO: smooth movement
    // @TODO: get rid off the magic numbers
    const bool moved = p_move.x || p_move.y || p_move.z || p_scroll != 0.0f;
    if (moved) {
        float move_speed = 10 * p_delta_time;
        float dx = (float)p_move.x;
        float dy = (float)p_move.y;
        float dz = (float)p_move.z;

        if (glm::abs(p_scroll) > glm::abs(dz)) {
            dz = p_scroll;
        }
        if (dx || dz) {
            Vector3f delta = (move_speed * dz) * p_camera.GetFront() + (move_speed * dx) * p_camera.GetRight();
            p_camera.m_position += delta;
        }
        if (dy) {
            p_camera.m_position += Vector3f(0, move_speed * dy, 0);
        }
    }

    auto rotate_camera = [&]() {
        float rotate_x = 0.0f;
        float rotate_y = 0.0f;

        if (InputManager::GetSingleton().IsButtonDown(MOUSE_BUTTON_MIDDLE)) {
            Vector2f movement = InputManager::GetSingleton().MouseMove();
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

    if (moved || rotate_camera()) {
        p_camera.SetDirty();
    }
}

}  // namespace my
