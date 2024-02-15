#include "camera_controller.h"

#include "core/input/input.h"

namespace vct {

void CameraController::move(float dt, CameraComponent& camera, TransformComponent& transform) {
    int dx = input::is_key_down(KEY_D) - input::is_key_down(KEY_A);
    int dy = input::is_key_down(KEY_E) - input::is_key_down(KEY_Q);
    int dz = input::is_key_down(KEY_W) - input::is_key_down(KEY_S);

    float move_speed = 15 * dt;
    if (dx || dz) {
        vec3 delta = (move_speed * dz) * camera.get_front() + (move_speed * dx) * camera.get_right();
        transform.increase_translation(delta);
        camera.set_dirty();
    }
    if (dy) {
        transform.increase_translation(vec3(0, move_speed * dy, 0));
        camera.set_dirty();
    }

    // rotate
    // if (input::is_button_down(MOUSE_BUTTON_MIDDLE)) {
    //    const float rotateSpeed = 20.0f * dt;
    //    vec2 p = input::mouse_move();
    //    bool dirty = false;
    //    if (p.x != 0.0f) {
    //        m_angle_x -= Degree(rotateSpeed * p.x);
    //        dirty = true;
    //    }
    //    if (p.y != 0.0f) {
    //        m_angle_xz += Degree(rotateSpeed * p.y);
    //        m_angle_xz.clamp(-80.0f, 80.0f);
    //        dirty = true;
    //    }

    //    if (dirty) {
    //        m_camera->set_eye(calculate_eye(m_camera->get_center()));
    //    }
    //    return;
    //}

    //// pan
    // if (input::is_button_down(MOUSE_BUTTON_RIGHT)) {
    //     vec2 p = input::mouse_move();
    //     if (glm::abs(p.x) >= 1.0f || glm::abs(p.y) >= 1.0f) {
    //         const float panSpeed = 10.0f * dt;

    //        mat4 model = glm::inverse(m_camera->get_view_matrix());
    //        vec3 offset = glm::normalize(vec3(p.x, p.y, 0.0f));
    //        vec3 new_center = m_camera->get_center();
    //        new_center -= panSpeed * vec3(model * vec4(offset, 0.0f));
    //        m_camera->set_center(new_center);
    //        m_camera->set_eye(calculate_eye(m_camera->get_center()));
    //        return;
    //    }
    //}

    //// scroll
    //// @TODO: push point forward
    // const float accel = 100.0f;
    // float scrolling = input::get_wheel().y;
    // if (scrolling != 0.0f) {
    //     m_scroll_speed += dt * accel;
    //     m_scroll_speed = glm::min(m_scroll_speed, MAX_SCROLL_SPEED);
    // } else {
    //     m_scroll_speed -= 10.0f;
    //     m_scroll_speed = glm::max(m_scroll_speed, 0.0f);
    // }

    // if (m_scroll_speed != 0.0f) {
    //     m_distance -= m_scroll_speed * scrolling;
    //     m_distance = glm::clamp(m_distance, 0.3f, 10000.0f);
    //     m_camera->set_eye(calculate_eye(m_camera->get_center()));
    // }
}

}  // namespace vct
