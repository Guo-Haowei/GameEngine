#pragma once
#include "editor/editor_window.h"
#include "engine/core/base/graph.h"
#include "engine/renderer/graphics_defines.h"

namespace my::renderer {
class RenderPass;
}

namespace my {

class RenderGraphViewer : public EditorWindow {
public:
    RenderGraphViewer(EditorLayer& p_editor);

protected:
    void UpdateInternal(Scene& p_scene) override;
    void DrawNodes(const Graph<renderer::RenderPass*> p_graph);

    bool m_firstFrame{ true };
    Backend m_backend{ Backend::COUNT };
};

}  // namespace my