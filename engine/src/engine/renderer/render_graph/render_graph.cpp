#include "render_graph.h"

#define RENDER_GRAPH_DEBUG_PRINT IN_USE

namespace my::renderer {

#if 0
std::shared_ptr<RenderPass> RenderGraph::CreatePass(RenderPassDesc& p_desc) {
    std::shared_ptr<RenderPass> render_pass = std::make_shared<RenderPass>();
    render_pass->CreateInternal(p_desc);
    m_renderPasses.emplace_back(render_pass);

    DEV_ASSERT(m_renderPassLookup.find(render_pass->m_name) == m_renderPassLookup.end());
    m_renderPassLookup[render_pass->m_name] = (int)m_renderPasses.size() - 1;
    return render_pass;
}

std::shared_ptr<RenderPass> RenderGraph::FindPass(RenderPassName p_name) const {
    auto it = m_renderPassLookup.find(p_name);
    if (it == m_renderPassLookup.end()) {
        return nullptr;
    }

    return m_renderPasses[it->second];
}

// @TODO: toposort
void RenderGraph::Compile() {
    const int num_passes = (int)m_renderPasses.size();

    m_graph.SetNodeCount(num_passes);

    for (int pass_index = 0; pass_index < num_passes; ++pass_index) {
        const std::shared_ptr<RenderPass>& pass = m_renderPasses[pass_index];
        m_graph.AddVertex(pass.get());
        for (RenderPassName input : pass->m_inputs) {
            auto it = m_renderPassLookup.find(input);
            if (it == m_renderPassLookup.end()) {
                CRASH_NOW_MSG(std::format("dependency '{}' not found", RenderPassNameToString(input)));
            } else {
                m_graph.AddEdge(it->second, pass_index);
                LOG("add edge: {} -> {}", it->second, pass_index);
            }
        }
    }

    if (!m_graph.Compile()) {
        CRASH_NOW_MSG("render graph has cycle");
    }
}
#endif

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

void RenderGraph::Execute(const renderer::RenderSystem& p_data, IGraphicsManager& p_graphics_manager) {
    for (auto pass : m_renderPasses) {
        pass->Execute(p_data, p_graphics_manager);
    }
}

}  // namespace my::renderer
