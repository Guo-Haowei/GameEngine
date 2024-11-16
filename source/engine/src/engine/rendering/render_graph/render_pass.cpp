#include "render_pass.h"

namespace my::rg {

void RenderPass::AddDrawPass(std::shared_ptr<DrawPass> p_draw_pass) {
    m_drawPasses.push_back(p_draw_pass);
}

void RenderPass::CreateInternal(RenderPassDesc& p_desc) {
    m_name = std::move(p_desc.name);
    m_inputs = std::move(p_desc.dependencies);
}

void RenderPass::Execute() {
    for (auto& draw_pass : m_drawPasses) {
        draw_pass->execFunc(draw_pass.get());
    }
}

}  // namespace my::rg
