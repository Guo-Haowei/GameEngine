#include "editor_item.h"

#include "editor/editor_layer.h"

namespace my {

void EditorItem::open_add_entity_popup(ecs::Entity p_parent) {
    if (ImGui::BeginMenu("Add")) {
        if (ImGui::BeginMenu("Mesh")) {
            if (ImGui::MenuItem("Plane")) {
                m_editor.add_entity(ENTITY_TYPE_PLANE, p_parent);
            }
            if (ImGui::MenuItem("Cube")) {
                m_editor.add_entity(ENTITY_TYPE_CUBE, p_parent);
            }
            if (ImGui::MenuItem("Sphere")) {
                m_editor.add_entity(ENTITY_TYPE_SPHERE, p_parent);
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Light")) {
            if (ImGui::MenuItem("Point")) {
                m_editor.add_entity(ENTITY_TYPE_POINT_LIGHT, p_parent);
            }
            if (ImGui::MenuItem("Sun")) {
                LOG_ERROR("not implemented");
            }
            if (ImGui::MenuItem("Spot")) {
                LOG_ERROR("not implemented");
            }
            if (ImGui::MenuItem("Area")) {
                m_editor.add_entity(ENTITY_TYPE_AREA_LIGHT, p_parent);
            }
            ImGui::EndMenu();
        }
        ImGui::EndMenu();
    }
}

}  // namespace my
