#include "editor_layer.h"

#include <imgui/imgui_internal.h>
#include <imnodes/imnodes.h>

#include "editor/panels/asset_inspector.h"
#include "editor/panels/content_browser.h"
#include "editor/panels/file_system_panel.h"
#include "editor/panels/hierarchy_panel.h"
#include "editor/panels/log_panel.h"
#include "editor/panels/menu_bar.h"
#include "editor/panels/propertiy_panel.h"
#include "editor/panels/render_graph_viewer.h"
#include "editor/panels/renderer_panel.h"
#include "editor/panels/viewer.h"
#include "editor/widget.h"
#include "engine/input/input_event.h"
#include "engine/core/string/string_utils.h"
#include "engine/renderer/graphics_manager.h"
#include "engine/runtime/input_manager.h"
#include "engine/runtime/layer.h"
#include "engine/runtime/physics_manager.h"
#include "engine/runtime/scene_manager.h"
#include "engine/runtime/script_manager.h"

// @NOTE: include dvars at last
#include "engine/renderer/graphics_dvars.h"

namespace my {

EditorLayer::EditorLayer() : Layer("EditorLayer") {
    const auto res = DVAR_GET_IVEC2(resolution);
    {
        CameraComponent& camera = context.cameras[CAMERA_3D];
        camera.SetDimension(res.x, res.y);
        camera.SetNear(1.0f);
        camera.SetFar(1000.0f);
        camera.SetPosition(Vector3f(0, 4, 10));
        camera.SetDirty();
        camera.Update();
    }
    {
        CameraComponent& camera = context.cameras[CAMERA_2D];
        camera.SetOrtho();
        camera.SetDimension(res.x, res.y);
        camera.SetNear(1.0f);
        camera.SetFar(1000.0f);
        camera.SetPosition(Vector3f(0, 0, 10));
        camera.SetDirty();
        camera.Update();
    }

    m_menuBar = std::make_shared<MenuBar>(*this);
    m_viewer = std::make_shared<Viewer>(*this);

    AddPanel(std::make_shared<LogPanel>(*this));
    AddPanel(std::make_shared<RendererPanel>(*this));
    AddPanel(std::make_shared<HierarchyPanel>(*this));
    AddPanel(std::make_shared<PropertyPanel>(*this));
    AddPanel(m_viewer);
    AddPanel(std::make_shared<AssetInspector>(*this));
    AddPanel(std::make_shared<RenderGraphViewer>(*this));
    AddPanel(std::make_shared<FileSystemPanel>(*this));
#if !USING(PLATFORM_WASM)
    AddPanel(std::make_shared<ContentBrowser>(*this));
#endif

    m_shortcuts[SHORT_CUT_SAVE_AS] = {
        "Save As..",
        "Ctrl+Shift+S",
        [&]() {
            this->BufferCommand(std::make_shared<SaveProjectCommand>(true));
        },
    };
    m_shortcuts[SHORT_CUT_SAVE] = {
        "Save",
        "Ctrl+S",
        [&]() { this->BufferCommand(std::make_shared<SaveProjectCommand>(false)); },
    };
    m_shortcuts[SHORT_CUT_OPEN] = {
        "Open",
        "Ctrl+O",
        [&]() { this->BufferCommand(std::make_shared<OpenProjectCommand>(true)); },
    };
    m_shortcuts[SHORT_CUT_REDO] = {
        "Redo",
        "Ctrl+Shift+Z",
        [&]() { this->BufferCommand(std::make_shared<RedoViewerCommand>()); },
        [&]() { return this->GetUndoStack().CanRedo(); },
    };
    m_shortcuts[SHORT_CUT_UNDO] = {
        "Undo",
        "Ctrl+Z",
        [&]() { this->BufferCommand(std::make_shared<UndoViewerCommand>()); },
        [&]() { return this->GetUndoStack().CanUndo(); },
    };

    // @TODO: proper key mapping
    std::map<std::string_view, KeyCode> keyMapping = {
        { "A", KeyCode::KEY_A },
        { "B", KeyCode::KEY_B },
        { "C", KeyCode::KEY_C },
        { "D", KeyCode::KEY_D },
        { "E", KeyCode::KEY_E },
        { "F", KeyCode::KEY_F },
        { "G", KeyCode::KEY_G },
        { "H", KeyCode::KEY_H },
        { "I", KeyCode::KEY_I },
        { "J", KeyCode::KEY_J },
        { "K", KeyCode::KEY_K },
        { "L", KeyCode::KEY_L },
        { "M", KeyCode::KEY_M },
        { "N", KeyCode::KEY_N },
        { "O", KeyCode::KEY_O },
        { "P", KeyCode::KEY_P },
        { "Q", KeyCode::KEY_Q },
        { "R", KeyCode::KEY_R },
        { "S", KeyCode::KEY_S },
        { "T", KeyCode::KEY_T },
        { "U", KeyCode::KEY_U },
        { "V", KeyCode::KEY_V },
        { "W", KeyCode::KEY_W },
        { "X", KeyCode::KEY_X },
        { "Y", KeyCode::KEY_Y },
        { "Z", KeyCode::KEY_Z },
    };

    for (auto& shortcut : m_shortcuts) {
        StringSplitter split(shortcut.shortcut);
        while (split.CanAdvance()) {
            std::string_view sv = split.Advance('+');
            if (sv == "Ctrl") {
                shortcut.ctrl = true;
            } else if (sv == "Shift") {
                shortcut.shift = true;
            } else if (sv == "Alt") {
                shortcut.alt = true;
            } else {
                if (sv.length() == 1) {
                    auto it = keyMapping.find(sv);
                    if (it == keyMapping.end()) {
                        CRASH_NOW();
                    }
                    shortcut.key = it->second;
                }
            }
        }
    }
}

void EditorLayer::OnAttach() {
    ImNodes::CreateContext();

    m_app->GetInputManager()->PushInputHandler(this);
    m_app->GetInputManager()->PushInputHandler(m_viewer.get());

    for (auto& panel : m_panels) {
        panel->OnAttach();
    }
}

void EditorLayer::OnDetach() {
    auto handler = m_app->GetInputManager()->PopInputHandler();
    DEV_ASSERT(handler == m_viewer.get());
    handler = m_app->GetInputManager()->PopInputHandler();
    DEV_ASSERT(handler == this);

    ImNodes::DestroyContext();
}

void EditorLayer::AddPanel(std::shared_ptr<EditorItem> p_panel) {
    m_panels.emplace_back(p_panel);
}

void EditorLayer::SelectEntity(ecs::Entity p_selected) {
    m_selected = p_selected;
    Scene* scene = m_app->GetActiveScene();
    scene->m_selected = m_selected;
}

void EditorLayer::DockSpace(Scene& p_scene) {
    ImGui::GetMainViewport();

    static bool opt_padding = false;
    static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_PassthruCentralNode;

    // We are using the ImGuiWindowFlags_NoDocking flag to make the parent window not dockable into,
    // because it would be confusing to have two docking targets within each others.
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    ImGui::SetNextWindowViewport(viewport->ID);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    window_flags |=
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
    window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

    if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode) {
        window_flags |= ImGuiWindowFlags_NoBackground;
    }

