#include "editor_window.h"

namespace my {

void EditorWindow::Update(Scene& scene) {
    if (ImGui::Begin(m_name.c_str(), nullptr, m_flags)) {
        UpdateInternal(scene);
    }
    ImGui::End();
}

}  // namespace my
