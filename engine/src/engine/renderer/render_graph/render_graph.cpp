#include "render_graph.h"

namespace my {

void RenderGraph::AddResource(const std::string& p_name, const std::shared_ptr<GpuTexture>& p_resource) {
    const int idx = static_cast<int>(m_resources.size());
    m_resources.push_back(p_resource);
    m_resourceLookup.insert({ p_name, idx });
}

std::shared_ptr<GpuTexture> RenderGraph::FindResource(const std::string& p_name) {
    auto it = m_resourceLookup.find(p_name);
    if (it == m_resourceLookup.end()) {
        return nullptr;
    }

    return m_resources[it->second];
}

void RenderGraph::AddPass(const std::string& p_name, const std::shared_ptr<RenderPass>& p_pass) {
    const int idx = static_cast<int>(m_renderPasses.size());
    m_renderPasses.push_back(p_pass);
    m_renderPassLookup.insert({ p_name, idx });
}

RenderPass* RenderGraph::FindPass(const std::string& p_name) {
    auto it = m_renderPassLookup.find(p_name);
    if (it == m_renderPassLookup.end()) {
        return nullptr;
    }

    return m_renderPasses[it->second].get();
}

void RenderGraph::Execute(const RenderSystem& p_data, IGraphicsManager& p_graphics_manager) {
    for (auto pass : m_renderPasses) {
        pass->Execute(p_data, p_graphics_manager);
    }
}

}  // namespace my
