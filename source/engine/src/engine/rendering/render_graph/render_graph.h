#pragma once
#include "core/base/graph.h"
#include "core/base/noncopyable.h"
#include "render_pass.h"

namespace my::rg {

class RenderGraph : public NonCopyable {
public:
    void add_pass(RenderPassDesc& desc);
    std::shared_ptr<RenderPass> find_pass(const std::string& name) const;

    std::shared_ptr<Resource> create_resource(const ResourceDesc& desc);
    std::shared_ptr<Resource> find_resouce(const std::string& name) const;

    void compile();

    void execute();

private:
    std::vector<std::shared_ptr<RenderPass>> m_render_passes;
    std::vector<int> m_sorted_order;
    std::vector<std::pair<int, int>> m_links;
    std::vector<std::vector<int>> m_levels;

    std::map<std::string, int> m_render_pass_lookup;
    std::map<std::string, std::shared_ptr<Resource>> m_resource_lookup;

    friend class RenderGraphEditorDelegate;
};

}  // namespace my::rg
