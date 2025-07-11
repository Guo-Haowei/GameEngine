#include "render_pass.h"

#include "engine/renderer/graphics_manager.h"

// @TODO: refactor
#if 0
#define RT_DEBUG(...) LOG_VERBOSE(__VA_ARGS__)
#else
#define RT_DEBUG(...)
#endif

namespace my {

void RenderPass::Execute(const FrameData& p_data, IRenderCmdContext& p_cmd) {
    RT_DEBUG("-- Executing pass '{}'", m_name);

    auto framebuffer = m_framebuffer.get();
    RenderPassExcutionContext ctx{
        .frameData = p_data,
        .framebuffer = framebuffer,
        .pass = *this,
        .cmd = p_cmd,
    };

    // bind srvs
    for (int i = 0; i < (int)m_srvs.size(); ++i) {
        const GpuTexture* srv = m_srvs[i].get();
        p_cmd.BindTexture(srv->desc.dimension, srv->GetHandle(), i);
    }
    // bind uavs
    for (int i = 0; i < (int)m_uavs.size(); ++i) {
        GpuTexture* uav = m_uavs[i].get();
        p_cmd.BindUnorderedAccessView(i, uav);
    }

    p_cmd.BeginDrawPass(framebuffer);

    m_executor(ctx);

    p_cmd.EndDrawPass(framebuffer);

    // unbind srvs
    for (int i = 0; i < (int)m_srvs.size(); ++i) {
        const GpuTexture* srv = m_srvs[i].get();
        p_cmd.UnbindTexture(srv->desc.dimension, i);
    }
    // unbind uavs
    for (int i = 0; i < (int)m_uavs.size(); ++i) {
        p_cmd.UnbindUnorderedAccessView(i);
    }

    RT_DEBUG("-------");
}

}  // namespace my
