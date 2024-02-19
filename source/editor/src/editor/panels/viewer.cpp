#include "viewer.h"

#include <imgui/imgui_internal.h>

#include "core/framework/common_dvars.h"
#include "core/framework/scene_manager.h"
#include "core/input/input.h"
#include "core/math/ray.h"
#include "editor/editor_layer.h"
#include "editor/utility/imguizmo.h"
#include "rendering/rendering_dvars.h"
#include "servers/display_server.h"

namespace my {

Viewer::Viewer(EditorLayer& editor) : EditorWindow("Viewer", editor) {
}

void Viewer::update_data() {
    auto [frame_width, frame_height] = DisplayServer::singleton().get_frame_size();
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

    if (input::is_button_pressed(MOUSE_BUTTON_RIGHT)) {
        auto [window_x, window_y] = DisplayServer::singleton().get_window_pos();
        vec2 clicked = input::get_cursor();
        clicked.x = (clicked.x + window_x - m_canvas_min.x) / m_canvas_size.x;
        clicked.y = (clicked.y + window_y - m_canvas_min.y) / m_canvas_size.y;

        if (clicked.x >= 0.0f && clicked.x <= 1.0f && clicked.y >= 0.0f && clicked.y <= 1.0f) {
            clicked *= 2.0f;
            clicked -= 1.0f;

            const mat4 inversed_projection_view = glm::inverse(camera.get_projection_view_matrix());

            const vec3 ray_start = camera.get_position();
            const vec3 direction = glm::normalize(vec3(inversed_projection_view * vec4(clicked.x, -clicked.y, 1.0f, 1.0f)));
            const vec3 ray_end = ray_start + direction * camera.get_far();
            Ray ray(ray_start, ray_end);

            const auto intersection_result = scene.Intersects(ray);

            m_editor.select_entity(intersection_result.entity);
        }
    }
}

void Viewer::draw_gui(Scene& scene, Camera& camera) {
    const mat4 view_matrix = camera.get_view_matrix();
    const mat4 projection_matrix = camera.get_projection_matrix();
    const mat4 projection_view_matrix = camera.get_projection_view_matrix();

    ImGuizmo::SetOrthographic(false);
    ImGuizmo::BeginFrame();

    ImGuizmo::SetDrawlist();
    ImGuizmo::SetRect(m_canvas_min.x, m_canvas_min.y, m_canvas_size.x, m_canvas_size.y);

    // add image for drawing
    ImVec2 top_left(m_canvas_min.x, m_canvas_min.y);
    ImVec2 bottom_right(top_left.x + m_canvas_size.x, top_left.y + m_canvas_size.y);

    uint64_t final_image = m_editor.get_displayed_image();
    ImGui::GetWindowDrawList()->AddImage((ImTextureID)final_image, top_left, bottom_right, ImVec2(0, 1), ImVec2(1, 0));

    bool draw_grid = DVAR_GET_BOOL(grid_visibility);
    if (draw_grid) {
        mat4 identity(1);
        // draw grid
        ImGuizmo::draw_grid(projection_view_matrix, identity, 10.0f);

        // draw light
        const LightComponent& light = scene.get_component_array<LightComponent>()[0];
        DEV_ASSERT(light.type == LIGHT_TYPE_OMNI);
        if (TransformComponent* transform_component = scene.get_component<TransformComponent>(scene.get_entity<LightComponent>(0)); transform_component) {
            mat4 transform = transform_component->get_world_matrix();
            ImGuizmo::draw_cone_wireframe(projection_view_matrix, transform);
        }
    }

    // draw aabb
    ecs::Entity id = m_editor.get_selected_entity();
    TransformComponent* transform_component = scene.get_component<TransformComponent>(id);
    ObjectComponent* object_component = scene.get_component<ObjectComponent>(id);
    if (object_component && transform_component) {
        auto mesh_component = scene.get_component<MeshComponent>(object_component->mesh_id);
        DEV_ASSERT(mesh_component);
        AABB aabb = mesh_component->local_bound;
        aabb.apply_matrix(transform_component->get_world_matrix());

        const mat4 model_matrix = glm::translate(mat4(1), aabb.center()) * glm::scale(mat4(1), aabb.size());
        ImGuizmo::draw_box_wireframe(projection_view_matrix, model_matrix);
    }

    auto draw_gizmo = [&](ImGuizmo::OPERATION operation) {
        if (transform_component) {
            mat4 local = transform_component->get_local_matrix();
            ImGuizmo::Manipulate(glm::value_ptr(view_matrix),
                                 glm::value_ptr(projection_matrix),
                                 operation,
                                 ImGuizmo::LOCAL,
                                 glm::value_ptr(local),
                                 nullptr, nullptr, nullptr, nullptr);
            transform_component->set_local_transform(local);
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

    update_data();

    if (m_focused) {
        m_camera_controller.move(scene.m_delta_time, camera);
    }

    select_entity(scene, camera);

    // Update state
    if (m_editor.get_selected_entity().is_valid()) {
        if (input::is_key_pressed(KEY_Z)) {
            m_editor.set_state(EditorLayer::STATE_TRANSLATE);
        } else if (input::is_key_pressed(KEY_X)) {
            m_editor.set_state(EditorLayer::STATE_ROTATE);
        } else if (input::is_key_pressed(KEY_C)) {
            m_editor.set_state(EditorLayer::STATE_SCALE);
        }
    }

    draw_gui(scene, camera);
}

}  // namespace my