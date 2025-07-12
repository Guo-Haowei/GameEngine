#include "file_system_panel.h"

#include <IconsFontAwesome/IconsFontAwesome6.h>

#include "editor/editor_layer.h"
#include "engine/runtime/asset_registry.h"
#include "engine/runtime/common_dvars.h"
#include "engine/core/string/string_utils.h"

namespace my {

namespace fs = std::filesystem;
//
//class AssetHandler {
//public:
//    virtual ~AssetHandler() = default;
//
//    virtual void DrawTooltip(const fs::path& p_path) {
//
//    }
//
//    virtual void DrawContextMenu(const fs::path& p_path) = 0;
//};
//
//class ImageHandler : AssetHandler {
//public:
//    void DrawTooltip(const fs::path& p_path) override {
//
//    }
//};

FileSystemPanel::FileSystemPanel(EditorLayer& p_editor)
    : EditorWindow("FileSystem", p_editor) {
}

void FileSystemPanel::OnAttach() {
    const auto& path = m_editor.GetApplication()->GetResourceFolder();
    m_root = fs::path{ path };
}

void FileSystemPanel::ShowResourceToolTip(const std::string& p_path) {
    auto asset_registry = m_editor.GetApplication()->GetAssetRegistry();
    auto handle = asset_registry->Request(p_path);

    if (handle.IsReady()) {
        const auto& meta = handle.entry->metadata;
        if (ImGui::BeginTooltip()) {
            ImGui::Text("%s", meta.path.c_str());
            ImGui::Text("type: %s", meta.type.ToString());

            if (meta.type == AssetType::Image) {
                auto texture = static_cast<ImageAsset*>(handle.entry->asset.get());
                DEV_ASSERT(texture);
                const int w = texture->width;
                const int h = texture->height;
                ImGui::Text("Dimension: %d x %d", w, h);

                if (texture->gpu_texture) {
                    float adjusted_w = glm::min(256.f, static_cast<float>(w));
                    float adjusted_h = adjusted_w / w * h;
                    ImGui::Image(texture->gpu_texture->GetHandle(), ImVec2(adjusted_w, adjusted_h));
                }
            }

            ImGui::EndTooltip();
        }
    }
}

void FileSystemPanel::FolderPopup(const std::filesystem::path& p_path) {
    if (ImGui::MenuItem("Rename")) {
        m_renaming = p_path;
    }
    if (ImGui::MenuItem("Delete")) {
        LOG_ERROR("{}", p_path.string());
    }
}

void FileSystemPanel::ListFile(const std::filesystem::path& p_path, const char* p_name_override) {
    if (!fs::exists(p_path)) {
        return;
    }

    const bool is_dir = fs::is_directory(p_path);
    const bool is_file = fs::is_regular_file(p_path);

    std::string filename = p_path.filename().string();
    auto ext = StringUtils::Extension(filename);
    if (ext == ".meta") {
        return;
    }

    const char* icon = is_dir ? ICON_FA_FOLDER : ICON_FA_FILE;

    int flags = 0;
    flags |= ImGuiTreeNodeFlags_DefaultOpen;
    flags |= is_file ? ImGuiTreeNodeFlags_Leaf : 0;

    auto id = std::format("{} ##{}", icon, p_path.string());

    const bool node_open = ImGui::TreeNodeEx(id.c_str(), flags);

    fs::path relative = fs::relative(p_path, m_root);
    std::string short_path = std::format("@res://{}", relative.generic_string());

    if (ImGui::BeginPopupContextItem()) {
        if (is_dir) {
            FolderPopup(p_path);
        }
        ImGui::EndPopup();
    }

    ImGui::SameLine();

    if (m_renaming == p_path) {
        std::string buffer;
        buffer.resize(128);
        if (ImGui::InputText("###rename", buffer.data(), buffer.size(), ImGuiInputTextFlags_EnterReturnsTrue)) {
            m_renaming = "";
        }
        if (!ImGui::IsItemActive() && ImGui::IsMouseClicked(0)) {
            m_renaming = "";
        }
    } else {
        if (p_name_override) {
            ImGui::Text(p_name_override);
        } else {
            ImGui::Text(filename.c_str());
        }
        const bool hovered = ImGui::IsItemHovered();
        if (hovered) {
            if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                LOG_OK("clicked");
                // handle click
            } else if (is_file) {
                ShowResourceToolTip(short_path);
                // handle hover
            }
        }
    }

    if (node_open && is_dir) {
        for (const auto& entry : fs::directory_iterator(p_path)) {
            ListFile(entry.path());
        }
    }
    if (node_open) {
        ImGui::TreePop();
    }
}

void FileSystemPanel::UpdateInternal(Scene&) {
    ListFile(m_root, "@res://");
}

}  // namespace my
