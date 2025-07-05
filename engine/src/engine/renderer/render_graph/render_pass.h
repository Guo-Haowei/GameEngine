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

struct RenderSystem;

struct RenderPassDesc {
    RenderPassName name;
    std::vector<RenderPassName> dependencies;
};

using DrawPassExecuteFunc = void (*)(const renderer::RenderSystem&,
                                     const Framebuffer*,
                                     DrawPass&,
                                     IRenderCmdContext&);

struct DrawPass {
    std::string name;
    std::shared_ptr<Framebuffer> framebuffer;
    DrawPassExecuteFunc executor;

    std::vector<RenderCommand> commands;

    void AddCommand(const RenderCommand& cmd) {
        commands.emplace_back(cmd);
    }
};

class RenderPass {
public:
    void AddDrawPass(std::string_view p_name, std::shared_ptr<Framebuffer> p_framebuffer, DrawPassExecuteFunc p_func);

    void Execute(const renderer::RenderSystem& p_data, IRenderCmdContext& p_cmd);

    RenderPassName GetName() const { return m_name; }

    const char* GetNameString() const { return RenderPassNameToString(m_name); }

    const auto& GetDrawPasses() const { return m_drawPasses; }

    [[nodiscard]] auto FindDrawPass(const std::string& p_name) -> Result<DrawPass*>;

protected:
    void CreateInternal(RenderPassDesc& pass_desc);

    RenderPassName m_name;
    std::vector<RenderPassName> m_inputs;

    std::vector<DrawPass> m_drawPasses;
    std::unordered_map<std::string, uint32_t> m_lookup;

    friend class RenderGraph;
};

}  // namespace my::renderer
