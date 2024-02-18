#pragma once
#include "editor/editor_window.h"

namespace my {

class DebugTexturePanel : public EditorWindow {
public:
    DebugTexturePanel(EditorLayer& editor) : EditorWindow("DebugTexture", editor) {}

protected:
    void update_internal(Scene& scene) override;
};

}  // namespace my
