#pragma once
#include "editor/editor_window.h"

namespace my {

struct ImageAsset;

class ContentBrowser : public EditorWindow {
public:
    ContentBrowser(EditorLayer& p_editor);

    void OnAttach() override;

    void Update(Scene&) override;

protected:
    void UpdateInternal(Scene&) override {}

    void DrawSideBarHelper(const std::filesystem::path& p_path);
    void DrawSideBar();
    void DrawDetailPanel();
    void DrawAssets();

    // @TODO: use FilePath
    std::filesystem::path m_rootPath;
    std::filesystem::path m_currentPath;

    struct ExtensionAction {
        const ImageAsset* image;
        const char* action;
    };

    std::map<std::string, ExtensionAction> m_iconMap;
    AssetType m_assetType{ AssetType::NONE };
};

}  // namespace my
