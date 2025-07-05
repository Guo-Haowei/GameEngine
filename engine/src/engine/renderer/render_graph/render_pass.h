#pragma once
#include "engine/renderer/render_command.h"
#include "engine/renderer/render_graph/framebuffer.h"
#include "engine/renderer/render_graph/render_graph_defines.h"

// clang-format off
namespace my { class IGraphicsManager; }
namespace my::renderer { struct DrawPass; }
// clang-format on

namespace my {
// @TODO: split RHI and RenderCommandContext
using IRenderCmdContext = IGraphicsManager;
}  // namespace my

namespace my::renderer {

struct RenderData;

struct RenderPassDesc {
    RenderPassName name;
    std::vector<RenderPassName> dependencies;
};

using DrawPassExecuteFunc = void (*)(const renderer::RenderData&,
                                     const Framebuffer*,
                                     DrawPass&,
                                     const IRenderCmdContext&);

struct DrawPass {
    std::shared_ptr<Framebuffer> framebuffer;
    DrawPassExecuteFunc executor;

    std::vector<RenderCommand> commnads;

    void AddCommand(const RenderCommand& cmd) {
        commnads.emplace_back(cmd);
    }
};

class RenderPass {
public:
    void AddDrawPass(std::shared_ptr<Framebuffer> p_framebuffer, DrawPassExecuteFunc p_func) {
        m_drawPasses.emplace_back(DrawPass{ p_framebuffer, p_func, {} });
    }

    void Execute(const renderer::RenderData& p_data, IRenderCmdContext& p_cmd);

    RenderPassName GetName() const { return m_name; }
    const char* GetNameString() const { return RenderPassNameToString(m_name); }

    const auto& GetDrawPasses() const { return m_drawPasses; }

protected:
    void CreateInternal(RenderPassDesc& pass_desc);

    RenderPassName m_name;
    std::vector<RenderPassName> m_inputs;
    std::vector<DrawPass> m_drawPasses;

    friend class RenderGraph;
};

}  // namespace my::renderer
