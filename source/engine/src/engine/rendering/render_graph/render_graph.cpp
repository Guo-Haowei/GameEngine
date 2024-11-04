#include "render_graph.h"

namespace my::rg {

std::shared_ptr<RenderPass> RenderGraph::CreatePass(RenderPassDesc& p_desc) {
    std::shared_ptr<RenderPass> render_pass = std::make_shared<RenderPass>();
    render_pass->createInternal(p_desc);
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

    Graph graph(num_passes);

    for (int pass_index = 0; pass_index < num_passes; ++pass_index) {
        const std::shared_ptr<RenderPass>& pass = m_renderPasses[pass_index];
        for (RenderPassName input : pass->m_inputs) {
            auto it = m_renderPassLookup.find(input);
            if (it == m_renderPassLookup.end()) {
                CRASH_NOW_MSG(std::format("dependency '{}' not found", RenderPassNameToString(input)));
            } else {
                graph.add_edge(it->second, pass_index);
            }
        }
    }

    if (graph.has_cycle()) {
        CRASH_NOW_MSG("render graph has cycle");
    }

    graph.remove_redundant();

    m_levels = graph.build_level();
    m_sortedOrder.reserve(num_passes);
    for (const auto& level : m_levels) {
        for (int i : level) {
            m_sortedOrder.emplace_back(i);
        }
    }

    for (int i = 1; i < (int)m_levels.size(); ++i) {
        for (int from : m_levels[i - 1]) {
            for (int to : m_levels[i]) {
                if (graph.has_edge(from, to)) {
                    const RenderPass* a = m_renderPasses[from].get();
                    const RenderPass* b = m_renderPasses[to].get();
                    LOG_VERBOSE("[render graph] dependency from '{}' to '{}'", a->getNameString(), b->getNameString());
                    m_links.push_back({ from, to });
                }
            }
        }
    }
}

void RenderGraph::Execute() {
    for (int index : m_sortedOrder) {
        m_renderPasses[index]->execute();
    }
}

}  // namespace my::rg
