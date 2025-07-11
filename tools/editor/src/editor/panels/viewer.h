#pragma once
#include "editor/editor_window.h"
#include "engine/input/input_router.h"
#include "engine/scene/camera_controller.h"

namespace my {

// @TODO: maybe move it to top level
enum class EditorToolType {
    Gizmo,
    TileMapEditor,
    Count,
};

class IEditorTool;

class Viewer : public EditorWindow, public IInputHandler {
public:
    Viewer(EditorLayer& p_editor);

    bool HandleInput(std::shared_ptr<InputEvent> p_input_event) override;

protected:
    void UpdateInternal(Scene& p_scene) override;

    std::optional<Vector2f> CursorToNDC(Vector2f p_point) const;

    void DrawToolBar();
    void DrawGui(Scene& p_scene, CameraComponent& p_camera);

    void UpdateData();
    bool HandleInputCamera(std::shared_ptr<InputEvent> p_input_event);

    IEditorTool* GetActiveTool() { return m_tools[std::to_underlying(m_active)].get(); }

    Vector2f m_canvasMin;
    Vector2f m_canvasSize;
    bool m_focused;

    CameraControllerFPS m_cameraController3D;
    CameraController2DEditor m_cameraController2D;

    struct InputState {
        int dx, dy, dz;
        float scroll;
        Vector2f mouse_move;
        std::optional<Vector2f> ndc;
        std::array<bool, std::to_underlying(MouseButton::COUNT)> buttons;

        InputState() { Reset(); }

        void Reset() {
            dx = dy = dz = 0;
            scroll = 0.0f;
            mouse_move = Vector2f{ 0, 0 };
            ndc = std::nullopt;
            buttons.fill(0);
        }
    } m_inputState;

    // ViewerTool m_activeTool{ ViewerTool::GizmoEditing };
    std::array<std::shared_ptr<IEditorTool>, std::to_underlying(EditorToolType::Count)> m_tools;
    EditorToolType m_active{ EditorToolType::Gizmo };

    friend class Gizmo;
    friend class TileMapEditor;
};

}  // namespace my
