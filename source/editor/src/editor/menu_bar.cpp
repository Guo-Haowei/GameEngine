#include "menu_bar.h"

#include "core/framework/common_dvars.h"
#include "core/framework/graphics_manager.h"
#include "core/framework/scene_manager.h"
#include "core/input/input.h"
#include "editor/editor_layer.h"
#include "platform/windows/dialog.h"
#include "rendering/render_graph/render_graph_vxgi.h"
#include "rendering/rendering_dvars.h"

namespace my {

// @TODO: fix this
static std::vector<std::string> quick_dirty_split(std::string str, std::string token) {
    std::vector<std::string> result;
    while (str.size()) {
        size_t index = str.find(token);
        if (index != std::string::npos) {
            result.push_back(str.substr(0, index));
            str = str.substr(index + token.size());
            if (str.size() == 0) result.push_back(str);
        } else {
            result.push_back(str);
            str = "";
        }
    }
    return result;
}

static void import_scene(ImporterName importer) {
    std::vector<const char*> filters = { ".gltf", ".obj" };
    auto path = open_file_dialog(filters);

    if (path.empty()) {
        return;
    }

    SceneManager::singleton().request_scene(path, importer);

    std::string files(DVAR_GET_STRING(recent_files));
    if (!files.empty()) {
        files.append(";");
    }
    files.append(path);

    DVAR_SET_STRING(recent_files, files);
}

static void import_recent(ImporterName importer) {
    std::string recent_files(DVAR_GET_STRING(recent_files));

    auto files = quick_dirty_split(recent_files, ";");

    for (const auto& file : files) {
        if (ImGui::MenuItem(file.c_str())) {
            SceneManager::singleton().request_scene(file, importer);
        }
    }
}

static void save_project(bool open_dialog) {
    const std::string& project = DVAR_GET_STRING(project);

    std::filesystem::path path{ project.empty() ? "untitled.scene" : project.c_str() };
    if (open_dialog || project.empty()) {
        if (!open_save_dialog(path)) {
            return;
        }
    }

    DVAR_SET_STRING(project, path.string());
    Scene& scene = SceneManager::singleton().get_scene();

    Archive archive;
    if (!archive.open_write(path.string())) {
        return;
    }

    if (scene.serialize(archive)) {
        LOG_OK("scene saved to '{}'", path.string());
    }
}

static uint64_t get_displayed_image(int e) {
    auto& rg = GraphicsManager::singleton().get_active_render_graph();
    switch (e) {
        case DISPLAY_FXAA_IMAGE:
            return rg.find_resouce(RT_RES_FXAA)->get_handle();
        case DISPLAY_GBUFFER_DEPTH:
            return rg.find_resouce(RT_RES_GBUFFER_DEPTH)->get_handle();
        case DISPLAY_GBUFFER_BASE_COLOR:
            return rg.find_resouce(RT_RES_GBUFFER_BASE_COLOR)->get_handle();
        case DISPLAY_GBUFFER_NORMAL:
            return rg.find_resouce(RT_RES_GBUFFER_NORMAL)->get_handle();
        case DISPLAY_SSAO:
            return rg.find_resouce(RT_RES_SSAO)->get_handle();
        case DISPLAY_SHADOW_MAP:
            return rg.find_resouce(RT_RES_SHADOW_MAP)->get_handle();
        default:
            return 0;
    }
}

void MenuBar::draw(Scene&) {
    if (m_editor.get_displayed_image() == 0) {
        m_editor.set_displayed_image(get_displayed_image(DVAR_GET_INT(r_debug_texture)));
    }

    // @TODO: input system, key s handled here, don't handle it in viewer
    if (input::is_key_down(KEY_LEFT_CONTROL)) {
        if (input::is_key_pressed(KEY_S)) {
            save_project(false);
        }
    }

    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Save", "Ctrl+S")) {
                save_project(false);
            }
            if (ImGui::MenuItem("Save As..")) {
                save_project(true);
            }
            ImGui::EndMenu();
        }
        ImGui::Separator();
        if (ImGui::BeginMenu("Import")) {
            if (ImGui::MenuItem("Import (Assimp)")) {
                import_scene(IMPORTER_ASSIMP);
            }
            if (ImGui::BeginMenu("Import Recent (Assimp)")) {
                import_recent(IMPORTER_ASSIMP);
                ImGui::EndMenu();
            }
            if (ImGui::MenuItem("Import (TinyGLTF)")) {
                import_scene(IMPORTER_TINYGLTF);
            }
            if (ImGui::BeginMenu("Import Recent (TinyGLTF)")) {
                import_recent(IMPORTER_TINYGLTF);
                ImGui::EndMenu();
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
        EditorItem::open_add_entity_popup(ecs::Entity::INVALID);
        ImGui::EndMenuBar();
    }
}

}  // namespace my
