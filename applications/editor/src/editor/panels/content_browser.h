#pragma once
#include "editor/editor_window.h"
#include "engine/assets/image.h"

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
