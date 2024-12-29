#include "render_graph.h"

#define RENDER_GRAPH_DEBUG_PRINT IN_USE

namespace my::renderer {

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

#if 0
    for (int i = 1; i < (int)m_levels.size(); ++i) {
        for (int from : m_levels[i - 1]) {
            for (int to : m_levels[i]) {
                if (graph.has_edge(from, to)) {
                    const RenderPass* a = m_renderPasses[from].get();
                    const RenderPass* b = m_renderPasses[to].get();
                    LOG_VERBOSE("[render graph] dependency from '{}' to '{}'", a->GetNameString(), b->GetNameString());
                    m_links.push_back({ from, to });
                }
            }
        }
    }

    int i = 0;
    for (int index : m_sortedOrder) {
        auto& pass = m_renderPasses[index];
        ++i;
        LOG_VERBOSE("Excuting order {}: '{}'", i, pass->GetNameString());
    }
#endif
}

void RenderGraph::Execute(const renderer::RenderData& p_data, GraphicsManager& p_graphics_manager) {
    for (auto pass : m_graph) {
        pass->Execute(p_data, p_graphics_manager);
    }
}

}  // namespace my::renderer
