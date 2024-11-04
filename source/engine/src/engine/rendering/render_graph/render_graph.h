#pragma once
#include "core/base/graph.h"
#include "core/base/noncopyable.h"
#include "render_pass.h"

namespace my::rg {

class RenderGraph : public NonCopyable {
public:
    std::shared_ptr<RenderPass> CreatePass(RenderPassDesc& p_desc);
    std::shared_ptr<RenderPass> FindPass(RenderPassName p_name) const;

    // @TODO: graph should not own resource, move to graphics manager

    void Compile();

    void Execute();

private:
    std::vector<std::shared_ptr<RenderPass>> m_renderPasses;
    std::vector<int> m_sortedOrder;
    std::vector<std::pair<int, int>> m_links;
    std::vector<std::vector<int>> m_levels;

    std::map<RenderPassName, int> m_renderPassLookup;

    friend class RenderGraphEditorDelegate;
};

}  // namespace my::rg
