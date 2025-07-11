#pragma once
#include "editor/editor_window.h"

namespace my {

struct ImageAsset;

class FileSystemPanel : public EditorWindow {
public:
    FileSystemPanel(EditorLayer& p_editor);

    void OnAttach() override;

protected:
    void UpdateInternal(Scene&) override;

    void DrawSideBarHelper(const std::filesystem::path& p_path);
    void DrawSideBar();
    void DrawAssets();

    // @TODO: use FilePath
    std::filesystem::path m_rootPath;
};

}  // namespace my
