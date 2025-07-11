#include "viewer.h"

#include <IconsFontAwesome/IconsFontAwesome6.h>
#include <imgui/imgui_internal.h>

#include "editor/editor_layer.h"
#include "editor/utility/imguizmo.h"
#include "editor/widget.h"
#include "engine/input/input_event.h"
#include "engine/math/ray.h"
#include "engine/renderer/graphics_dvars.h"
#include "engine/renderer/graphics_manager.h"
#include "engine/runtime/common_dvars.h"
#include "engine/runtime/display_manager.h"
#include "engine/runtime/input_manager.h"
#include "engine/runtime/scene_manager.h"

namespace my {

Viewer::Viewer(EditorLayer& p_editor) : EditorWindow("Viewer", p_editor) {
    // m_flags = ImGuiWindowFlags_MenuBar;
}

void Viewer::UpdateData() {
    Vector2i frame_size = DVAR_GET_IVEC2(resolution);
    int frame_width = frame_size.x;
    int frame_height = frame_size.y;
    const float ratio = (float)frame_width / frame_height;
    m_canvasSize.x = ImGui::GetWindowSize().x;
    m_canvasSize.y = ImGui::GetWindowSize().y;
    if (m_canvasSize.y * ratio > m_canvasSize.x) {
        m_canvasSize.y = m_canvasSize.x / ratio;
    } else {
        m_canvasSize.x = m_canvasSize.y * ratio;
    }

    ImGuiWindow* window = ImGui::FindWindowByName(m_name.c_str());
    DEV_ASSERT(window);
    m_canvasMin.x = window->ContentRegionRect.Min.x;
    // @TODO: fix hard code
    m_canvasMin.y = 40 + window->ContentRegionRect.Min.y;

    m_focused = ImGui::IsWindowHovered();
}

void Viewer::SelectEntity(Scene& p_scene, const CameraComponent& p_camera) {
    if (!m_focused) {
        return;
    }

    if (InputManager::GetSingleton().IsButtonPressed(MouseButton::RIGHT)) {
        auto [window_x, window_y] = DisplayManager::GetSingleton().GetWindowPos();
        Vector2f clicked = InputManager::GetSingleton().GetCursor();
        clicked.x = (clicked.x + window_x - m_canvasMin.x) / m_canvasSize.x;
        clicked.y = (clicked.y + window_y - m_canvasMin.y) / m_canvasSize.y;

        if (clicked.x >= 0.0f && clicked.x <= 1.0f && clicked.y >= 0.0f && clicked.y <= 1.0f) {
            clicked *= 2.0f;
            clicked -= 1.0f;

            const Matrix4x4f inversed_projection_view = glm::inverse(p_camera.GetProjectionViewMatrix());

            const Vector3f ray_start = p_camera.GetPosition();
            const Vector3f direction = normalize(Vector3f((inversed_projection_view * Vector4f(clicked, 1.0f, 1.0f)).xyz));
            const Vector3f ray_end = ray_start + direction * p_camera.GetFar();
            Ray ray(ray_start, ray_end);

            const auto result = p_scene.Intersects(ray);

            m_editor.SelectEntity(result.entity);
        }
    }
}

void Viewer::DrawGui(Scene& p_scene, CameraComponent& p_camera) {
    const Matrix4x4f view_matrix = p_camera.GetViewMatrix();
    const Matrix4x4f proj_matrix = p_camera.GetProjectionMatrix();

    ImGuizmo::SetOrthographic(false);
    ImGuizmo::BeginFrame();

    ImGuizmo::SetDrawlist();
    ImGuizmo::SetRect(m_canvasMin.x, m_canvasMin.y, m_canvasSize.x, m_canvasSize.y);

    // add image for drawing
    ImVec2 top_left(m_canvasMin.x, m_canvasMin.y);
    ImVec2 bottom_right(top_left.x + m_canvasSize.x, top_left.y + m_canvasSize.y);

    // @TODO: fix this
    const auto& gm = IGraphicsManager::GetSingleton();
    uint64_t handle = gm.GetFinalImage();

    switch (gm.GetBackend()) {
        case Backend::D3D11:
        case Backend::D3D12: {
            ImGui::GetWindowDrawList()->AddImage((ImTextureID)handle, top_left, bottom_right);
        } break;
        case Backend::OPENGL: {
            ImVec2 uv_min = ImVec2(0, 1);
            ImVec2 uv_max = ImVec2(1, 0);
            if (gm.GetActiveRenderGraphName() == RenderGraphName::PATHTRACER) {
                uv_min = ImVec2(0, 0);
                uv_max = ImVec2(1, 1);
            }
            ImGui::GetWindowDrawList()->AddImage((ImTextureID)handle, top_left, bottom_right, uv_min, uv_max);
        } break;
        case Backend::VULKAN:
        case Backend::METAL: {
        } break;
        default:
            CRASH_NOW();
            break;
    }

    bool show_editor = DVAR_GET_BOOL(show_editor);
    if (show_editor) {
        Matrix4x4f identity(1.0f);
        auto plane = m_editor.context.cameraType == CAMERA_2D ? ImGuizmo::GridPlane::XY : ImGuizmo::GridPlane::XZ;
        ImGuizmo::DrawGrid(p_camera.GetProjectionViewMatrix(), identity, 10.0f, plane);
    }

#if 0  // debug code
    {
        static uint8_t dummy = 0;
        if (dummy % 128 == 0) {
            const auto& pos = p_camera.GetPosition();
            LOG_OK("Camera at {} {} {}", pos.x, pos.y, pos.z);
        }
        dummy++;
    }
#endif

    ecs::Entity id = m_editor.GetSelectedEntity();
    TransformComponent* transform_component = p_scene.GetComponent<TransformComponent>(id);
    if (transform_component) {
        LightComponent* light = p_scene.GetComponent<LightComponent>(id);
        if (light && light->GetType() == LIGHT_TYPE_INFINITE) {
            const auto& matrix = transform_component->GetWorldMatrix();
            ImGuizmo::DrawCone(p_camera.GetProjectionViewMatrix(), matrix);
        }
    }

    auto draw_gizmo = [&](ImGuizmo::OPERATION p_operation, CommandType p_type) {
        if (transform_component) {
            const Matrix4x4f before = transform_component->GetLocalMatrix();
            Matrix4x4f after = before;
            if (ImGuizmo::Manipulate(glm::value_ptr(view_matrix),
                                     glm::value_ptr(proj_matrix),
                                     p_operation,
                                     // ImGuizmo::LOCAL,
                                     ImGuizmo::WORLD,
                                     glm::value_ptr(after),
                                     nullptr, nullptr, nullptr, nullptr)) {

                auto command = std::make_shared<EntityTransformCommand>(p_type, p_scene, id, before, after);
                m_editor.BufferCommand(command);
            }
        }
    };

    // draw gizmo
    switch (m_editor.GetState()) {
        case EditorLayer::STATE_TRANSLATE:
            draw_gizmo(ImGuizmo::TRANSLATE, COMMAND_TYPE_ENTITY_TRANSLATE);
            break;
        case EditorLayer::STATE_ROTATE:
            draw_gizmo(ImGuizmo::ROTATE, COMMAND_TYPE_ENTITY_ROTATE);
            break;
        case EditorLayer::STATE_SCALE:
            draw_gizmo(ImGuizmo::SCALE, COMMAND_TYPE_ENTITY_SCALE);
            break;
        default:
            break;
    }

    if (show_editor) {
        // draw view cube
        const float size = 120.f;
        ImGuizmo::ViewManipulate((float*)&view_matrix[0].x, 10.0f, ImVec2(m_canvasMin.x, m_canvasMin.y), ImVec2(size, size), IM_COL32(64, 64, 64, 96));
    }
}

struct ToolBarButtonDesc {
    const char* display{ nullptr };
    const char* tooltip{ nullptr };
    std::function<void()> func;
    std::function<bool()> enabledFunc;
};

void Viewer::DrawToolBar() {
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
    auto& colors = ImGui::GetStyle().Colors;
    const auto& button_hovered = colors[ImGuiCol_ButtonHovered];
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(button_hovered.x, button_hovered.y, button_hovered.z, 0.5f));
    const auto& button_active = colors[ImGuiCol_ButtonActive];
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(button_active.x, button_active.y, button_active.z, 0.5f));

    auto app = m_editor.GetApplication();
    auto app_state = app->GetState();
    auto& context = m_editor.context;

    static const ToolBarButtonDesc s_buttons[] = {
        { ICON_FA_PLAY, "Run Project",
          [&]() { app->SetState(Application::State::BEGIN_SIM); },
          [&]() { return app_state != Application::State::SIM; } },
        { ICON_FA_PAUSE, "Pause Running Project",
          [&]() { app->SetState(Application::State::END_SIM); },
          [&]() { return app_state != Application::State::EDITING; } },
        { ICON_FA_HAND, "Enter gizmo mode",
          [&]() {} },
        { ICON_FA_CAMERA_ROTATE, "Toggle 2D/3D view",
          [&]() {
              bool is_2d = context.cameraType == CAMERA_2D;
              context.cameraType = is_2d ? CAMERA_3D : CAMERA_2D;
          } },
        { ICON_FA_BRUSH, "TileMap editor mode",
          [&]() {} },
    };

    for (int i = 0; i < array_length(s_buttons); ++i) {
        const auto& desc = s_buttons[i];
        const bool enabled = desc.enabledFunc ? desc.enabledFunc() : true;

        if (!enabled) {
            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.5f);
        }

        if (ImGui::Button(desc.display) && enabled) {
            desc.func();
        }

        if (!enabled) {
            ImGui::PopStyleVar();
        }

        if (ImGui::IsItemHovered()) {
            ImGui::BeginTooltip();
            ImGui::Text(desc.tooltip);
            ImGui::EndTooltip();
        }

        ImGui::SameLine();
    }

