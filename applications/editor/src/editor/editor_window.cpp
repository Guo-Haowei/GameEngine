#include "editor_window.h"

namespace my {

void EditorWindow::Update(Scene& scene) {
    if (ImGui::Begin(m_name.c_str())) {
        UpdateInternal(scene);
    }
    ImGui::End();
}

}  // namespace my
