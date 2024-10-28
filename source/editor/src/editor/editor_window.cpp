#include "editor_window.h"

namespace my {

void EditorWindow::Update(Scene& scene) {
    if (ImGui::Begin(m_name.c_str())) {
        UpdateInternal(scene);
    }
    ImGui::End();
}

void EditorCompositeWindow::Update(Scene& p_scene) {
    for (auto& it : m_windows) {
        it->Update(p_scene);
    }
}

void EditorCompositeWindow::AddWindow(std::shared_ptr<EditorWindow> p_window) {
    DEV_ASSERT(p_window);
    m_windows.emplace_back(p_window);
}

}  // namespace my
