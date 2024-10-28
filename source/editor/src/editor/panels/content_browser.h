#pragma once
#include "assets/image.h"
#include "editor/editor_window.h"

namespace my {

class ContentBrowser : public EditorWindow {
public:
    ContentBrowser(EditorLayer& p_editor);
    ~ContentBrowser();

protected:
    void UpdateInternal(Scene& p_scene) override;

    // @TODO: use FilePath
    std::filesystem::path m_rootPath;
    std::filesystem::path m_currentPath;

    struct ExtensionAction {
        Image* image;
        const char* action;
    };
    std::map<std::string, ExtensionAction> m_iconMap;
};

}  // namespace my
