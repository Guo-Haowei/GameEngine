#include "file_system_panel.h"

#include <IconsFontAwesome/IconsFontAwesome6.h>

#include "editor/editor_layer.h"
#include "engine/runtime/asset_registry.h"
#include "engine/runtime/common_dvars.h"
#include "engine/core/string/string_utils.h"

namespace my {

namespace fs = std::filesystem;

FileSystemPanel::FileSystemPanel(EditorLayer& p_editor) : EditorWindow("FileSystem", p_editor) {
}

void FileSystemPanel::OnAttach() {
    const auto& path = m_editor.GetApplication()->GetResourceFolder();
    m_rootPath = fs::path{ path };
}

void FileSystemPanel::DrawSideBarHelper(const std::filesystem::path& p_path) {
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
        auto stem = full_path.stem().string();

        std::string name;
        if (is_dir) {
            name = ICON_FA_FOLDER " ";
            name.append(stem);
        } else {
            name = ICON_FA_FILE " ";
            name = full_path.filename().string();
        }

        if (ImGui::TreeNode(name.c_str())) {
            if (is_dir) {
                DrawSideBarHelper(full_path);
            }
            ImGui::TreePop();
        }
    }
}

void FileSystemPanel::DrawSideBar() {
    for (const auto& entry : fs::directory_iterator(m_rootPath)) {
        const bool is_dir = entry.is_directory();
        if (!is_dir) {
            continue;
        }
        fs::path full_path = entry.path();
        auto stem = full_path.stem().string();
        bool should_skip = true;
        AssetType type = AssetType::NONE;
        if (stem == "images") {
            should_skip = false;
            type = AssetType::IMAGE;
        }
        if (stem == "scripts") {
            should_skip = false;
            type = AssetType::TEXT;
        }
        if (should_skip) {
            continue;
        }

        std::string name;
        if (is_dir) {
            name = ICON_FA_FOLDER " ";
            name.append(stem);
        } else {
            name = ICON_FA_FILE " ";
            name = full_path.filename().string();
        }

        if (ImGui::TreeNode(name.c_str())) {
            if (is_dir) {
                DrawSideBarHelper(full_path);
            }
            ImGui::TreePop();
        }
    }
}

void FileSystemPanel::UpdateInternal(Scene&) {
    int flags = ImGuiTableFlags_Resizable;
    flags |= ImGuiTableFlags_NoBordersInBody;
    if (ImGui::BeginTable("Outter", 2, flags)) {
        ImGui::TableNextColumn();
        DrawSideBar();
        ImGui::TableNextColumn();
        DrawAssets();
        ImGui::EndTable();
    }
}

void FileSystemPanel::DrawAssets() {
    ImVec2 window_size = ImGui::GetContentRegionAvail();
    constexpr float desired_icon_size = 120.f;
    int num_col = static_cast<int>(glm::floor(window_size.x / desired_icon_size));
    num_col = glm::max(1, num_col);

    ImGui::BeginTable("Inner", num_col);
    ImGui::TableNextColumn();

    auto registry = m_editor.GetApplication()->GetAssetRegistry();
    ImVec2 thumbnail_size{ 96.f, 96.f };

    auto list_image_assets = [&]() {
        std::vector<IAsset*> assets;
        registry->GetAssetByType(AssetType::IMAGE, assets);

        for (const auto& asset : assets) {
            const ImageAsset* image = dynamic_cast<const ImageAsset*>(asset);
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
                ImGui::TableNextColumn();
            }
        }
    };

    ImGui::EndTable();
}

}  // namespace my
