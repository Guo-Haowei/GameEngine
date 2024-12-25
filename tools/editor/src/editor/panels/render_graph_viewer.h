#pragma once
#include "editor/editor_window.h"

namespace my {

class RenderGraphViewer : public EditorWindow {
public:
    RenderGraphViewer(EditorLayer& editor) : EditorWindow("RenderGraph", editor) {}

protected:
    void UpdateInternal(Scene& scene) override;
};

}  // namespace my