#include "render_pass.h"

#include "engine/renderer/base_graphics_manager.h"

// @TODO: refactor
#if 0
#define RT_DEBUG(...) LOG_VERBOSE(__VA_ARGS__)
#else
#define RT_DEBUG(...)
#endif

namespace my::renderer {

void RenderPass::CreateInternal(RenderPassDesc& p_desc) {
    m_name = std::move(p_desc.name);
    m_inputs = std::move(p_desc.dependencies);
}

void RenderPass::Execute(const renderer::RenderData& p_data, IRenderCmdContext& p_cmd) {
    RT_DEBUG("-- Executing pass '{}'", RenderPassNameToString(m_name));

    for (auto& pass : m_drawPasses) {
        auto framebuffer = pass->m_framebuffer.get();
        p_cmd.BeginDrawPass(framebuffer);
        pass->m_executor(p_data, framebuffer, *pass.get(), p_cmd);
        p_cmd.EndDrawPass(framebuffer);
    }

    RT_DEBUG("-------");
}

}  // namespace my::renderer
