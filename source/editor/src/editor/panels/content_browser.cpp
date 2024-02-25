#include "content_browser.h"

#include "core/framework/asset_manager.h"
#include "editor/panels/panel_util.h"

namespace my {

namespace fs = std::filesystem;

ContentBrowser::ContentBrowser(EditorLayer& p_editor) : EditorWindow("Content Browser", p_editor) {
    m_root_path = fs::path{ fs::path(ROOT_FOLDER) / "resources" };
    m_current_path = m_root_path;

    auto folder_icon = AssetManager::singleton().load_image_sync("@res://images/icons/folder_icon.png")->get();
    auto image_icon = AssetManager::singleton().load_image_sync("@res://images/icons/image_icon.png")->get();
    auto scene_icon = AssetManager::singleton().load_image_sync("@res://images/icons/scene_icon.png")->get();

    m_icon_map["."] = folder_icon;
    m_icon_map[".png"] = image_icon;
    m_icon_map[".hdr"] = image_icon;
    m_icon_map[".lua"] = scene_icon;
}

void ContentBrowser::update_internal(Scene&) {
    if (ImGui::Button("<-")) {
        if (m_current_path == m_root_path) {
            LOG_WARN("???");
        }
        m_current_path = m_current_path.parent_path();
    }

    // ImGui::Image((ImTextureID)handle, ImVec2(dim.x, dim.y), ImVec2(0, 1), ImVec2(1, 0));

    ImVec2 window_size = ImGui::GetWindowSize();
    float desired_icon_size = 128.f;
    int num_col = static_cast<int>(glm::floor(window_size.x / desired_icon_size));

    ImGui::Columns(num_col, nullptr, false);

    for (const auto& entry : fs::directory_iterator(m_current_path)) {
        const bool is_file = entry.is_regular_file();
        const bool is_dir = entry.is_directory();
        if (!is_dir && !is_file) {
            continue;
        }

        fs::path full_path = entry.path();
        fs::path relative_path = fs::relative(full_path, m_root_path);
        // std::string relative_path_string = relative_path.string();

        std::string name;
        std::string extention;
        if (is_dir) {
            name = full_path.stem().string();
            extention = ".";
        } else if (is_file) {
            name = full_path.filename().string();
            extention = full_path.extension().string();
        }

        bool clicked = false;
        auto it = m_icon_map.find(extention);
        ImVec2 size{ desired_icon_size, desired_icon_size };
        if (it != m_icon_map.end()) {
            uint64_t handle = it->second->texture.handle;
            clicked = ImGui::ImageButton(name.c_str(), (ImTextureID)handle, size);
        } else {
            clicked = ImGui::Button(name.c_str(), size);
        }

        ImGui::Text(name.c_str());

        ImGui::NextColumn();

        if (clicked) {
            if (is_dir) {
                m_current_path = full_path;
            } else if (is_file) {
            }
        }
    }

    ImGui::Columns(1);
}

}  // namespace my
