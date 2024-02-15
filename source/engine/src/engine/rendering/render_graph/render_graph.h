#pragma once
#include "core/base/graph.h"
#include "core/base/noncopyable.h"
#include "render_pass.h"

namespace my {

class RenderGraph : public NonCopyable {
public:
    void add_pass(RenderPassDesc& desc);

    std::shared_ptr<RenderPass> find_pass(const std::string& name) const;

    void compile();

    void execute();

private:
    std::vector<std::shared_ptr<RenderPass>> m_render_passes;
    std::vector<int> m_sorted_order;
    std::vector<std::pair<int, int>> m_links;
    std::vector<std::vector<int>> m_levels;

    std::map<std::string, int> m_render_pass_lookup;

    friend class RenderGraphEditorDelegate;
};

}  // namespace my
