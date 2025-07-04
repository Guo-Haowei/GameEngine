#include "menu_bar.h"

#include "editor/editor_layer.h"
#include "engine/renderer/base_graphics_manager.h"
#include "engine/runtime/input_manager.h"

namespace my {

void MenuBar::Update(Scene&) {
    const auto& shortcuts = m_editor.GetShortcuts();
    auto build_menu_item = [&](int p_index) {
        const auto& it = shortcuts[p_index];
        const bool enabled = it.enabledFunc ? it.enabledFunc() : true;
        if (ImGui::MenuItem(it.name, it.shortcut, false, enabled)) {
            it.executeFunc();
        }
    };

    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            build_menu_item(SHORT_CUT_OPEN);
            // Open Recent
            ImGui::Separator();
            build_menu_item(SHORT_CUT_SAVE);
            build_menu_item(SHORT_CUT_SAVE_AS);
            ImGui::EndMenu();
        }
        ImGui::Separator();
        if (ImGui::BeginMenu("Edit")) {
            build_menu_item(SHORT_CUT_UNDO);
            build_menu_item(SHORT_CUT_REDO);
            ImGui::Separator();
            if (ImGui::MenuItem("Cut", "Ctrl+X")) {
            }
            if (ImGui::MenuItem("Copy", "Ctrl+C")) {
            }
            if (ImGui::MenuItem("Paste", "Ctrl+V")) {
            }
            ImGui::EndMenu();
        }
        ImGui::Separator();
        EditorItem::OpenAddEntityPopup(ecs::Entity::INVALID);
        ImGui::EndMenuBar();
    }
}

}  // namespace my
