#include "editor_item.h"

#include "editor/editor_layer.h"

namespace my {

void EditorItem::open_add_entity_popup(ecs::Entity parent) {
    if (ImGui::BeginMenu("Add")) {
        if (ImGui::BeginMenu("Mesh")) {
            if (ImGui::MenuItem("Plane")) {
                LOG_ERROR("not implemented");
            }
            if (ImGui::MenuItem("Cube")) {
                m_editor.add_entity(ENTITY_TYPE_CUBE, parent);
            }
            if (ImGui::MenuItem("Sphere")) {
                LOG_ERROR("not implemented");
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Light")) {
            if (ImGui::MenuItem("Point")) {
                m_editor.add_entity(ENTITY_TYPE_POINT_LIGHT, parent);
            }
            if (ImGui::MenuItem("Sun")) {
                LOG_ERROR("not implemented");
            }
            if (ImGui::MenuItem("Spot")) {
                LOG_ERROR("not implemented");
            }
            if (ImGui::MenuItem("Area")) {
                LOG_ERROR("not implemented");
            }
            ImGui::EndMenu();
        }
        ImGui::EndMenu();
    }
}

}  // namespace my
