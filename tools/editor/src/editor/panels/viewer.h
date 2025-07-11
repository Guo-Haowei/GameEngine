#pragma once
#include "editor/editor_window.h"
#include "engine/core/input/input_router.h"
#include "engine/scene/camera_controller.h"

namespace my {

class Viewer : public EditorWindow, public IInputHandler {
public:
    Viewer(EditorLayer& p_editor);

    bool HandleInput(std::shared_ptr<InputEvent> p_input_event) override;

protected:
    void UpdateInternal(Scene& p_scene) override;

private:
    void UpdateData();
    void SelectEntity(Scene& p_scene, const CameraComponent& p_camera);
    void DrawToolBar();
    void DrawGui(Scene& p_scene, CameraComponent& p_camera);

    Vector2f m_canvasMin;
    Vector2f m_canvasSize;
    bool m_focused;

    CameraControllerFPS m_cameraController3D;
    CameraController2DEditor m_cameraController2D;

    struct InputState {
        int dx, dy, dz;
        float scroll;
        Vector2f mouse_move;

        InputState() {
            Reset();
        }

        void Reset() {
            dx = dy = dz = 0;
            scroll = 0.0f;
            mouse_move = Vector2f{ 0, 0 };
        }
    } m_inputState;
};

}  // namespace my
