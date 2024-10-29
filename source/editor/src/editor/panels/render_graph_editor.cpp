#include "render_graph_editor.h"

#include "core/framework/graphics_manager.h"

using namespace my::rg;

namespace my {

class RenderGraphEditorDelegate : public GraphEditor::Delegate {
public:
    RenderGraphEditorDelegate(const RenderGraph& p_graph) {
        float x_offset = 0.0f;
        for (auto& level : p_graph.m_levels) {
            float y_offset = 0.0f;
            for (int id : level) {
                const std::shared_ptr<RenderPass>& pass = p_graph.m_render_passes[id];

                mNodes.push_back(
                    {
                        pass->getNameString(),
                        0,
                        x_offset,
                        y_offset,
                        false,
                    });
                y_offset += 300.f;
            }
            x_offset += 300.f;
        }

        for (const auto& pair : p_graph.m_links) {
            GraphEditor::Link link{
                .mInputNodeIndex = pair.first,
                .mInputSlotIndex = 0,
                .mOutputNodeIndex = pair.second,
                .mOutputSlotIndex = 0,
            };
            mLinks.emplace_back(link);
        }
    }

    bool AllowedLink(GraphEditor::NodeIndex, GraphEditor::NodeIndex) override {
        return true;
    }

    void SelectNode(GraphEditor::NodeIndex, bool) override {
        // mNodes[nodeIndex].mSelected = selected;
    }

    void MoveSelectedNodes(const ImVec2 delta) override {
        for (auto& node : mNodes) {
            if (!node.mSelected) {
                continue;
            }
            node.x += delta.x;
            node.y += delta.y;
        }
    }

    virtual void RightClick(GraphEditor::NodeIndex, GraphEditor::SlotIndex, GraphEditor::SlotIndex) override {
    }

    void AddLink(GraphEditor::NodeIndex inputNodeIndex, GraphEditor::SlotIndex inputSlotIndex, GraphEditor::NodeIndex outputNodeIndex, GraphEditor::SlotIndex outputSlotIndex) override {
        mLinks.push_back({ inputNodeIndex, inputSlotIndex, outputNodeIndex, outputSlotIndex });
    }

    void DelLink(GraphEditor::LinkIndex linkIndex) override {
        mLinks.erase(mLinks.begin() + linkIndex);
    }

    void CustomDraw(ImDrawList* drawList, ImRect rectangle, GraphEditor::NodeIndex nodeIndex) override {
        unused(drawList);
        unused(rectangle);
        unused(nodeIndex);
        // drawList->AddLine(rectangle.Min, rectangle.Max, IM_COL32(0, 0, 0, 255));
        // drawList->AddText(rectangle.Min, IM_COL32(255, 128, 64, 255), "Add your stuff here");
    }

    const size_t GetTemplateCount() override {
        return sizeof(mTemplates) / sizeof(GraphEditor::Template);
    }

    const GraphEditor::Template GetTemplate(GraphEditor::TemplateIndex index) override {
        return mTemplates[index];
    }

    const size_t GetNodeCount() override {
        return mNodes.size();
    }

    const GraphEditor::Node GetNode(GraphEditor::NodeIndex index) override {
        const auto& myNode = mNodes[index];
        return GraphEditor::Node{
            myNode.name,
            myNode.templateIndex,
            ImRect(ImVec2(myNode.x, myNode.y), ImVec2(myNode.x + 200, myNode.y + 200)),
            myNode.mSelected
        };
    }

    const size_t GetLinkCount() override {
        return mLinks.size();
    }

    const GraphEditor::Link GetLink(GraphEditor::LinkIndex index) override {
        return mLinks[index];
    }

    // Graph datas
    static const inline GraphEditor::Template mTemplates[] = {
        {
            IM_COL32(160, 160, 180, 255),
            IM_COL32(100, 100, 140, 255),
            IM_COL32(110, 110, 150, 255),
            1,
            nullptr,
            nullptr,
            1,
            nullptr,
            nullptr,
        },
    };

    struct Node {
        const char* name;
        GraphEditor::TemplateIndex templateIndex;
        float x, y;
        bool mSelected;
    };

    std::vector<Node> mNodes;
    std::vector<GraphEditor::Link> mLinks;
};

RenderGraphEditor::RenderGraphEditor(EditorLayer& editor) : EditorWindow("Render Graph", editor) {
    m_delegate = std::make_shared<RenderGraphEditorDelegate>(GraphicsManager::GetSingleton().GetActiveRenderGraph());
}

void RenderGraphEditor::UpdateInternal(my::Scene&) {
    if (ImGui::Button("Fit all nodes")) {
        m_fit = GraphEditor::Fit_AllNodes;
    }
    ImGui::SameLine();
    if (ImGui::Button("Fit selected nodes")) {
        m_fit = GraphEditor::Fit_SelectedNodes;
    }
    GraphEditor::Show(*m_delegate.get(), m_options, m_view_state, true, &m_fit);
}

}  // namespace my
