#include "editor_item.h"

#include "editor/editor_layer.h"

namespace my {

void EditorItem::OpenAddEntityPopup(ecs::Entity p_parent) {
    if (ImGui::BeginMenu("Add")) {
        if (ImGui::BeginMenu("Mesh")) {
            if (ImGui::MenuItem("Plane")) {
                m_editor.AddEntity(EntityType::PLANE, p_parent);
            }
            if (ImGui::MenuItem("Cube")) {
                m_editor.AddEntity(EntityType::CUBE, p_parent);
            }
            if (ImGui::MenuItem("Sphere")) {
                m_editor.AddEntity(EntityType::SPHERE, p_parent);
            }
            if (ImGui::MenuItem("Cylinder")) {
                m_editor.AddEntity(EntityType::CYLINDER, p_parent);
            }
            if (ImGui::MenuItem("Torus")) {
                m_editor.AddEntity(EntityType::TORUS, p_parent);
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Light")) {
            if (ImGui::MenuItem("Point")) {
                m_editor.AddEntity(EntityType::POINT_LIGHT, p_parent);
            }
            if (ImGui::MenuItem("Infinite")) {
                m_editor.AddEntity(EntityType::INFINITE_LIGHT, p_parent);
            }
            if (ImGui::MenuItem("Spot")) {
                LOG_ERROR("not implemented");
            }
            if (ImGui::MenuItem("Area")) {
                m_editor.AddEntity(EntityType::AREA_LIGHT, p_parent);
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Other")) {
            if (ImGui::MenuItem("Transform")) {
                m_editor.AddEntity(EntityType::TRANSFORM, p_parent);
            }
            if (ImGui::MenuItem("Particle Emitter")) {
                m_editor.AddEntity(EntityType::PARTICLE_EMITTER, p_parent);
            }
            if (ImGui::MenuItem("Force Field")) {
                m_editor.AddEntity(EntityType::FORCE_FIELD, p_parent);
            }
            ImGui::EndMenu();
        }
        ImGui::EndMenu();
    }
}

}  // namespace my
