#include "content_browser.h"

#include <IconsFontAwesome/IconsFontAwesome6.h>

#include "editor/editor_layer.h"
#include "editor/panels/panel_util.h"
#include "engine/core/framework/asset_registry.h"
#include "engine/core/framework/common_dvars.h"
#include "engine/core/string/string_utils.h"

namespace my {

namespace fs = std::filesystem;

ContentBrowser::ContentBrowser(EditorLayer& p_editor) : EditorWindow("Content Browser", p_editor) {
}

void ContentBrowser::OnAttach() {
    const auto& path = m_editor.GetApplication()->GetResourceFolder();
    m_rootPath = fs::path{ path };
    m_currentPath = m_rootPath;

    auto asset_registry = m_editor.GetApplication()->GetAssetRegistry();
    auto folder_icon = asset_registry->GetAssetByHandle<Image>("@res://images/icons/folder_icon.png");
    auto image_icon = asset_registry->GetAssetByHandle<Image>("@res://images/icons/image_icon.png");
    auto scene_icon = asset_registry->GetAssetByHandle<Image>("@res://images/icons/scene_icon.png");
    auto meta_icon = asset_registry->GetAssetByHandle<Image>("@res://images/icons/meta_icon.png");

    m_iconMap["."] = { folder_icon, nullptr };
    m_iconMap[".png"] = { image_icon, nullptr };
    m_iconMap[".meta"] = { meta_icon, nullptr };
    m_iconMap[".hdr"] = { image_icon, EditorItem::DRAG_DROP_ENV };
    m_iconMap[".gltf"] = { scene_icon, EditorItem::DRAG_DROP_IMPORT };
    m_iconMap[".obj"] = { scene_icon, EditorItem::DRAG_DROP_IMPORT };
    m_iconMap[".scene"] = { scene_icon, EditorItem::DRAG_DROP_IMPORT };
    m_iconMap[".lua"] = { scene_icon, EditorItem::DRAG_DROP_IMPORT };
}

void ContentBrowser::DrawSideBarHelper(const std::filesystem::path& p_path) {
    if (!fs::exists(p_path)) {
        return;
    }

    for (const auto& entry : fs::directory_iterator(p_path)) {
        const bool is_file = entry.is_regular_file();
        const bool is_dir = entry.is_directory();
        if (!is_dir && !is_file) {
            continue;
        }

        fs::path full_path = entry.path();

        std::string name;
        if (is_dir) {
            name = ICON_FA_FOLDER " ";
            name.append(full_path.stem().string());
        } else {
            name = ICON_FA_FILE " ";
            name = full_path.filename().string();
        }

        if (ImGui::TreeNode(name.c_str())) {
            DrawSideBarHelper(full_path);
            ImGui::TreePop();
        }
    }
}

void ContentBrowser::DrawSideBar() {
    DrawSideBarHelper(m_rootPath);
}

void ContentBrowser::Update(Scene&) {
    if (ImGui::Begin(m_name.c_str())) {
        DrawSideBar();
    }
    ImGui::End();

    if (ImGui::Begin("Assets##ContentBrowser")) {
        DrawAssets();
    }
    ImGui::End();
}

void ContentBrowser::DrawAssets() {
    ImVec2 window_size = ImGui::GetWindowSize();
    constexpr float desired_icon_size = 120.f;
    int num_col = static_cast<int>(glm::floor(window_size.x / desired_icon_size));
    num_col = glm::max(1, num_col);

    ImGui::Columns(num_col, nullptr, false);

    auto registry = m_editor.GetApplication()->GetAssetRegistry();
    std::vector<IAsset*> assets;
    registry->GetAssetByType(AssetType::IMAGE, assets);

    ImVec2 thumbnail_size{ 96.f, 96.f };

    for (const auto& asset : assets) {
        const Image* image = dynamic_cast<const Image*>(asset);
        if (DEV_VERIFY(image)) {
            std::string_view path{ image->meta.path };
            path = StringUtils::FileName(path, '/');
            std::string name{ path };

            [[maybe_unused]] bool clicked = false;

            if (image->gpu_texture) {
                clicked = ImGui::ImageButton(name.c_str(), (ImTextureID)image->gpu_texture->GetHandle(), thumbnail_size);
            } else {
                clicked = ImGui::Button(name.c_str(), thumbnail_size);
            }

            if (true) {
                std::string full_path_string = name;
                char* dragged_data = StringUtils::Strdup(full_path_string.c_str());

                // if (action)
                {
                    ImGuiDragDropFlags flags = ImGuiDragDropFlags_SourceNoDisableHover;
                    if (ImGui::BeginDragDropSource(flags)) {
                        ImGui::SetDragDropPayload("dummy", &dragged_data, sizeof(const char*));
                        ImGui::Text("%s", name.c_str());
                        ImGui::EndDragDropSource();
                    }
                }
            }

            ImGui::Text("%s", name.c_str());
            ImGui::NextColumn();
        }

#if 0
#endif
    }

    ImGui::Columns(1);
}

void ContentBrowser::DrawDetailPanel() {
#if 0
    if (ImGui::Button("<-")) {
        if (m_currentPath != m_rootPath) {
            m_currentPath = m_currentPath.parent_path();
        }
    }

    // ImGui::Image((ImTextureID)handle, ImVec2(dim.x, dim.y), ImVec2(0, 1), ImVec2(1, 0));

    ImVec2 window_size = ImGui::GetWindowSize();
    constexpr float desired_icon_size = 120.f;
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

        bool clicked = false;
        bool has_texture = false;

        if (it->second.image) {
            auto texture = it->second.image->gpu_texture;
            if (texture) {
                clicked = ImGui::ImageButton(name.c_str(), (ImTextureID)texture->GetHandle(), size);
                has_texture = true;
            }
        }
        if (!has_texture) {
            clicked = ImGui::Button(name.c_str(), size);
        }

        if (is_file) {
            std::string full_path_string = full_path.string();
            char* dragged_data = StringUtils::Strdup(full_path_string.c_str());
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

        ImGui::Text("%s", name.c_str());

        if (clicked) {
            if (is_dir) {
                m_currentPath = full_path;
            } else if (is_file) {
            }
        }

        ImGui::NextColumn();
    }

    ImGui::Columns(1);
#endif
}

}  // namespace my
