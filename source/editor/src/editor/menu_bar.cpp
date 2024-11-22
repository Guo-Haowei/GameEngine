#include "menu_bar.h"

#include "core/framework/graphics_manager.h"
#include "core/framework/input_manager.h"
#include "editor/editor_layer.h"

namespace my {

void MenuBar::Update(Scene&) {
    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Save", "Ctrl+S")) {
                m_editor.BufferCommand(std::make_shared<SaveProjectCommand>(false));
            }
            if (ImGui::MenuItem("Save As..")) {
                m_editor.BufferCommand(std::make_shared<SaveProjectCommand>(true));
            }
            ImGui::EndMenu();
        }
        ImGui::Separator();
        if (ImGui::BeginMenu("Edit")) {
            auto& undo_stack = m_editor.GetUndoStack();
            if (ImGui::MenuItem("Undo", "CTRL+Z", false, undo_stack.CanUndo())) {
                m_editor.BufferCommand(std::make_shared<UndoViewerCommand>());
            }
            if (ImGui::MenuItem("Redo", "CTRL+Y", false, undo_stack.CanRedo())) {
                m_editor.BufferCommand(std::make_shared<RedoViewerCommand>());
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Cut", "CTRL+X")) {
            }
            if (ImGui::MenuItem("copy", "CTRL+C")) {
            }
            if (ImGui::MenuItem("Paste", "CTRL+V")) {
            }
            ImGui::EndMenu();
        }
        ImGui::Separator();
        EditorItem::OpenAddEntityPopup(ecs::Entity::INVALID);
        ImGui::EndMenuBar();
    }
}

}  // namespace my
