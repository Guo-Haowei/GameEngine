#pragma once
#include "engine/renderer/render_graph/draw_pass.h"
#include "engine/renderer/render_graph/render_graph_defines.h"

namespace my {
class GraphicsManager;
}  // namespace my

namespace my::renderer {

struct DrawData;

struct RenderPassDesc {
    RenderPassName name;
    std::vector<RenderPassName> dependencies;
};

class RenderPass {
public:
    void AddDrawPass(std::shared_ptr<DrawPass> p_draw_pass);

    void Execute(const renderer::DrawData& p_data, GraphicsManager& p_graphics_manager);

    RenderPassName GetName() const { return m_name; }
    const char* GetNameString() const { return RenderPassNameToString(m_name); }

protected:
    void CreateInternal(RenderPassDesc& pass_desc);

    RenderPassName m_name;
    std::vector<RenderPassName> m_inputs;
    std::vector<std::shared_ptr<DrawPass>> m_drawPasses;

    friend class RenderGraph;
};

}  // namespace my::renderer
