#pragma once
#include "core/base/graph.h"
#include "core/base/noncopyable.h"
#include "render_pass.h"

namespace my::rg {

class RenderGraph : public NonCopyable {
public:
    std::shared_ptr<RenderPass> createPass(RenderPassDesc& p_desc);
    std::shared_ptr<RenderPass> findPass(RenderPassName p_name) const;

    // @TODO: graph should not own resource, move to graphics manager

    void compile();

    void execute();

private:
    std::vector<std::shared_ptr<RenderPass>> m_render_passes;
    std::vector<int> m_sorted_order;
    std::vector<std::pair<int, int>> m_links;
    std::vector<std::vector<int>> m_levels;

    std::map<RenderPassName, int> m_render_pass_lookup;

    friend class RenderGraphEditorDelegate;
};

}  // namespace my::rg
