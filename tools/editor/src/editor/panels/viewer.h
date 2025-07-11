#pragma once
#include "editor/editor_window.h"
#include "engine/input/input_router.h"
#include "engine/scene/camera_controller.h"

namespace my {

enum class ViewerTool {
    GizmoEditing,
    TileMapPainting,
};

enum class GizmoState {
    Translating,
    Rotating,
    Scaling,
};

class Viewer : public EditorWindow, public IInputHandler {
public:
    Viewer(EditorLayer& p_editor);

    bool HandleInput(std::shared_ptr<InputEvent> p_input_event) override;

protected:
    void UpdateInternal(Scene& p_scene) override;

    void DrawToolBar();
    void DrawGui(Scene& p_scene, CameraComponent& p_camera);

    void UpdateData();
    void SelectEntity(Scene& p_scene, const CameraComponent& p_camera);
    bool HandleTileMapPainting(std::shared_ptr<InputEvent> p_input_event);
    bool HandleGizmoEditing(std::shared_ptr<InputEvent> p_input_event);
    bool HandleInputCamera(std::shared_ptr<InputEvent> p_input_event);

    Vector2f m_canvasMin;
    Vector2f m_canvasSize;
    bool m_focused;

    CameraControllerFPS m_cameraController3D;
    CameraController2DEditor m_cameraController2D;

    struct CameraInput {
        int dx, dy, dz;
        float scroll;
        Vector2f mouse_move;

        CameraInput() { Reset(); }

        void Reset() {
            dx = dy = dz = 0;
            scroll = 0.0f;
            mouse_move = Vector2f{ 0, 0 };
        }
    } m_cameraInput;
    struct GizmoInput {
        Vector2f clicked;

        GizmoInput() { Reset(); }

        void Reset() {
            clicked.x = clicked.y = -1.0f;
        }
    } m_gizmoInput;

    ViewerTool m_activeTool{ ViewerTool::GizmoEditing };
    GizmoState m_gizmoState{ GizmoState::Translating };
};

}  // namespace my
