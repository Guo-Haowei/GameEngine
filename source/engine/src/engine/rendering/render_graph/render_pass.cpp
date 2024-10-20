#include "render_pass.h"

#include "drivers/opengl/opengl_prerequisites.h"

namespace my::rg {

void RenderPass::addSubpass(std::shared_ptr<Subpass> p_subpass) {
    m_subpasses.push_back(p_subpass);
}

void RenderPass::createInternal(RenderPassDesc& p_desc) {
    m_name = std::move(p_desc.name);
    m_inputs = std::move(p_desc.dependencies);
}

void RenderPass::execute() {
    for (auto& subpass : m_subpasses) {
        subpass->exec_func(subpass.get());
    }
}

}  // namespace my::rg