#if 0
    ImGui::Button(ICON_FA_PAUSE " ");
    ImGui::SameLine();
    ImGui::Button(ICON_FA_PEN " ");
    ImGui::SameLine();
    ImGui::Button(ICON_FA_CAMERA " ");
    ImGui::SameLine();
    ImGui::Button(ICON_FA_CAMERA_RETRO " ");
    ImGui::SameLine();
    ImGui::Button(ICON_FA_PAINT_ROLLER " ");
    ImGui::SameLine();
    ImGui::Button(ICON_FA_PAINTBRUSH " ");
    ImGui::SameLine();
    ImGui::Button(ICON_FA_BRUSH " ");
#endif

    // ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(3);
}

bool Viewer::HandleInput(std::shared_ptr<InputEvent> p_input_event) {
    // @TODO: split to gizmo translate, gizmo rotate, etc
    InputEvent* event = p_input_event.get();
    if (auto e = dynamic_cast<InputEventKey*>(event); e) {
#if 0
        if (e->IsPressed()) {
            switch (e->GetKey()) {
                case KeyCode::KEY_Z: {
                    m_editor.SetState(EditorLayer::STATE_TRANSLATE);
                } break;
                case KeyCode::KEY_X: {
                    m_editor.SetState(EditorLayer::STATE_ROTATE);
                } break;
                case KeyCode::KEY_C: {
                    m_editor.SetState(EditorLayer::STATE_SCALE);
                } break;
                default:
                    break;
            }
            return true;
        }
#endif

        if (e->IsHolding() && !e->IsModiferPressed()) {
            bool handled = true;
            switch (e->GetKey()) {
                case KeyCode::KEY_D:
                    ++m_inputState.dx;
                    break;
                case KeyCode::KEY_A:
                    --m_inputState.dx;
                    break;
                case KeyCode::KEY_E:
                    ++m_inputState.dy;
                    break;
                case KeyCode::KEY_Q:
                    --m_inputState.dy;
                    break;
                case KeyCode::KEY_W:
                    ++m_inputState.dz;
                    break;
                case KeyCode::KEY_S:
                    --m_inputState.dz;
                    break;
                default:
                    handled = false;
                    break;
            }
            return handled;
        }
    }

    if (auto e = dynamic_cast<InputEventMouseWheel*>(event); e) {
        if (!e->IsModiferPressed()) {
            m_inputState.scroll += 3.0f * e->GetWheelY();
            return true;
        }
    }

    if (auto e = dynamic_cast<InputEventMouseMove*>(event); e) {
        if (!e->IsModiferPressed() && e->IsButtonDown(MouseButton::MIDDLE)) {
            m_inputState.mouse_move += e->GetDelta();
            return true;
        }
    }

    return false;
}