    if (!opt_padding) {
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    }
    ImGui::Begin("DockSpace Demo", nullptr, window_flags);
    if (!opt_padding) {
        ImGui::PopStyleVar();
    }

    ImGui::PopStyleVar(2);

    // Submit the DockSpace
    ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
    ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);

    m_menuBar->Update(p_scene);

    ImGui::End();
    return;
}

void EditorLayer::OnUpdate(float p_timestep) {
    context.timestep = p_timestep;

    Scene* scene = SceneManager::GetSingleton().GetScenePtr();
    switch (m_app->GetState()) {
        case Application::State::EDITING: {
            m_app->SetActiveScene(scene);
        } break;
        case Application::State::BEGIN_SIM: {
            DEV_ASSERT(m_simScene == nullptr);

            m_simScene = new Scene;
            m_simScene->Copy(*scene);
            m_simScene->Update(0.0f);

            m_app->SetActiveScene(m_simScene);
            m_app->SetState(Application::State::SIM);

            m_app->AttachGameLayer();
        } break;
        case Application::State::END_SIM: {
            m_app->DetachGameLayer();

            m_app->SetActiveScene(scene);
            m_app->SetState(Application::State::EDITING);

            if (DEV_VERIFY(m_simScene)) {
                delete m_simScene;
                m_simScene = nullptr;
            }
        } break;
        case Application::State::SIM: {
            DEV_ASSERT(m_simScene);
            m_app->SetActiveScene(m_simScene);
        } break;
        default:
            CRASH_NOW();
            break;
    }
}

void EditorLayer::OnImGuiRender() {
    Scene* scene = m_app->GetActiveScene();
    DEV_ASSERT(scene);

    // @TODO: fix this
    DockSpace(*scene);
    for (auto& it : m_panels) {
        it->Update(*scene);
    }
    FlushCommand(*scene);
}

bool EditorLayer::HandleInput(std::shared_ptr<InputEvent> p_input_event) {
    bool handled = false;
    InputEvent* event = p_input_event.get();
    if (auto e = dynamic_cast<InputEventKey*>(event); e) {
        for (auto shortcut : m_shortcuts) {
            // @TODO: refactor this
            auto is_key_handled = [&]() {
                if (!e->IsPressed()) {
                    return false;
                }
                if (e->GetKey() != shortcut.key) {
                    return false;
                }
                if (e->IsAltPressed() != shortcut.alt) {
                    return false;
                }
                if (e->IsShiftPressed() != shortcut.shift) {
                    return false;
                }
                if (e->IsCtrlPressed() != shortcut.ctrl) {
                    return false;
                }
                return true;
            };
            if (is_key_handled()) {
                shortcut.executeFunc();
                handled = true;
                break;
            }
        }
    }

    return handled;
}

void EditorLayer::BufferCommand(std::shared_ptr<EditorCommandBase>&& p_command) {
    p_command->m_editor = this;
    m_commandBuffer.emplace_back(std::move(p_command));
}

void EditorLayer::AddComponent(ComponentType p_type, ecs::Entity p_target) {
    auto command = std::make_shared<EditorCommandAddComponent>(p_type);
    command->target = p_target;
    BufferCommand(command);
}

void EditorLayer::AddEntity(EntityType p_type, ecs::Entity p_parent) {
    auto command = std::make_shared<EditorCommandAddEntity>(p_type);
    command->m_parent = p_parent;
    BufferCommand(command);
}

void EditorLayer::RemoveEntity(ecs::Entity p_target) {
    auto command = std::make_shared<EditorCommandRemoveEntity>(p_target);
    BufferCommand(command);
}

void EditorLayer::FlushCommand(Scene& p_scene) {
    while (!m_commandBuffer.empty()) {
        auto task = m_commandBuffer.front();
        m_commandBuffer.pop_front();
        if (auto undo_command = std::dynamic_pointer_cast<UndoCommand>(task); undo_command) {
            m_undoStack.PushCommand(std::move(undo_command));
            continue;
        }
        task->Execute(p_scene);
    }
}

CameraComponent& EditorLayer::GetActiveCamera() {
    return context.GetActiveCamera();
}

}  // namespace my
