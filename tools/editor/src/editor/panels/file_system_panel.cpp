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

void FileSystemPanel::ListFile(const std::filesystem::path& p_path, const char* p_name_override) {
    if (!fs::exists(p_path)) {
        return;
    }

    const bool is_dir = fs::is_directory(p_path);
    const bool is_file = fs::is_regular_file(p_path);

    int flags = 0;
    flags |= ImGuiTreeNodeFlags_DefaultOpen;
    flags |= is_file ? ImGuiTreeNodeFlags_Leaf : 0;

    std::string name = is_dir ? ICON_FA_FOLDER " " : ICON_FA_FILE " ";
    if (p_name_override) {
        name.append(p_name_override);
    } else {
        name.append(p_path.filename().string());
    }

    if (ImGui::TreeNodeEx(name.c_str(), flags)) {
        if (is_dir) {
            for (const auto& entry : fs::directory_iterator(p_path)) {
                ListFile(entry.path());
            }
        }
        ImGui::TreePop();
    }
}

void FileSystemPanel::UpdateInternal(Scene&) {
    ListFile(m_rootPath, "@res://");
}

}  // namespace my
