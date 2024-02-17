#pragma once
#include "panel.h"

namespace my {

class DebugTexturePanel : public Panel {
public:
    DebugTexturePanel(EditorLayer& editor) : Panel("DebugTexture", editor) {}

protected:
    void update_internal(Scene& scene) override;
};

}  // namespace my
