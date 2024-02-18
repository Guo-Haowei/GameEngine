#include "editor_window.h"

namespace my {

void EditorWindow::update(Scene& scene) {
    if (ImGui::Begin(m_name.c_str())) {
        update_internal(scene);
    }
    ImGui::End();
}

}  // namespace my
