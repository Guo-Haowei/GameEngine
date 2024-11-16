#include "render_pass.h"

#include "core/framework/graphics_manager.h"

// @TODO: refactor
#if 0
#define RT_DEBUG(...) LOG_VERBOSE(__VA_ARGS__)
#else
#define RT_DEBUG(...)
#endif

namespace my::rg {

void RenderPass::AddDrawPass(std::shared_ptr<DrawPass> p_draw_pass) {
    m_drawPasses.push_back(p_draw_pass);

    for (auto it : p_draw_pass->colorAttachments) {
        m_outputs.emplace_back(it);
    }
    if (p_draw_pass->depthAttachment) {
        m_outputs.emplace_back(p_draw_pass->depthAttachment);
    }
}

void RenderPass::CreateInternal(RenderPassDesc& p_desc) {
    m_name = std::move(p_desc.name);
    m_inputs = std::move(p_desc.dependencies);
}

void RenderPass::Execute(GraphicsManager& p_graphics_manager) {
    RT_DEBUG("-- Executing pass '{}'", RenderPassNameToString(m_name));

    p_graphics_manager.BeginPass(this);

    for (auto& draw_pass : m_drawPasses) {
        draw_pass->execFunc(draw_pass.get());
    }

    // @TODO: fix this
    // p_graphics_manager.UnsetRenderTarget(m_draw);

    // @TODO: unbound
    p_graphics_manager.EndPass(this);
    // for (auto& it : m_outputs) {
    //     if (it->slot >= 0) {
    //     }
    // }

    RT_DEBUG("-------");
}

}  // namespace my::rg
