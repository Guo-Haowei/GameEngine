#include "editor_item.h"

#include "editor/editor_layer.h"

namespace my {

void EditorItem::OpenAddEntityPopup(ecs::Entity p_parent) {
    if (ImGui::BeginMenu("Add")) {
        if (ImGui::BeginMenu("Mesh")) {
            if (ImGui::MenuItem("Plane")) {
                m_editor.AddEntity(ENTITY_TYPE_PLANE, p_parent);
            }
            if (ImGui::MenuItem("Cube")) {
                m_editor.AddEntity(ENTITY_TYPE_CUBE, p_parent);
            }
            if (ImGui::MenuItem("Sphere")) {
                m_editor.AddEntity(ENTITY_TYPE_SPHERE, p_parent);
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Light")) {
            if (ImGui::MenuItem("Point")) {
                m_editor.AddEntity(ENTITY_TYPE_POINT_LIGHT, p_parent);
            }
            if (ImGui::MenuItem("Infinite")) {
                m_editor.AddEntity(ENTITY_TYPE_INFINITE_LIGHT, p_parent);
            }
            if (ImGui::MenuItem("Spot")) {
                LOG_ERROR("not implemented");
            }
            if (ImGui::MenuItem("Area")) {
                m_editor.AddEntity(ENTITY_TYPE_AREA_LIGHT, p_parent);
            }
            ImGui::EndMenu();
        }
        ImGui::EndMenu();
    }
}

}  // namespace my
