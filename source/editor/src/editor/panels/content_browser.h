#pragma once
#include "assets/image.h"
#include "editor/editor_window.h"

namespace my {

class ContentBrowser : public EditorWindow {
public:
    ContentBrowser(EditorLayer& p_editor);
    ~ContentBrowser();

protected:
    void update_internal(Scene& p_scene) override;

    std::filesystem::path m_root_path;
    std::filesystem::path m_current_path;

    struct ExtensionAction {
        Image* image;
        const char* action;
    };
    std::map<std::string, ExtensionAction> m_icon_map;
};

}  // namespace my
