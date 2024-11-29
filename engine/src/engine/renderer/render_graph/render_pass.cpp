#include "render_pass.h"

#include "engine/core/framework/graphics_manager.h"

// @TODO: refactor
#if 0
#define RT_DEBUG(...) LOG_VERBOSE(__VA_ARGS__)
#else
#define RT_DEBUG(...)
#endif

namespace my::renderer {

void RenderPass::AddDrawPass(std::shared_ptr<DrawPass> p_draw_pass) {
    m_drawPasses.push_back(p_draw_pass);
}

void RenderPass::CreateInternal(RenderPassDesc& p_desc) {
    m_name = std::move(p_desc.name);
    m_inputs = std::move(p_desc.dependencies);
}

void RenderPass::Execute(const renderer::RenderData& p_data, GraphicsManager& p_graphics_manager) {
    RT_DEBUG("-- Executing pass '{}'", RenderPassNameToString(m_name));

    for (auto& draw_pass : m_drawPasses) {
        p_graphics_manager.BeginDrawPass(draw_pass.get());
        draw_pass->desc.execFunc(p_data, draw_pass.get());
        p_graphics_manager.EndDrawPass(draw_pass.get());
    }

    RT_DEBUG("-------");
}

}  // namespace my::renderer
