#pragma once
#include "editor/editor_window.h"

namespace my {

class ContentBrowser : public EditorWindow {
public:
    ContentBrowser(EditorLayer& editor) : EditorWindow("Content Browser", editor) {}

protected:
    void update_internal(Scene& scene) override;
};

}  // namespace my
