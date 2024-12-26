#include "render_graph_viewer.h"

#include <imnodes/imnodes.h>

#include "editor/editor_layer.h"
#include "engine/core/framework/application.h"
#include "engine/core/framework/graphics_manager.h"

namespace my {

void RenderGraphViewer::DrawNodes(const Graph<RenderPass*> p_graph) {
    const auto& order = p_graph.GetSortedOrder();
    const auto& vertices = p_graph.GetVertices();

    // @TODO: draw nodes without dependencies
    std::list<int> no_dependencies;
    std::list<int> with_dependencies;
    const auto& adj_list = p_graph.GetAdjList();
    for (auto id : order) {
        bool has_dep = false;
        for (const auto& adj : adj_list) {
            if (adj.contains(id)) {
                has_dep = true;
                break;
            }
        }

        if (has_dep) {
            with_dependencies.push_back(id);
        } else {
            no_dependencies.push_front(id);
        }
    }

#if 0
    for (auto id : no_dependencies) {
        LOG("{} doesn't have dependecies", vertices[id]->GetNameString());
    }
    for (auto id : with_dependencies) {
        LOG("{} has dependecies", vertices[id]->GetNameString());
    }
#endif

    auto draw_node = [&vertices, this](int id, float x, float y) {
        const auto pass = vertices[id];

        ImNodes::BeginNode(id);

        if (m_firstFrame) {
            ImVec2 position(x, y);
            ImNodes::SetNodeGridSpacePos(id, position);
        }
        {
            ImNodes::BeginNodeTitleBar();
            ImGui::TextUnformatted(pass->GetNameString());
            ImNodes::EndNodeTitleBar();
        }
        ImGui::Spacing();
        {
            ImNodes::BeginInputAttribute(id << 16);
            ImGui::PushItemWidth(120.0f);
            ImGui::TextUnformatted("input");
            ImGui::PopItemWidth();
            ImNodes::EndInputAttribute();
        }
        ImGui::Spacing();
        {
            ImNodes::BeginStaticAttribute(id);
            const auto& framebuffer = pass->GetDrawPasses()[0].framebuffer;
            if (framebuffer) {
                auto add_image = [](const std::shared_ptr<GpuTexture>& p_texture) {
                    if (p_texture && p_texture->desc.dimension == Dimension::TEXTURE_2D) {
                        ImGui::Image(p_texture->GetHandle(), ImVec2(180, 120));
                    }
                };

                for (const auto& texture : framebuffer->desc.colorAttachments) {
                    add_image(texture);
                }
                add_image(framebuffer->desc.depthAttachment);
            }
            ImNodes::EndStaticAttribute();
        }
        ImGui::Spacing();
        {
            ImNodes::BeginOutputAttribute(id << 24);
            const float text_width = ImGui::CalcTextSize("output").x;
            ImGui::Indent(120.f + ImGui::CalcTextSize("value").x - text_width);
            ImGui::TextUnformatted("output");
            ImNodes::EndOutputAttribute();
        }

        ImNodes::EndNode();
    };

    const float initial_offset = 20.f;
    float x_offset = initial_offset;
    float y_offset = initial_offset;
    for (auto id : no_dependencies) {
        draw_node(id, x_offset, y_offset);
        y_offset += 240.0f;
    }

    for (auto id : with_dependencies) {
        x_offset += 240.0f;
        draw_node(id, x_offset, initial_offset);
    }

    for (int from = 0; from < (int)adj_list.size(); ++from) {
        for (auto to : adj_list[from]) {
            const int id = (from << 24) | (to << 16);
            ImNodes::Link(id, from << 24, to << 16);
        }
    }
}

void RenderGraphViewer::UpdateInternal(Scene&) {
    auto graphics_manager = m_editor.GetApplication()->GetGraphicsManager();

    switch (graphics_manager->GetBackend()) {
        case Backend::METAL:
            return;
        default:
            break;
    }
    
    const auto graph = graphics_manager->GetActiveRenderGraph();

#if 0
    if (frame_count == 0) {
        // @TODO: debug print the dependencies
        int size = (int)graph->m_renderPasses.size();
        std::vector<std::vector<int>> chart;

        std::string header;
        for (int i = 0; i < size; ++i) {
            chart.push_back(std::vector<int>());
            chart.back().resize(size);

            header += std::format("{:<12}", graph->m_renderPasses[i]->GetNameString());
        }

        const auto& adj_list = graph->m_graph.GetAdjList();
        for (int from = 0; from < (int)adj_list.size(); ++from) {
            for (int to : adj_list[from]) {
                chart[from][to] = 1;
            }
        }

        LOG("{}", header);

        for (int i = 0; i < size; ++i) {
            std::string line;
            for (int j = 0; j < size; ++j) {
                line += std::format("{:<12}", chart[i][j]);
            }
            line += std::format("[{}]", graph->m_renderPasses[i]->GetNameString());
            LOG("{}", line);
        }
    }
#endif

    ImNodes::BeginNodeEditor();

    DrawNodes(graph->GetGraph());

    ImNodes::MiniMap(0.2f, ImNodesMiniMapLocation_BottomRight);
    ImNodes::EndNodeEditor();

    m_firstFrame = false;
}

}  // namespace my
