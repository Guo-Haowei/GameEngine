#include "render_graph_builder.h"

#include "engine/algorithm/algorithm.h"
#include "engine/renderer/renderer_misc.h"
#include "engine/renderer/sampler.h"
#include "engine/runtime/graphics_manager.h"
#include "render_graph.h"
#include "render_graph_defines.h"
#include "render_pass_builder.h"

namespace my {

RenderGraphBuilder::RenderGraphBuilder(const RenderGraphBuilderConfig& p_config)
    : m_config(p_config),
      m_graphicsManager(IGraphicsManager::GetSingleton()) {
}

RenderPassBuilder& RenderGraphBuilder::AddPass(std::string_view p_pass_name) {
    RenderPassBuilder builder{ p_pass_name };
    m_passes.push_back(builder);
    return m_passes.back();
}

void RenderGraphBuilder::AddDependency(std::string_view p_from, std::string_view p_to) {
    m_dependencies.emplace_back(std::make_pair(p_from, p_to));
}

auto RenderGraphBuilder::Compile() -> Result<std::shared_ptr<RenderGraph>> {

#define DEBUG_BUILDER NOT_IN_USE
#if USING(DEBUG_BUILDER)
#define DEBUG_PRINT(...) LOG_OK(__VA_ARGS__)
#else
#define DEBUG_PRINT(...) ((void)0)
#endif
    if constexpr (USING(DEBUG_BUILDER)) {
        int id = 0;
        for (const auto& pass : m_passes) {
            DEBUG_PRINT("found pass: {} (id: {})", pass.GetName(), id++);
            DEBUG_PRINT("  it creates:");
            for (const auto& create : pass.m_creates) {
                DEBUG_PRINT("  -- {}", create.first);
            }
            DEBUG_PRINT("  it reads:");
            for (const auto& read : pass.m_reads) {
                DEBUG_PRINT("  -- {}", read.name);
            }
            DEBUG_PRINT("  it writes:");
            for (const auto& write : pass.m_writes) {
                DEBUG_PRINT("  -- {}", write.name);
            }
        }
    }

    std::unordered_map<std::string_view, int> lookup;

    std::vector<std::pair<std::string_view, int>> reads;
    std::vector<std::pair<std::string_view, int>> writes;

    std::unordered_map<std::string_view, int> creates;

    const int N = static_cast<int>(m_passes.size());
    DEV_ASSERT(N);

    for (int i = 0; i < N; ++i) {
        const auto& pass = m_passes[i];
        {
            auto [_, inserted] = lookup.try_emplace(pass.m_name, i);
            if (!inserted) {
                return HBN_ERROR(ErrorCode::ERR_ALREADY_EXISTS, "pass '{}' already exists", pass.m_name);
            }
        }

        for (const auto& create : pass.m_creates) {
            auto [_, inserted] = creates.try_emplace(std::string_view(create.first), i);
            if (!inserted) {
                return HBN_ERROR(ErrorCode::ERR_ALREADY_EXISTS, "resource '{}' is created multiple times", create.first);
            }
        }

        // @TODO: figure out what access to give to resource
        for (const auto& read : pass.m_reads) {
            reads.push_back(std::make_pair(std::string_view(read.name), i));
        }
        for (const auto& write : pass.m_writes) {
            writes.push_back(std::make_pair(std::string_view(write.name), i));
        }
    }

    std::vector<std::pair<int, int>> edges;
    edges.reserve(m_dependencies.size());
    // add manual dependencies
    for (const auto& [from, to] : m_dependencies) {
        auto it = lookup.find(from);
        if (it == lookup.end()) {
            return HBN_ERROR(ErrorCode::ERR_DOES_NOT_EXIST, "pass '{}' not found", from);
        }
        const int from_idx = it->second;
        it = lookup.find(to);
        if (it == lookup.end()) {
            return HBN_ERROR(ErrorCode::ERR_DOES_NOT_EXIST, "pass '{}' not found", to);
        }
        const int to_idx = it->second;
        edges.push_back({ from_idx, to_idx });
    }

    auto add_edges = [&creates, &edges](const std::vector<std::pair<std::string_view, int>>& p_res) {
        for (const auto& [name, to] : p_res) {
            if (auto it = creates.find(name); it != creates.end()) {
                const int from = it->second;
                // remove passes that create and write the same buffer
                if (from == to) continue;
                // DEBUG_PRINT("edge found from {} (create) to {}", m_passes[from].GetName(), m_passes[to].GetName());
                edges.push_back(std::make_pair(from, to));
            } else {
                // if not found, it's possible the resource is imported
                // return HBN_ERROR(ErrorCode::ERR_DOES_NOT_EXIST, "resource '{}' not found", name);
            }
        }
    };

    add_edges(reads);
    add_edges(writes);

    auto sorted = topological_sort(N, edges);
    if (static_cast<int>(sorted.size()) != N) {
        return HBN_ERROR(ErrorCode::ERR_CYCLIC_LINK);
    }

    DEBUG_PRINT("sorted:");
    for (auto i : sorted) {
        unused(i);
        DEBUG_PRINT("{}", m_passes[i].GetName());
    }

    auto render_graph = std::make_shared<RenderGraph>();

    // 1. Create/Import resources
    for (const auto& pass : m_passes) {
        for (const auto& create : pass.m_creates) {
            const auto& name = create.first;
            const auto& create_info = create.second;
            GpuTextureDesc desc = create_info.resourceDesc;
            // @TODO: move desc
            desc.name = name;
            auto texture = m_graphicsManager.CreateTexture(desc, create_info.samplerDesc);
            render_graph->AddResource(name, texture);
        }
        for (const auto& import : pass.m_imports) {
            const auto& name = import.first;
            auto texture = import.second();
            render_graph->AddResource(name, texture);
        }
    }

    // 2. Create framebuffer (should only create it for opengl)
    for (int idx : sorted) {
        const auto& pass = m_passes[idx];

        std::vector<std::shared_ptr<GpuTexture>> srvs;
        std::vector<std::shared_ptr<GpuTexture>> uavs;
        std::vector<std::shared_ptr<GpuTexture>> rtvs;
        std::shared_ptr<GpuTexture> dsv;

        for (const auto& write : pass.m_writes) {
            switch (write.access) {
                case ResourceAccess::DSV: {
                    DEV_ASSERT(dsv == nullptr);
                    dsv = render_graph->FindResource(write.name);
                } break;
                case ResourceAccess::RTV: {
                    auto rtv = render_graph->FindResource(write.name);
                    DEV_ASSERT(rtv);
                    rtvs.emplace_back(rtv);
                } break;
                default:
                    break;
            }
        }

        FramebufferDesc info{
            .colorAttachments = rtvs,
            .depthAttachment = dsv,
        };

        for (const auto& read : pass.m_reads) {
            switch (read.access) {
                case ResourceAccess::SRV: {
                    auto srv = render_graph->FindResource(read.name);
                    DEV_ASSERT(srv);
                    srvs.emplace_back(srv);
                } break;
                case ResourceAccess::UAV: {
                    auto uav = render_graph->FindResource(read.name);
                    DEV_ASSERT(uav);
                    uavs.emplace_back(uav);
                } break;
                default:
                    break;
            }
        }

        auto render_pass = std::make_shared<RenderPass>();
        render_pass->m_name = pass.m_name;
        render_pass->m_framebuffer = m_graphicsManager.CreateFramebuffer(info);
        render_pass->m_executor = pass.m_func;

        render_pass->m_srvs = std::move(srvs);
        render_pass->m_uavs = std::move(uavs);
        render_pass->m_rtvs = std::move(rtvs);
        render_pass->m_dsv = std::move(dsv);

        render_graph->AddPass(pass.m_name, render_pass);
    }

    return Result<std::shared_ptr<RenderGraph>>(render_graph);
}

}  // namespace my
