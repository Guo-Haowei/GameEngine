#pragma once
#include "editor/editor_window.h"

namespace my {

struct Image;

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

    // @TODO: use FilePath
    std::filesystem::path m_rootPath;
    std::filesystem::path m_currentPath;

    struct ExtensionAction {
        const Image* image;
        const char* action;
    };

    std::map<std::string, ExtensionAction> m_iconMap;
};

}  // namespace my
