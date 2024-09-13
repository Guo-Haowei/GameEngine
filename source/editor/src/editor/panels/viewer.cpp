#include "viewer.h"

#include <imgui/imgui_internal.h>

#include "core/framework/common_dvars.h"
#include "core/framework/display_manager.h"
#include "core/framework/graphics_manager.h"
#include "core/framework/scene_manager.h"
#include "core/input/input.h"
#include "core/math/ray.h"
#include "editor/editor_layer.h"
#include "editor/utility/imguizmo.h"
#include "rendering/render_manager.h"
#include "rendering/rendering_dvars.h"

namespace my {

void Viewer::update_data() {
    ivec2 frame_size = DVAR_GET_IVEC2(resolution);
    int frame_width = frame_size.x;
    int frame_height = frame_size.y;
    const float ratio = (float)frame_width / frame_height;
    m_canvas_size.x = ImGui::GetWindowSize().x;
    m_canvas_size.y = ImGui::GetWindowSize().y;
    if (m_canvas_size.y * ratio > m_canvas_size.x) {
        m_canvas_size.y = m_canvas_size.x / ratio;
    } else {
        m_canvas_size.x = m_canvas_size.y * ratio;
    }

    ImGuiWindow* window = ImGui::FindWindowByName(m_name.c_str());
    DEV_ASSERT(window);
    m_canvas_min.x = window->ContentRegionRect.Min.x;
    m_canvas_min.y = window->ContentRegionRect.Min.y;

    m_focused = ImGui::IsWindowHovered();
}

void Viewer::select_entity(Scene& scene, const Camera& camera) {
    if (!m_focused) {
        return;
    }

    if (input::isButtonPressed(MOUSE_BUTTON_RIGHT)) {
        auto [window_x, window_y] = DisplayManager::singleton().get_window_pos();
        vec2 clicked = input::getCursor();
        clicked.x = (clicked.x + window_x - m_canvas_min.x) / m_canvas_size.x;
        clicked.y = (clicked.y + window_y - m_canvas_min.y) / m_canvas_size.y;

        if (clicked.x >= 0.0f && clicked.x <= 1.0f && clicked.y >= 0.0f && clicked.y <= 1.0f) {
            clicked *= 2.0f;
            clicked -= 1.0f;

            const mat4 inversed_projection_view = glm::inverse(camera.getProjectionViewMatrix());

            const vec3 ray_start = camera.getPosition();
            const vec3 direction = glm::normalize(vec3(inversed_projection_view * vec4(clicked.x, -clicked.y, 1.0f, 1.0f)));
            const vec3 ray_end = ray_start + direction * camera.getFar();
            Ray ray(ray_start, ray_end);

            const auto result = scene.intersects(ray);
            // const auto result = scene.select(ray);

            m_editor.select_entity(result.entity);
        }
    }
}

void Viewer::draw_gui(Scene& scene, Camera& camera) {
    const mat4 view_matrix = camera.getViewMatrix();
    const mat4 projection_matrix = camera.getProjectionMatrix();
    const mat4 projection_view_matrix = camera.getProjectionViewMatrix();

    ImGuizmo::SetOrthographic(false);
    ImGuizmo::BeginFrame();

    ImGuizmo::SetDrawlist();
    ImGuizmo::SetRect(m_canvas_min.x, m_canvas_min.y, m_canvas_size.x, m_canvas_size.y);

    // add image for drawing
    ImVec2 top_left(m_canvas_min.x, m_canvas_min.y);
    ImVec2 bottom_right(top_left.x + m_canvas_size.x, top_left.y + m_canvas_size.y);

    // @TODO: fix this
    uint64_t handle = GraphicsManager::singleton().get_final_image();

    switch (GraphicsManager::singleton().get_backend()) {
        case Backend::OPENGL:
        case Backend::D3D11:
            ImGui::GetWindowDrawList()->AddImage((ImTextureID)handle, top_left, bottom_right, ImVec2(0, 1), ImVec2(1, 0));
            break;
        default:
            break;
    }

    bool draw_grid = DVAR_GET_BOOL(show_editor);
    if (draw_grid) {
        mat4 identity(1);
        // draw grid
        ImGuizmo::draw_grid(projection_view_matrix, identity, 10.0f);
    }

    ecs::Entity id = m_editor.get_selected_entity();
    TransformComponent* transform_component = scene.getComponent<TransformComponent>(id);
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

            const mat4 model_matrix = glm::translate(mat4(1), aabb.center()) * glm::scale(mat4(1), aabb.size());
            ImGuizmo::draw_box_wireframe(projection_view_matrix, model_matrix);
        }
    }
    ObjectComponent* object_component = scene.get_component<ObjectComponent>(id);
    if (object_component && transform_component) {
        auto mesh_component = scene.get_component<MeshComponent>(object_component->mesh_id);
        DEV_ASSERT(mesh_component);
        AABB aabb = mesh_component->local_bound;
        aabb.apply_matrix(transform_component->get_world_matrix());

        const mat4 model_matrix = glm::translate(mat4(1), aabb.center()) * glm::scale(mat4(1), aabb.size());
        ImGuizmo::draw_box_wireframe(projection_view_matrix, model_matrix);
    }
#endif

    auto draw_gizmo = [&](ImGuizmo::OPERATION operation) {
        if (transform_component) {
            mat4 local = transform_component->getLocalMatrix();
            if (ImGuizmo::Manipulate(glm::value_ptr(view_matrix),
                                     glm::value_ptr(projection_matrix),
                                     operation,
                                     // ImGuizmo::LOCAL,
                                     ImGuizmo::WORLD,
                                     glm::value_ptr(local),
                                     nullptr, nullptr, nullptr, nullptr)) {
                transform_component->setLocalTransform(local);
            }
        }
    };

    // draw gizmo
    switch (m_editor.get_state()) {
        case EditorLayer::STATE_TRANSLATE:
            draw_gizmo(ImGuizmo::TRANSLATE);
            break;
        case EditorLayer::STATE_ROTATE:
            draw_gizmo(ImGuizmo::ROTATE);
            break;
        case EditorLayer::STATE_SCALE:
            draw_gizmo(ImGuizmo::SCALE);
            break;
        default:
            break;
    }

    // draw view cube
    const float size = 120.f;
    ImGuizmo::ViewManipulate((float*)&view_matrix[0].x, 10.0f, ImVec2(m_canvas_min.x, m_canvas_min.y), ImVec2(size, size), IM_COL32(64, 64, 64, 96));
}

void Viewer::update_internal(Scene& scene) {
    Camera& camera = *scene.m_camera;

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
            SceneManager::singleton().request_scene(dragged_data);

            // @TODO: no strdup and free
            free(dragged_data);
        }
        ImGui::EndDragDropTarget();
    }

    update_data();

    if (m_focused) {
        m_camera_controller.move(scene.m_delta_time, camera);
    }

    select_entity(scene, camera);

    // Update state
    if (m_editor.get_selected_entity().isValid()) {
        if (input::isKeyPressed(KEY_Z)) {
            m_editor.set_state(EditorLayer::STATE_TRANSLATE);
        } else if (input::isKeyPressed(KEY_X)) {
            m_editor.set_state(EditorLayer::STATE_ROTATE);
        } else if (input::isKeyPressed(KEY_C)) {
            m_editor.set_state(EditorLayer::STATE_SCALE);
        }
    }

    draw_gui(scene, camera);
}

}  // namespace my