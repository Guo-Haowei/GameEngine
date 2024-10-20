#include "render_pass.h"

#include "drivers/opengl/opengl_prerequisites.h"

namespace my::rg {

void RenderPass::addDrawPass(std::shared_ptr<DrawPass> p_draw_pass) {
    m_draw_passes.push_back(p_draw_pass);
}

void RenderPass::createInternal(RenderPassDesc& p_desc) {
    m_name = std::move(p_desc.name);
    m_inputs = std::move(p_desc.dependencies);
}

void RenderPass::execute() {
    for (auto& draw_pass : m_draw_passes) {
        draw_pass->exec_func(draw_pass.get());
    }
}

}  // namespace my::rg
