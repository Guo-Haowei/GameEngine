#pragma once
#include "engine/core/base/noncopyable.h"
#include "render_pass.h"

namespace my {

#define RENDER_GRAPH_LIST                    \
    RENDER_GRAPH_DECLARE(EMPTY, "empty")     \
    RENDER_GRAPH_DECLARE(DEFAULT, "default") \
    RENDER_GRAPH_DECLARE(DUMMY, "dummy")     \
    RENDER_GRAPH_DECLARE(PATHTRACER, "pathtracer")

enum class RenderGraphName : uint8_t {
#define RENDER_GRAPH_DECLARE(ENUM, ...) ENUM,
    RENDER_GRAPH_LIST
#undef RENDER_GRAPH_DECLARE
        COUNT,
};

}  // namespace my

namespace my::renderer {

struct RenderSystem;

class RenderGraph : public NonCopyable {
public:
    struct Edge {
        int from;
        int to;
    };

    void AddResource(const std::string& p_name, const std::shared_ptr<GpuTexture>& p_resource);
    std::shared_ptr<GpuTexture> FindResource(const std::string& p_name);

    void AddPass(const std::string& p_name, const std::shared_ptr<RenderPass>& p_pass);
    RenderPass* FindPass(const std::string& p_name);

    void Execute(const renderer::RenderSystem& p_data, IGraphicsManager& p_graphics_manager);

    const auto& GetRenderPasses() const { return m_renderPasses; }

private:
    std::vector<std::shared_ptr<RenderPass>> m_renderPasses;
    std::map<std::string, int> m_renderPassLookup;

    std::vector<std::shared_ptr<GpuTexture>> m_resources;
    std::map<std::string, int> m_resourceLookup;

    friend class RenderGraphBuilder;
};

}  // namespace my::renderer
