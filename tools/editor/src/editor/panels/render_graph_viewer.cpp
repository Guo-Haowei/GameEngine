#include "render_graph_viewer.h"

#include <imnodes/imnodes.h>

#include "editor/editor_layer.h"
#include "engine/renderer/graphics_manager.h"
#include "engine/render_graph/render_graph.h"
#include "engine/runtime/application.h"

namespace my {

// @TODO: save the nodes position to disk
// @TODO: find longese path, and arrange nodes

RenderGraphViewer::RenderGraphViewer(EditorLayer& p_editor) : EditorWindow("RenderGraph", p_editor) {
}

void RenderGraphViewer::DrawNodes(const RenderGraph& p_graph) {
    const auto& passes = p_graph.GetRenderPasses();

    auto draw_node = [&passes, this](int id, float x, float y) {
        const auto pass = passes[id].get();
        const bool flip_image = m_backend == Backend::OPENGL;

        ImNodes::BeginNode(id);

        if (m_firstFrame) {
            ImVec2 position(x, y);
            ImNodes::SetNodeGridSpacePos(id, position);
        }
        {
            ImNodes::BeginNodeTitleBar();
            ImGui::TextUnformatted(pass->GetName().data());
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

        auto add_image = [](bool p_flip, const std::shared_ptr<GpuTexture>& p_texture) {
            ImGui::Text("%s", p_texture->desc.name.c_str());
            if (p_texture && p_texture->desc.dimension == Dimension::TEXTURE_2D) {
                ImVec2 size(180 * 3, 120 * 3);
                if (p_flip) {
                    ImGui::Image(p_texture->GetHandle(), size, ImVec2(0, 1), ImVec2(1, 0));
                } else {
                    ImGui::Image(p_texture->GetHandle(), size);
                }
            }
        };

        ImGui::Spacing();
        {
            ImNodes::BeginStaticAttribute(id);
            for (const auto& srv : pass->GetSrvs()) {
                add_image(flip_image, srv);
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
        ImGui::Spacing();
        {
            ImNodes::BeginInputAttribute(id);

            for (const auto& rtv : pass->GetRtvs()) {
                add_image(flip_image, rtv);
            }
            ImNodes::EndInputAttribute();
        }

        ImNodes::EndNode();
    };

    const float initial_offset = 20.f;
    float x_offset = initial_offset;
    [[maybe_unused]] float y_offset = initial_offset;

    for (int i = 0; i < (int)passes.size(); ++i) {
        x_offset += 240.0f * 3;
        draw_node(i, x_offset, initial_offset);
    }

    // for (int from = 0; from < (int)adj_list.size(); ++from) {
    //     for (auto to : adj_list[from]) {
    //         const int id = (from << 24) | (to << 16);
    //         ImNodes::Link(id, from << 24, to << 16);
    //     }
    // }
}

void RenderGraphViewer::UpdateInternal(Scene&) {
    auto graphics_manager = m_editor.GetApplication()->GetGraphicsManager();
    if (m_backend == Backend::COUNT) {
        m_backend = m_editor.GetApplication()->GetGraphicsManager()->GetBackend();
    }

    switch (graphics_manager->GetBackend()) {
        case Backend::VULKAN:
        case Backend::METAL:
            return;
        default:
            break;
    }

    const auto graph = graphics_manager->GetActiveRenderGraph();

    ImNodes::BeginNodeEditor();

    DrawNodes(*graph);

    ImNodes::MiniMap(0.2f, ImNodesMiniMapLocation_BottomRight);
    ImNodes::EndNodeEditor();

    m_firstFrame = false;
}

}  // namespace my
