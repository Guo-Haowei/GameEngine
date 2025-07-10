#pragma once
#include "engine/render_graph/framebuffer.h"
#include "engine/render_graph/render_graph_defines.h"
#include "engine/renderer/render_command.h"

// clang-format off
namespace my { class IGraphicsManager; }
namespace my { struct GpuTexture; }
namespace my { class RenderPass; }
// clang-format on

namespace my {
// @TODO: split RHI and RenderCommandContext
using IRenderCmdContext = IGraphicsManager;
}  // namespace my

namespace my {

struct FrameData;

struct RenderPassExcutionContext {
    const FrameData& frameData;
    Framebuffer* framebuffer;
    RenderPass& pass;
    IRenderCmdContext& cmd;
};

using ExecuteFunc = void (*)(RenderPassExcutionContext& ctx);

class RenderPass {
public:
    void Execute(const FrameData& p_data, IRenderCmdContext& p_cmd);

    std::string_view GetName() const { return m_name; }

    const auto& GetUavs() const { return m_uavs; }
    const auto& GetRtvs() const { return m_rtvs; }
    const auto& GetSrvs() const { return m_srvs; }
    const auto& GetDsv() const { return m_dsv; }

protected:
    std::string m_name;

    std::vector<std::shared_ptr<GpuTexture>> m_rtvs;
    std::shared_ptr<GpuTexture> m_dsv;

    std::vector<std::shared_ptr<GpuTexture>> m_uavs;
    std::vector<std::shared_ptr<GpuTexture>> m_srvs;

    std::shared_ptr<Framebuffer> m_framebuffer;

    ExecuteFunc m_executor;

    friend class RenderPassBuilder;
    friend class RenderGraphBuilder;
    friend class RenderGraph;
};

}  // namespace my
