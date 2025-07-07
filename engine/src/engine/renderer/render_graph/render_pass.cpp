#include "render_pass.h"

#include "engine/renderer/base_graphics_manager.h"

// @TODO: refactor
#if 0
#define RT_DEBUG(...) LOG_VERBOSE(__VA_ARGS__)
#else
#define RT_DEBUG(...)
#endif

namespace my::renderer {

void RenderPass::Execute(const renderer::RenderSystem& p_data, IRenderCmdContext& p_cmd) {
    RT_DEBUG("-- Executing pass '{}'", m_name);

    auto framebuffer = m_framebuffer.get();
    RenderPassExcutionContext ctx{
        .render_system = p_data,
        .framebuffer = framebuffer,
        .pass = *this,
        .cmd = p_cmd,
    };

    p_cmd.BeginDrawPass(framebuffer);
    m_executor(ctx);
    p_cmd.EndDrawPass(framebuffer);
    m_commands.clear();

    RT_DEBUG("-------");
}

}  // namespace my::renderer
