#pragma once
#include "editor/editor_window.h"

namespace my {

class DebugPanel : public EditorWindow {
public:
    DebugPanel(EditorLayer& editor) : EditorWindow("Debug", editor) {}

protected:
    void update_internal(Scene& scene) override;
};

}  // namespace my
