#pragma once
#include "engine/renderer/render_graph/framebuffer.h"
#include "engine/renderer/render_graph/render_graph_defines.h"

namespace my {
class GraphicsManager;
}  // namespace my

namespace my::renderer {

struct DrawData;

using DrawPassExecuteFunc = void (*)(const renderer::DrawData&, const Framebuffer*);

struct RenderPassDesc {
    RenderPassName name;
    std::vector<RenderPassName> dependencies;
};

class RenderPass {
public:
    void AddDrawPass(std::shared_ptr<Framebuffer> p_framebuffer, DrawPassExecuteFunc p_function);

    void Execute(const renderer::DrawData& p_data, GraphicsManager& p_graphics_manager);

    RenderPassName GetName() const { return m_name; }
    const char* GetNameString() const { return RenderPassNameToString(m_name); }

    const auto& GetDrawPasses() const { return m_drawPasses; }
protected:
    void CreateInternal(RenderPassDesc& pass_desc);

    struct DrawPass {
        std::shared_ptr<Framebuffer> framebuffer;
        DrawPassExecuteFunc executor;
    };

    RenderPassName m_name;
    std::vector<RenderPassName> m_inputs;
    std::vector<DrawPass> m_drawPasses;

    friend class RenderGraph;
};

}  // namespace my::renderer
