#include "render_graph_viewer.h"

#include <imnodes/imnodes.h>

#include "editor/editor_layer.h"
#include "engine/core/framework/application.h"
#include "engine/core/framework/graphics_manager.h"

namespace my {

void RenderGraphViewer::UpdateInternal(Scene&) {
    auto graphics_manager = m_editor.GetApplication()->GetGraphicsManager();
    const auto graph = graphics_manager->GetActiveRenderGraph();

    ImNodes::BeginNodeEditor();

    static int first_frame = -1;
    first_frame++;

    for (size_t x = 0; x < graph->m_levels.size(); ++x) {
        for (size_t y = 0; y < graph->m_levels[x].size(); ++y) {
            const int id = graph->m_levels[x][y];
            const auto& pass = graph->m_renderPasses[id];

            ImNodes::BeginNode(id);

            const float width = 300.0f * x + 100;
            const float height = 200.0f * y + 100;

            ImVec2 position(width, height);
            // @HACK
            if (first_frame == 0) {
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
                const auto& framebuffer = pass->m_drawPasses[0].framebuffer;
                if (framebuffer) {
                    const GpuTexture* texture = nullptr;
                    if (!framebuffer->desc.colorAttachments.empty()) {
                        texture = framebuffer->desc.colorAttachments[0].get();
                    } else if (framebuffer->desc.depthAttachment) {
                        texture = framebuffer->desc.depthAttachment.get();
                    }
                    if (texture && texture->desc.dimension == Dimension::TEXTURE_2D) {
                        ImGui::Image(texture->GetHandle(), ImVec2(180, 120));
                    }
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
        }
    }

    for (int link = 0; link < (int)graph->m_links.size(); ++link) {
        auto [a, b] = graph->m_links[link];
        //ImNodes::Link(link, a << 16, b << 24);
        ImNodes::Link(link, a << 24, b << 16);
        //ImNodes::Link(link, a, b);
    }

    ImNodes::MiniMap(0.2f, ImNodesMiniMapLocation_BottomRight);
    ImNodes::EndNodeEditor();
}

}  // namespace my