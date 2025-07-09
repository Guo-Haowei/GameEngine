#include "camera_controller.h"

#include "engine/math/angle.h"
#include "engine/runtime/input_manager.h"
#include "engine/scene/scene_component.h"

namespace my {

void CameraControllerFPS::Update(CameraComponent& p_camera,
                                 const CameraInputState& p_state) {

    const bool moved = p_state.move.x || p_state.move.y || p_state.move.z || p_state.zoomDelta != 0.0f;
    if (moved) {
        const float dx = (float)p_state.move.x;
        const float dy = (float)p_state.move.y;

        float dz = (float)p_state.move.z;
        const float scroll_z = m_scrollSpeed * p_state.zoomDelta;
        if (glm::abs(scroll_z) > glm::abs(dz)) {
            dz = scroll_z;
        }
        if (dx || dz) {
            Vector3f delta = (m_moveSpeed * dz) * p_camera.GetFront() + (m_moveSpeed * dx) * p_camera.GetRight();
            p_camera.m_position += delta;
        }
        if (dy) {
            p_camera.m_position += Vector3f(0, m_moveSpeed * dy, 0);
        }
    }

    auto rotate_camera = [&]() {
        float rotate_x = 0.0f;
        float rotate_y = 0.0f;

        Vector2f movement = p_state.rotation;
        movement = m_rotateSpeed * movement;
        if (glm::abs(movement.x) > glm::abs(movement.y)) {
            rotate_y = movement.x;
        } else {
            rotate_x = movement.y;
        }

        // @TODO: DPI
        if (rotate_y) {
            p_camera.m_yaw += Degree(rotate_y);
        }

        if (rotate_x) {
            p_camera.m_pitch += Degree(rotate_x);
            p_camera.m_pitch.Clamp(-85.0f, 85.0f);
        }

        return rotate_x != 0.0f || rotate_y != 0.0f;
    };

    if (moved || rotate_camera()) {
        p_camera.SetDirty();
    }
}

}  // namespace my
