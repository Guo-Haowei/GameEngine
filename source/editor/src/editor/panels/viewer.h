#pragma once
#include "editor/editor_window.h"
#include "scene/camera_controller.h"

namespace my {

class Viewer : public EditorWindow {
public:
    Viewer(EditorLayer& editor);

protected:
    void update_internal(Scene& scene) override;

private:
    void update_data();
    void select_entity(Scene& scene, const Camera& camera);
    void draw_gui(Scene& scene, Camera& camera);
    void update_display_image();

    vec2 m_canvas_min;
    vec2 m_canvas_size;
    bool m_focused;
    // @TODO: move camera controller to somewhere else
    CameraController m_camera_controller;
};

}  // namespace my
