#include "viewer.h"

#include <imgui/imgui_internal.h>

#include "engine/core/framework/common_dvars.h"
#include "engine/core/framework/display_manager.h"
#include "engine/core/framework/graphics_manager.h"
#include "engine/core/framework/input_manager.h"
#include "engine/core/framework/scene_manager.h"
#include "engine/core/io/input_event.h"
#include "engine/core/math/ray.h"
#include "editor/editor_layer.h"
#include "editor/utility/imguizmo.h"
#include "rendering/graphics_dvars.h"
#include "rendering/render_manager.h"

namespace my {

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
    m_canvasMin.y = window->ContentRegionRect.Min.y;

    m_focused = ImGui::IsWindowHovered();
}

void Viewer::SelectEntity(Scene& p_scene, const Camera& p_camera) {
    if (!m_focused) {
        return;
    }

    if (InputManager::GetSingleton().IsButtonPressed(MOUSE_BUTTON_RIGHT)) {
        auto [window_x, window_y] = DisplayManager::GetSingleton().GetWindowPos();
        Vector2f clicked = InputManager::GetSingleton().GetCursor();
        clicked.x = (clicked.x + window_x - m_canvasMin.x) / m_canvasSize.x;
        clicked.y = (clicked.y + window_y - m_canvasMin.y) / m_canvasSize.y;

        if (clicked.x >= 0.0f && clicked.x <= 1.0f && clicked.y >= 0.0f && clicked.y <= 1.0f) {
            clicked *= 2.0f;
            clicked -= 1.0f;

            const Matrix4x4f inversed_projection_view = glm::inverse(p_camera.GetProjectionViewMatrix());

            const Vector3f ray_start = p_camera.GetPosition();
            const Vector3f direction = glm::normalize(Vector3f(inversed_projection_view * Vector4f(clicked.x, -clicked.y, 1.0f, 1.0f)));
            const Vector3f ray_end = ray_start + direction * p_camera.GetFar();
            Ray ray(ray_start, ray_end);

            const auto result = p_scene.Intersects(ray);

            m_editor.SelectEntity(result.entity);
        }
    }
}

void Viewer::DrawGui(Scene& p_scene, Camera& p_camera) {
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
    const auto& gm = GraphicsManager::GetSingleton();
    uint64_t handle = gm.GetFinalImage();

    ImVec2 uv_min(0, 1);
    ImVec2 uv_max(1, 0);
    switch (gm.GetBackend()) {
        case Backend::D3D11:
        case Backend::D3D12: {
            uv_min = ImVec2(0, 0);
            uv_max = ImVec2(1, 1);
            ImGui::GetWindowDrawList()->AddImage((ImTextureID)handle, top_left, bottom_right, uv_min, uv_max);
        } break;
        case Backend::OPENGL: {
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
        Matrix4x4f identity(1);
        // draw grid
        ImGuizmo::draw_grid(p_camera.GetProjectionViewMatrix(), identity, 10.0f);
    }

    ecs::Entity id = m_editor.GetSelectedEntity();
    TransformComponent* transform_component = p_scene.GetComponent<TransformComponent>(id);
#if 0
    if (transform_component) {
        AABB aabb;
        if (const ObjectComponent* object = scene.get_component<ObjectComponent>(id); object) {
            MeshComponent* mesh = scene.get_component<MeshComponent>(object->mesh_id);
            transform_component = scene.get_component<TransformComponent>(id);
            DEV_ASSERT(transform_component);
            aabb = mesh->local_bound;
        }

        if (aabb.is_valid()) {
            aabb.apply_matrix(transform_component->get_world_matrix());

            const Matrix4x4f model_matrix = glm::translate(Matrix4x4f(1), aabb.center()) * glm::scale(Matrix4x4f(1), aabb.size());
            ImGuizmo::draw_box_wireframe(projection_view_matrix, model_matrix);
        }
    }
    ObjectComponent* object_component = scene.get_component<ObjectComponent>(id);
    if (object_component && transform_component) {
        auto mesh_component = scene.get_component<MeshComponent>(object_component->mesh_id);
        DEV_ASSERT(mesh_component);
        AABB aabb = mesh_component->local_bound;
        aabb.apply_matrix(transform_component->get_world_matrix());

        const Matrix4x4f model_matrix = glm::translate(Matrix4x4f(1), aabb.center()) * glm::scale(Matrix4x4f(1), aabb.size());
        ImGuizmo::draw_box_wireframe(projection_view_matrix, model_matrix);
    }
#endif

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

void Viewer::UpdateInternal(Scene& p_scene) {
    Camera& camera = *p_scene.m_camera;

    ImGui::Dummy(ImGui::GetContentRegionAvail());
    if (ImGui::BeginDragDropTarget()) {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(EditorItem::DRAG_DROP_ENV)) {
            IM_ASSERT(payload->DataSize == sizeof(const char*));
            char* dragged_data = *(char**)payload->Data;
            renderer::request_env_map(dragged_data);

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

    Vector3i delta_camera(0);
    auto& events = m_editor.GetUnhandledEvents();
    bool selected = m_editor.GetSelectedEntity().IsValid();
    float mouse_scroll = 0.0f;

    for (auto& event : events) {
        if (InputEventKey* e = dynamic_cast<InputEventKey*>(event.get()); e) {
            if (e->IsPressed()) {
                switch (e->GetKey()) {
                    case KeyCode::KEY_Z: {
                        if (selected) {
                            m_editor.SetState(EditorLayer::STATE_TRANSLATE);
                        }
                    } break;
                    case KeyCode::KEY_X: {
                        if (selected) {
                            m_editor.SetState(EditorLayer::STATE_ROTATE);
                        }
                    } break;
                    case KeyCode::KEY_C: {
                        if (selected) {
                            m_editor.SetState(EditorLayer::STATE_SCALE);
                        }
                    } break;
                    default:
                        break;
                }
            } else if (e->IsHolding()) {
                switch (e->GetKey()) {
                    case KeyCode::KEY_D:
                        ++delta_camera.x;
                        break;
                    case KeyCode::KEY_A:
                        --delta_camera.x;
                        break;
                    case KeyCode::KEY_E:
                        ++delta_camera.y;
                        break;
                    case KeyCode::KEY_Q:
                        --delta_camera.y;
                        break;
                    case KeyCode::KEY_W:
                        ++delta_camera.z;
                        break;
                    case KeyCode::KEY_S:
                        --delta_camera.z;
                        break;
                    default:
                        break;
                }
            }
        }
        if (InputEventMouseWheel* e = dynamic_cast<InputEventMouseWheel*>(event.get()); e) {
            if (!e->IsModiferPressed()) {
                mouse_scroll = 3.0f * e->GetWheelY();
            }
        }
    }

    if (m_focused) {
        m_cameraController.Move(p_scene.m_elapsedTime, camera, delta_camera, mouse_scroll);
    }

    SelectEntity(p_scene, camera);

    DrawGui(p_scene, camera);
}

}  // namespace my
