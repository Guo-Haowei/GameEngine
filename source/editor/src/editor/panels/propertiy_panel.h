#pragma once
#include "editor/editor_window.h"

namespace my {

class PropertyPanel : public EditorWindow {
public:
    PropertyPanel(EditorLayer& editor) : EditorWindow("Properties", editor) {}

protected:
    void update_internal(Scene& scene) override;
};

}  // namespace my
