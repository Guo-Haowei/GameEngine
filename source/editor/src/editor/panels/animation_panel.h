#pragma once
#include "editor/editor_window.h"

namespace my {

class AnimationPanel : public EditorWindow {
public:
    AnimationPanel(EditorLayer& editor) : EditorWindow("Animation", editor) {}

protected:
    void update_internal(Scene& scene) override;
};

}  // namespace my
