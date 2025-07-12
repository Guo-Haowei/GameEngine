#pragma once
#include "editor/editor_window.h"

namespace my {

struct IAsset;
struct AssetMetaData;

class FileSystemPanel : public EditorWindow {
public:
    FileSystemPanel(EditorLayer& p_editor);

    void OnAttach() override;

protected:
    void UpdateInternal(Scene&) override;

    void ListFile(const std::filesystem::path& p_path, const char* p_name_override = nullptr);

    void ShowResourceToolTip(const AssetMetaData& p_meta, const IAsset& p_asset);

    void FolderPopup(const std::filesystem::path& p_path, bool p_is_dir);

    std::filesystem::path m_root;
    std::filesystem::path m_renaming;
};

}  // namespace my
