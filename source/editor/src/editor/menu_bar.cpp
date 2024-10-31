#include "menu_bar.h"

#include "core/framework/common_dvars.h"
#include "core/framework/graphics_manager.h"
#include "core/framework/scene_manager.h"
#include "core/input/input.h"
#include "drivers/windows/dialog.h"
#include "editor/editor_layer.h"
#include "rendering/rendering_dvars.h"

namespace my {

static void SaveProject(bool p_need_open_dialog) {
    const std::string& project = DVAR_GET_STRING(project);

    std::filesystem::path path{ project.empty() ? "untitled.scene" : project.c_str() };
    if (p_need_open_dialog || project.empty()) {
        if (!OpenSaveDialog(path)) {
            return;
        }
    }

    DVAR_SET_STRING(project, path.string());
    Scene& scene = SceneManager::GetSingleton().GetScene();

    Archive archive;
    if (!archive.OpenWrite(path.string())) {
        return;
    }

    if (scene.Serialize(archive)) {
        LOG_OK("scene saved to '{}'", path.string());
    }
}

void MenuBar::Update(Scene&) {
    // @TODO: input system, key s handled here, don't handle it in viewer
    if (input::IsKeyDown(KEY_LEFT_CONTROL)) {
        if (input::IsKeyPressed(KEY_S)) {
            SaveProject(false);
        }
    }

    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Save", "Ctrl+S")) {
                SaveProject(false);
            }
            if (ImGui::MenuItem("Save As..")) {
                SaveProject(true);
            }
            ImGui::EndMenu();
        }
        ImGui::Separator();
        if (ImGui::BeginMenu("Edit")) {
            if (ImGui::MenuItem("Undo", "CTRL+Z")) {
            }
            if (ImGui::MenuItem("Redo", "CTRL+Y", false, false)) {
            }  // Disabled item
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
