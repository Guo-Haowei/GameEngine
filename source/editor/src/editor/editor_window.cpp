#include "editor_window.h"

namespace my {

void EditorWindow::update(Scene& scene) {
    if (ImGui::Begin(m_name.c_str())) {
        update_internal(scene);
    }
    ImGui::End();
}

void EditorCompositeWindow::update(Scene& p_scene) {
    for (auto& it : m_windows) {
        it->update(p_scene);
    }
}

void EditorCompositeWindow::add_window(std::shared_ptr<EditorWindow> p_window) {
    DEV_ASSERT(p_window);
    m_windows.emplace_back(p_window);
}

}  // namespace my
