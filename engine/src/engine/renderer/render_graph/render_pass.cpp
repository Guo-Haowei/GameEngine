#include "render_pass.h"

#include "engine/core/framework/graphics_manager.h"

// @TODO: refactor
#if 0
#define RT_DEBUG(...) LOG_VERBOSE(__VA_ARGS__)
#else
#define RT_DEBUG(...)
#endif

namespace my::renderer {

void RenderPass::AddDrawPass(std::shared_ptr<Framebuffer> p_framebuffer) {
    m_drawPasses.push_back(p_framebuffer);
}

void RenderPass::CreateInternal(RenderPassDesc& p_desc) {
    m_name = std::move(p_desc.name);
    m_inputs = std::move(p_desc.dependencies);
}

void RenderPass::Execute(const renderer::DrawData& p_data, GraphicsManager& p_graphics_manager) {
    RT_DEBUG("-- Executing pass '{}'", RenderPassNameToString(m_name));

    for (auto& framebuffer : m_drawPasses) {
        p_graphics_manager.BeginDrawPass(framebuffer.get());
        framebuffer->desc.execFunc(p_data, framebuffer.get());
        p_graphics_manager.EndDrawPass(framebuffer.get());
    }

    RT_DEBUG("-------");
}

}  // namespace my::renderer
