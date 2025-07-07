#pragma once
#include "engine/renderer/render_command.h"
#include "engine/renderer/render_graph/framebuffer.h"
#include "engine/renderer/render_graph/render_graph_defines.h"

// clang-format off
namespace my { class IGraphicsManager; }
namespace my { struct GpuTexture; }
namespace my::renderer { class RenderPass; }
// clang-format on

namespace my {
// @TODO: split RHI and RenderCommandContext
using IRenderCmdContext = IGraphicsManager;
}  // namespace my

namespace my::renderer {

struct RenderSystem;

struct RenderPassExcutionContext {
    const RenderSystem& render_system;
    Framebuffer* framebuffer;
    RenderPass& pass;
    IRenderCmdContext& cmd;
};

using ExecuteFunc = void (*)(RenderPassExcutionContext& ctx);

class RenderPass {
public:
    void Execute(const renderer::RenderSystem& p_data, IRenderCmdContext& p_cmd);

    void AddCommand(const RenderCommand& cmd) { m_commands.emplace_back(cmd); }
    const auto& GetCommands() const { return m_commands; }

    std::string_view GetName() const { return m_name; }

    const auto& GetUavs() const { return m_uavs; }

protected:
    std::string m_name;

    std::vector<std::shared_ptr<GpuTexture>> m_rtvs;
    std::shared_ptr<GpuTexture> m_dsv;

    std::vector<std::shared_ptr<GpuTexture>> m_uavs;
    std::vector<std::shared_ptr<GpuTexture>> m_srvs;

    std::shared_ptr<Framebuffer> m_framebuffer;

    std::vector<RenderCommand> m_commands;
    ExecuteFunc m_executor;

    friend class RenderPassBuilder;
    friend class RenderGraphBuilder;
    friend class RenderGraph;
};

}  // namespace my::renderer
