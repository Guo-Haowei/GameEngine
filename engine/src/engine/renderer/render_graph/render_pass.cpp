#include "render_pass.h"

#include "engine/renderer/base_graphics_manager.h"

// @TODO: refactor
#if 0
#define RT_DEBUG(...) LOG_VERBOSE(__VA_ARGS__)
#else
#define RT_DEBUG(...)
#endif

namespace my::renderer {

void RenderPass::AddDrawPass(std::string_view p_name,
                             std::shared_ptr<Framebuffer> p_framebuffer,
                             DrawPassExecuteFunc p_func) {
    m_lookup.insert(std::make_pair(p_name, static_cast<uint32_t>(m_drawPasses.size())));

    m_drawPasses.emplace_back(DrawPass{ std::string(p_name), p_framebuffer, p_func, {} });
}

auto RenderPass::FindDrawPass(const std::string& p_name) -> Result<DrawPass*> {
    auto it = m_lookup.find(p_name);
    if (it == m_lookup.end()) {
        return HBN_ERROR(ErrorCode::ERR_DOES_NOT_EXIST, "DrawPass '{}' doesn exist", p_name);
    }

    const uint32_t idx = it->second;
    if (idx >= m_drawPasses.size()) {
        return HBN_ERROR(ErrorCode::ERR_INVALID_INDEX);
    }

    return &m_drawPasses.at(idx);
}

void RenderPass::CreateInternal(RenderPassDesc& p_desc) {
    m_name = std::move(p_desc.name);
    m_inputs = std::move(p_desc.dependencies);
}

void RenderPass::Execute(const renderer::RenderSystem& p_data, IRenderCmdContext& p_cmd) {
    RT_DEBUG("-- Executing pass '{}'", RenderPassNameToString(m_name));

    for (auto& pass : m_drawPasses) {
        RenderPassExcutionContext ctx{
            .render_system = p_data,
            .framebuffer = pass.framebuffer.get(),
            .pass = pass,
            .cmd = p_cmd,
        };

        auto framebuffer = pass.framebuffer.get();
        p_cmd.BeginDrawPass(framebuffer);
        pass.executor(ctx);
        p_cmd.EndDrawPass(framebuffer);

        pass.commands.clear();
    }

    RT_DEBUG("-------");
}

}  // namespace my::renderer
