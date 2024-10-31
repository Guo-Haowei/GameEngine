#include "content_browser.h"

#include "core/framework/asset_manager.h"
#include "core/framework/common_dvars.h"
#include "editor/panels/panel_util.h"

namespace my {

namespace fs = std::filesystem;

ContentBrowser::ContentBrowser(EditorLayer& p_editor) : EditorWindow("Content Browser", p_editor) {
    m_rootPath = fs::path{ fs::path(ROOT_FOLDER) / "resources" };

    const std::string& cached_path = DVAR_GET_STRING(content_browser_path);
    if (!cached_path.empty() && fs::exists(cached_path)) {
        m_currentPath = cached_path;
    } else {
        m_currentPath = m_rootPath;
    }

    auto folder_icon = AssetManager::GetSingleton().LoadImageSync(FilePath{ "@res://images/icons/folder_icon.png" })->Get();
    auto image_icon = AssetManager::GetSingleton().LoadImageSync(FilePath{ "@res://images/icons/image_icon.png" })->Get();
    auto scene_icon = AssetManager::GetSingleton().LoadImageSync(FilePath{ "@res://images/icons/scene_icon.png" })->Get();

    m_iconMap["."] = { folder_icon, nullptr };
    m_iconMap[".png"] = { image_icon, nullptr };
    m_iconMap[".hdr"] = { image_icon, EditorItem::DRAG_DROP_ENV };
    m_iconMap[".gltf"] = { scene_icon, EditorItem::DRAG_DROP_IMPORT };
    m_iconMap[".obj"] = { scene_icon, EditorItem::DRAG_DROP_IMPORT };
    m_iconMap[".scene"] = { scene_icon, EditorItem::DRAG_DROP_IMPORT };
    m_iconMap[".lua"] = { scene_icon, EditorItem::DRAG_DROP_IMPORT };
}

ContentBrowser::~ContentBrowser() {
    DVAR_SET_STRING(content_browser_path, m_currentPath.string());
}

void ContentBrowser::UpdateInternal(Scene&) {
    if (ImGui::Button("<-")) {
        if (m_currentPath == m_rootPath) {
            LOG_WARN("TODO: don't go outside project dir");
        }
        m_currentPath = m_currentPath.parent_path();
    }

    // ImGui::Image((ImTextureID)handle, ImVec2(dim.x, dim.y), ImVec2(0, 1), ImVec2(1, 0));

    ImVec2 window_size = ImGui::GetWindowSize();
    float desired_icon_size = 128.f;
    int num_col = static_cast<int>(glm::floor(window_size.x / desired_icon_size));
    num_col = glm::max(1, num_col);

    ImGui::Columns(num_col, nullptr, false);

    for (const auto& entry : fs::directory_iterator(m_currentPath)) {
        const bool is_file = entry.is_regular_file();
        const bool is_dir = entry.is_directory();
        if (!is_dir && !is_file) {
            continue;
        }

        fs::path full_path = entry.path();
        fs::path relative_path = fs::relative(full_path, m_rootPath);
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

        auto it = m_iconMap.find(extention);
        ImVec2 size{ desired_icon_size, desired_icon_size };
        if (it == m_iconMap.end()) {
            continue;
        }

        auto texture = it->second.image->gpu_texture;
        bool clicked = false;
        if (texture) {
            clicked = ImGui::ImageButton(name.c_str(), (ImTextureID)texture->GetHandle(), size);
        } else {
            clicked = ImGui::Button(name.c_str(), size);
        }

        if (is_file) {
            std::string full_path_string = full_path.string();
            char* dragged_data = _strdup(full_path_string.c_str());

            const char* action = it->second.action;

            if (action) {
                ImGuiDragDropFlags flags = ImGuiDragDropFlags_SourceNoDisableHover;
                if (ImGui::BeginDragDropSource(flags)) {
                    ImGui::SetDragDropPayload(action, &dragged_data, sizeof(const char*));
                    ImGui::Text("%s", name.c_str());
                    ImGui::EndDragDropSource();
                }
            }
        }

        ImGui::Text(name.c_str());

        if (clicked) {
            if (is_dir) {
                m_currentPath = full_path;
            } else if (is_file) {
            }
        }

        ImGui::NextColumn();
    }

    ImGui::Columns(1);
}

}  // namespace my