void Viewer::UpdateInternal(Scene& p_scene) {
    DrawToolBar();

    const auto mode = m_editor.GetApplication()->GetState();
    ecs::Entity camera_id;
    switch (mode) {
        case Application::State::EDITING:
            camera_id = p_scene.GetEditorCamera();
            break;
        case Application::State::SIM:
            camera_id = p_scene.GetMainCamera();
            break;
        case Application::State::BEGIN_SIM:
            break;
        case Application::State::END_SIM:
            break;
        default:
            break;
    }

    CameraComponent& camera = m_editor.context.GetActiveCamera();

    ImGui::Dummy(ImGui::GetContentRegionAvail());
    if (ImGui::BeginDragDropTarget()) {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(EditorItem::DRAG_DROP_ENV)) {
            IM_ASSERT(payload->DataSize == sizeof(const char*));
            char* dragged_data = *(char**)payload->Data;

            // @TODO: no strdup and free
            free(dragged_data);
        }
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(EditorItem::DRAG_DROP_IMPORT)) {
            IM_ASSERT(payload->DataSize == sizeof(const char*));
            char* dragged_data = *(char**)payload->Data;
            SceneManager::GetSingleton().RequestScene(dragged_data);

            // @TODO: no strdup and free
            free(dragged_data);
        }
        ImGui::EndDragDropTarget();
    }

    UpdateData();

    if (m_focused && mode == Application::State::EDITING) {
        const float dt = m_editor.context.timestep;
        const auto& move = m_inputState.mouse_move;
        const auto& scroll = m_inputState.scroll;
        switch (m_editor.context.cameraType) {
            case CAMERA_2D: {
                CameraInputState state{
                    .move = dt * Vector3f(-move.x, move.y, 0.0f),
                    .zoomDelta = -dt * scroll,
                };
                m_cameraController2D.Update(camera, state);
            } break;
            case CAMERA_3D: {
                CameraInputState state{
                    .move = dt * Vector3f(m_inputState.dx, m_inputState.dy, m_inputState.dz),
                    .zoomDelta = dt * scroll,
                    .rotation = dt * move,
                };
                m_cameraController3D.Update(camera, state);
            } break;
            default:
                CRASH_NOW();
                break;
        }
        camera.Update();
    }

    SelectEntity(p_scene, camera);

    DrawGui(p_scene, camera);

    m_inputState.Reset();
}

}  // namespace my
