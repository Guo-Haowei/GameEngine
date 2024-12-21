#include "camera_controller.h"

#include "engine/core/framework/input_manager.h"
#include "engine/math/angle.h"
#include "engine/scene/scene_component.h"

namespace my {

void EditorCameraController::OnCreate() {
}

void EditorCameraController::OnUpdate(float p_timestep) {
    PerspectiveCameraComponent* camera = GetComponent<PerspectiveCameraComponent>();

    if (DEV_VERIFY(camera)) {
        InputManager& input_manager = InputManager::GetSingleton();
        const int dx = input_manager.IsKeyDown(KeyCode::KEY_D) -
                       input_manager.IsKeyDown(KeyCode::KEY_A);
        const int dy = input_manager.IsKeyDown(KeyCode::KEY_E) -
                       input_manager.IsKeyDown(KeyCode::KEY_Q);
        const int dz = input_manager.IsKeyDown(KeyCode::KEY_W) -
                       input_manager.IsKeyDown(KeyCode::KEY_S);

        Vector3i delta(dx, dy, dz);
        Vector2f rotation(0);
        if (input_manager.IsButtonDown(MouseButton::MIDDLE)) {
            rotation = input_manager.MouseMove();
        }

        Context context{
            .timestep = p_timestep,
            .scroll = input_manager.GetWheel().y,
            .camera = camera,
            .move = delta,
            .rotation = rotation,
        };

        Move(context);
    }
}

void EditorCameraController::Move(const Context& p_context) {
    // @TODO: smooth movement
    // @TODO: get rid off the magic numbers
    const bool moved = p_context.move.x || p_context.move.y || p_context.move.z || p_context.scroll != 0.0f;
    const float timestep = p_context.timestep;
    auto& camera = p_context.camera;
    if (moved) {
        const float move_speed = m_moveSpeed * timestep;
        const float dx = (float)p_context.move.x;
        const float dy = (float)p_context.move.y;

        float dz = (float)p_context.move.z;
        const float scroll = m_scrollSpeed * p_context.scroll;
        if (glm::abs(scroll) > glm::abs(dz)) {
            dz = scroll;
        }
        if (dx || dz) {
            Vector3f delta = (move_speed * dz) * camera->GetFront() + (move_speed * dx) * camera->GetRight();
            camera->m_position += delta;
        }
        if (dy) {
            camera->m_position += Vector3f(0, move_speed * dy, 0);
        }
    }

    auto rotate_camera = [&]() {
        float rotate_x = 0.0f;
        float rotate_y = 0.0f;

        Vector2f movement = p_context.rotation;
        movement = m_rotateSpeed * timestep * movement;
        if (glm::abs(movement.x) > glm::abs(movement.y)) {
            rotate_y = movement.x;
        } else {
            rotate_x = movement.y;
        }

        // @TODO: DPI
        if (rotate_y) {
            camera->m_yaw += Degree(rotate_y);
        }

        if (rotate_x) {
            camera->m_pitch += Degree(rotate_x);
            camera->m_pitch.Clamp(-85.0f, 85.0f);
        }

        return rotate_x != 0.0f || rotate_y != 0.0f;
    };

    if (moved || rotate_camera()) {
        camera->SetDirty();
    }
}

}  // namespace my
