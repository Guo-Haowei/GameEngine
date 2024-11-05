#include "editor_item.h"

#include "editor/editor_layer.h"

namespace my {

void EditorItem::OpenAddEntityPopup(ecs::Entity p_parent) {
    if (ImGui::BeginMenu("Add")) {
#define ENTITY_TYPE(ENUM, NAME, SEP)                    \
    if (ImGui::MenuItem(#NAME)) {                       \
        m_editor.AddEntity(EntityType::ENUM, p_parent); \
    }                                                   \
    if constexpr (SEP) {                                \
        ImGui::Separator();                             \
    }
        ENTITY_TYPE_LIST
#undef ENTITY_TYPE
        ImGui::EndMenu();
    }
}

}  // namespace my
