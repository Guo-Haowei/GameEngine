#pragma once
#include "rendering/render_graph/draw_pass.h"
#include "rendering/render_graph/render_graph_defines.h"

namespace my::rg {

struct RenderPassDesc {
    RenderPassName name;
    std::vector<RenderPassName> dependencies;
};

class RenderPass {
public:
    void addDrawPass(std::shared_ptr<DrawPass> p_draw_pass);

    void execute();

    RenderPassName getName() const { return m_name; }
    const char* getNameString() const { return renderPassNameToString(m_name); }

protected:
    virtual void createInternal(RenderPassDesc& pass_desc);

    RenderPassName m_name;
    std::vector<RenderPassName> m_inputs;
    std::vector<std::shared_ptr<DrawPass>> m_draw_passes;

    friend class RenderGraph;
};

}  // namespace my::rg
