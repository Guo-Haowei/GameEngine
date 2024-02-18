#pragma once
#include "editor/editor_window.h"

namespace my {

class HierarchyPanel : public EditorWindow {
public:
    HierarchyPanel(EditorLayer& editor) : EditorWindow("Scene", editor) {}

protected:
    void update_internal(Scene& scene) override;

private:
    void draw_popup(Scene& scene);
};

}  // namespace my
