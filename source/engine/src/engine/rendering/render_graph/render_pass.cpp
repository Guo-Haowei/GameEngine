#include "render_pass.h"

#include "rendering/opengl/opengl_prerequisites.h"

namespace my::rg {

void RenderPass::add_sub_pass(std::shared_ptr<Subpass> p_subpass) {
    m_subpasses.push_back(p_subpass);
}

void RenderPass::create_internal(RenderPassDesc& desc) {
    m_name = std::move(desc.name);
    m_inputs = std::move(desc.dependencies);
}

void RenderPass::execute() {
    for (auto& subpass : m_subpasses) {
        subpass->func(subpass.get());
    }
}

}  // namespace my::rg
