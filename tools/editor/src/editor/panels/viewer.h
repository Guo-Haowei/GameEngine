#pragma once
#include "editor/editor_window.h"
#include "engine/scene/camera_controller.h"

namespace my {

class Viewer : public EditorWindow {
public:
    Viewer(EditorLayer& p_editor) : EditorWindow("Viewer", p_editor) {}

protected:
    void UpdateInternal(Scene& p_scene) override;

private:
    void DrawToolBar();
    void UpdateData();
    void SelectEntity(Scene& p_scene, const CameraComponent& p_camera);
    void DrawGui(Scene& p_scene, CameraComponent& p_camera);

    Vector2f m_canvasMin;
    Vector2f m_canvasSize;
    bool m_focused;

    CameraControllerFPS m_cameraController3D;
    CameraController2DEditor m_cameraController2D;
};

}  // namespace my
