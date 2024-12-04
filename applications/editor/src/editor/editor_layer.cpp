#include "editor_layer.h"

#include <imgui/imgui_internal.h>

#include "editor/panels/content_browser.h"
#include "editor/panels/hierarchy_panel.h"
#include "editor/panels/log_panel.h"
#include "editor/panels/propertiy_panel.h"
#include "editor/panels/renderer_panel.h"
#include "editor/panels/viewer.h"
#include "engine/assets/asset.h"
#include "engine/core/framework/asset_registry.h"
#include "engine/core/framework/input_manager.h"
#include "engine/core/framework/layer.h"
#include "engine/core/framework/scene_manager.h"
#include "engine/core/io/input_event.h"
#include "engine/core/string/string_utils.h"

namespace my {

EditorLayer::EditorLayer() : Layer("EditorLayer") {
    AddPanel(std::make_shared<LogPanel>(*this));
    AddPanel(std::make_shared<RendererPanel>(*this));
    AddPanel(std::make_shared<HierarchyPanel>(*this));
    AddPanel(std::make_shared<PropertyPanel>(*this));
    AddPanel(std::make_shared<Viewer>(*this));
    AddPanel(std::make_shared<ContentBrowser>(*this));

    m_menuBar = std::make_shared<MenuBar>(*this);

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
    auto asset_registry = m_app->GetAssetRegistry();
    m_playButtonImage = asset_registry->GetAssetByHandle<ImageAsset>(AssetHandle{ "@res://images/icons/play.png" });
    m_pauseButtonImage = asset_registry->GetAssetByHandle<ImageAsset>(AssetHandle{ "@res://images/icons/pause.png" });

    m_app->GetInputManager()->GetEventQueue().RegisterListener(this);

    for (auto& panel : m_panels) {
        panel->OnAttach();
    }
}

void EditorLayer::OnDetach() {
    m_app->GetInputManager()->GetEventQueue().UnregisterListener(this);
}

void EditorLayer::AddPanel(std::shared_ptr<EditorItem> p_panel) {
    m_panels.emplace_back(p_panel);
}

void EditorLayer::SelectEntity(ecs::Entity p_selected) {
    m_selected = p_selected;
    // TODO: fix this, shouldn't fetch globally
    // SceneManager::GetScene().m_selected = m_selected;
    LOG_ERROR("TODO: fix select entity");
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

void EditorLayer::DrawToolbar() {
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 2));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemInnerSpacing, ImVec2(0, 0));
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
    auto& colors = ImGui::GetStyle().Colors;
    const auto& button_hovered = colors[ImGuiCol_ButtonHovered];
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(button_hovered.x, button_hovered.y, button_hovered.z, 0.5f));
    const auto& button_active = colors[ImGuiCol_ButtonActive];
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(button_active.x, button_active.y, button_active.z, 0.5f));

    ImGuiWindowFlags toolbar_flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoDocking;
    ImGui::Begin("##toolbar", nullptr, toolbar_flags);

    bool toolbar_enabled = true;
    ImVec4 tint_color = ImVec4(1, 1, 1, 1);
    if (!toolbar_enabled) {
        tint_color.w = 0.5f;
    }

    float size = ImGui::GetWindowHeight() - 4.0f;
    ImGui::SetCursorPosX((ImGui::GetWindowContentRegionMax().x * 0.5f) - (size * 0.5f));

    auto app_state = m_app->GetState();
    if (auto image = m_playButtonImage; image && image->gpu_texture) {
        ImVec2 image_size(static_cast<float>(image->width), static_cast<float>(image->height));
        bool disable = app_state != Application::State::EDITING;
        ImGui::BeginDisabled(disable);
        if (ImGui::ImageButton("play", (ImTextureID)image->gpu_texture->GetHandle(), image_size)) {
            m_app->SetState(Application::State::BEGIN_SIM);
        }
        ImGui::EndDisabled();
    }
    ImGui::SameLine();
    if (auto image = m_pauseButtonImage; image && image->gpu_texture) {
        ImVec2 image_size(static_cast<float>(image->width), static_cast<float>(image->height));
        bool disable = app_state != Application::State::SIM;
        ImGui::BeginDisabled(disable);
        if (ImGui::ImageButton("pause", (ImTextureID)image->gpu_texture->GetHandle(), image_size)) {
            m_app->SetState(Application::State::END_SIM);
        }
        ImGui::EndDisabled();
    }

    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(3);
    ImGui::End();
}

void EditorLayer::OnUpdate(float) {
    Scene* scene = SceneManager::GetSingleton().GetScenePtr();
    switch (m_app->GetState()) {
        case Application::State::EDITING: {
            m_app->SetActiveScene(scene);
        } break;
        case Application::State::BEGIN_SIM: {
            if (m_simScene) {
                delete m_simScene;
            }
            m_simScene = new Scene;
            m_simScene->Copy(*scene);
            m_app->SetActiveScene(m_simScene);
            m_app->SetState(Application::State::SIM);
        } break;
        case Application::State::END_SIM: {
            if (DEV_VERIFY(m_simScene)) {
                delete m_simScene;
                m_simScene = nullptr;
            }
            m_app->SetActiveScene(scene);
            m_app->SetState(Application::State::EDITING);
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
    DrawToolbar();
    FlushCommand(*scene);

    m_unhandledEvents.clear();
}

void EditorLayer::EventReceived(std::shared_ptr<IEvent> p_event) {
    bool handled = false;
    if (InputEventKey* e = dynamic_cast<InputEventKey*>(p_event.get()); e) {
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

    if (!handled) {
        if (!handled) {
            // save unhandled events
            m_unhandledEvents.emplace_back(p_event);
        }
    }
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

}  // namespace my
