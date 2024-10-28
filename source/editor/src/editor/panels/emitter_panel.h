#pragma once
#include "editor/editor_window.h"

namespace my {

class EmitterPanel : public EditorWindow {
public:
    EmitterPanel(EditorLayer& editor) : EditorWindow("Emitter", editor) {}

protected:
    void UpdateInternal(Scene& scene) override;
};

}  // namespace my
