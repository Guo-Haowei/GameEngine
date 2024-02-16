#pragma once
#include "panel.h"
#include "scene/camera_controller.h"

namespace my {

class Viewer : public Panel {
public:
    Viewer(EditorLayer& editor);

protected:
    void update_internal(Scene& scene) override;

private:
    void update_data();
    void select_entity(Scene& scene, const Camera& camera);
    void draw_gui(Scene& scene, Camera& camera);

    vec2 m_canvas_min;
    vec2 m_canvas_size;
    bool m_focused;
    // @TODO: move camera controller to somewhere else
    CameraController m_camera_controller;
};

}  // namespace my
