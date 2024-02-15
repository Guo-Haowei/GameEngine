#pragma once
#include "panel.h"
#include "scene/camera_controller.h"

namespace my {

class Viewer : public Panel {
public:
    Viewer(EditorLayer& editor, const Scene& scene);

protected:
    void update_internal(Scene& scene) override;

private:
    void update_data();
    void update_camera(float dt, CameraComponent& camera, TransformComponent& transform);
    void select_entity(Scene& scene, const CameraComponent& camera);
    void draw_gui(Scene& scene, CameraComponent& camera);

    vec2 m_canvas_min;
    vec2 m_canvas_size;
    bool m_focused;
    // @TODO: move camera controller to somewhere else
    CameraController m_camera_controller;
};

}  // namespace my
