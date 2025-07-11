#pragma once
#include "editor/editor_command.h"
#include "editor/editor_window.h"
#include "engine/core/base/ring_buffer.h"
#include "engine/input/input_router.h"
#include "engine/runtime/application.h"
#include "engine/runtime/layer.h"
#include "engine/scene/scene.h"
#include "engine/scene/scene_component.h"
#include "engine/systems/undo_redo/undo_stack.h"

namespace my {

struct ImageAsset;
class MenuBar;
enum class KeyCode : uint16_t;
class Viewer;

enum {
    SHORT_CUT_SAVE_AS = 0,
    SHORT_CUT_SAVE,
    SHORT_CUT_OPEN,
    SHORT_CUT_UNDO,
    SHORT_CUT_REDO,
    SHORT_CUT_MAX,
};

enum EditorCameraType : uint8_t {
    CAMERA_2D = 0,
    CAMERA_3D,
    CAMERA_MAX,
};

struct EditorContext {
    float timestep{ 0 };
    EditorCameraType cameraType{ CAMERA_3D };
    CameraComponent cameras[CAMERA_MAX];

    CameraComponent& GetActiveCamera() {
        return cameras[cameraType];
    }
};

enum class EditorState {
    TileMapEditing,
    Translating,
    Rotating,
    Scaling,
};

class EditorLayer : public Layer, public IInputHandler {
public:
    enum State {
        STATE_TRANSLATE,
        STATE_ROTATE,
        STATE_SCALE,
    };

    EditorLayer();
    virtual ~EditorLayer() = default;

    void OnAttach() override;
    void OnDetach() override;

    void OnUpdate(float p_timestep) override;
    void OnImGuiRender() override;

    void SelectEntity(ecs::Entity p_selected);
    ecs::Entity GetSelectedEntity() const { return m_selected; }
    State GetState() const { return m_state; }
    void SetState(State p_state) { m_state = p_state; }

    uint64_t GetDisplayedImage() const { return m_displayedImage; }
    void SetDisplayedImage(uint64_t p_image) { m_displayedImage = p_image; }

    void BufferCommand(std::shared_ptr<EditorCommandBase>&& p_command);
    void AddComponent(ComponentType p_type, ecs::Entity p_target);
    void AddEntity(EntityType p_type, ecs::Entity p_parent);
    void RemoveEntity(ecs::Entity p_target);

    UndoStack& GetUndoStack() { return m_undoStack; }

    bool HandleInput(std::shared_ptr<InputEvent> p_input_event) override;

    const auto& GetShortcuts() const { return m_shortcuts; }

    CameraComponent& GetActiveCamera();

    EditorContext context;

private:
    void DockSpace(Scene& p_scene);
    void AddPanel(std::shared_ptr<EditorItem> p_panel);

    void FlushCommand(Scene& p_scene);

    std::shared_ptr<MenuBar> m_menuBar;
    std::shared_ptr<Viewer> m_viewer;

    std::vector<std::shared_ptr<EditorItem>> m_panels;
    ecs::Entity m_selected;
    State m_state{ STATE_TRANSLATE };
    Scene* m_simScene{ nullptr };

    uint64_t m_displayedImage = 0;
    std::list<std::shared_ptr<EditorCommandBase>> m_commandBuffer;
    UndoStack m_undoStack;

    struct ShortcutDesc {
        const char* name{ nullptr };
        const char* shortcut{ nullptr };
        std::function<void()> executeFunc{ nullptr };
        std::function<bool()> enabledFunc{ nullptr };

        KeyCode key{};
        bool ctrl{};
        bool alt{};
        bool shift{};
    };

    std::array<ShortcutDesc, SHORT_CUT_MAX> m_shortcuts;
};

}  // namespace my
