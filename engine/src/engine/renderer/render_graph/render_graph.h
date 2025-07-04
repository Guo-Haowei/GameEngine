#pragma once
#include "engine/core/base/graph.h"
#include "engine/core/base/noncopyable.h"
#include "render_pass.h"

// clang-format off
namespace my { class IGraphicsManager; }
// clang-format on

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

struct RenderData;

class RenderGraph : public NonCopyable {
public:
    struct Edge {
        int from;
        int to;
    };

    std::shared_ptr<RenderPass> CreatePass(RenderPassDesc& p_desc);
    std::shared_ptr<RenderPass> FindPass(RenderPassName p_name) const;

    // @TODO: graph should not own resource, move to graphics manager

    void Compile();

    void Execute(const renderer::RenderData& p_data, IGraphicsManager& p_graphics_manager);

    const auto& GetGraph() const { return m_graph; }

private:
    std::vector<std::shared_ptr<RenderPass>> m_renderPasses;
    Graph<RenderPass*> m_graph;

    std::map<RenderPassName, int> m_renderPassLookup;
};

}  // namespace my::renderer
