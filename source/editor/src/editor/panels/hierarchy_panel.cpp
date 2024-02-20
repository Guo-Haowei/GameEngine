#include "hierarchy_panel.h"

#include <imgui/imgui_internal.h>

#include "core/framework/scene_manager.h"
#include "editor/editor_layer.h"

namespace my {

// @TODO: do not traverse every frame
class HierarchyCreator {
public:
    struct HierarchyNode {
        HierarchyNode* parent = nullptr;
        ecs::Entity entity;

        std::vector<HierarchyNode*> children;
    };

    HierarchyCreator(EditorLayer& editor) : m_editor_layer(editor) {}

    void update(const Scene& scene) {
        if (build(scene)) {
            DEV_ASSERT(m_root);
            draw_node(scene, m_root, ImGuiTreeNodeFlags_DefaultOpen);
        }
    }

private:
    bool build(const Scene& scene);
    void draw_node(const Scene& scene, HierarchyNode* pNode, ImGuiTreeNodeFlags flags = 0);

    std::map<ecs::Entity, std::shared_ptr<HierarchyNode>> m_nodes;
    HierarchyNode* m_root = nullptr;
    EditorLayer& m_editor_layer;
};

namespace imgui_util {

}

// @TODO: make it an widget
void HierarchyCreator::draw_node(const Scene& p_scene, HierarchyNode* p_hier, ImGuiTreeNodeFlags p_flags) {
    DEV_ASSERT(p_hier);
    ecs::Entity id = p_hier->entity;
    const NameComponent* name_component = p_scene.get_component<NameComponent>(id);
    const char* name = name_component ? name_component->get_name().c_str() : "Untitled";

    auto nodeTag = std::format("##{}", id.get_id());
    auto tag = std::format("{}{}", name, nodeTag);

    p_flags |= ImGuiTreeNodeFlags_NoTreePushOnOpen;
    p_flags |= p_hier->children.empty() ? ImGuiTreeNodeFlags_Leaf : 0;
    p_flags |= m_editor_layer.get_selected_entity() == id ? ImGuiTreeNodeFlags_Selected : 0;
    bool expanded = ImGui::TreeNodeEx(nodeTag.c_str(), p_flags);
    ImGui::SameLine();
    ImGui::Selectable(tag.c_str());
    if (ImGui::IsItemHovered()) {
        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
            m_editor_layer.select_entity(id);
        } else if (ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
            m_editor_layer.select_entity(id);
            ImGui::OpenPopup(POPUP_NAME_ID);
        }
    }

    if (expanded) {
        float indentWidth = 8.f;
        ImGui::Indent(indentWidth);
        for (auto& child : p_hier->children) {
            draw_node(p_scene, child);
        }
        ImGui::Unindent(indentWidth);
    }
}

bool HierarchyCreator::build(const Scene& scene) {
    // @TODO: on scene change instead of build every frame
    const size_t hierarchy_count = scene.get_count<HierarchyComponent>();
    if (hierarchy_count == 0) {
        return false;
    }

    for (int i = 0; i < hierarchy_count; ++i) {
        auto FindOrCreate = [this](ecs::Entity id) {
            auto it = m_nodes.find(id);
            if (it == m_nodes.end()) {
                m_nodes[id] = std::make_shared<HierarchyNode>();
                return m_nodes[id].get();
            }
            return it->second.get();
        };

        const HierarchyComponent& hier = scene.get_component<HierarchyComponent>(i);
        const ecs::Entity selfId = scene.get_entity<HierarchyComponent>(i);
        const ecs::Entity parentId = hier.GetParent();
        HierarchyNode* parentNode = FindOrCreate(parentId);
        HierarchyNode* selfNode = FindOrCreate(selfId);
        parentNode->children.push_back(selfNode);
        parentNode->entity = parentId;
        selfNode->parent = parentNode;
        selfNode->entity = selfId;
    }

    int nodes_without_parent = 0;
    for (auto& it : m_nodes) {
        if (!it.second->parent) {
            ++nodes_without_parent;
            m_root = it.second.get();
        }
    }
    DEV_ASSERT(nodes_without_parent == 1);
    return true;
}

void HierarchyPanel::update_internal(Scene& scene) {
    // @TODO: on scene change, rebuild hierarchy
    HierarchyCreator creator(m_editor);

    draw_popup(scene);

    creator.update(scene);
}

void HierarchyPanel::draw_popup(Scene&) {
    auto selected = m_editor.get_selected_entity();
    // @TODO: save commands for undo

    if (ImGui::BeginPopup(POPUP_NAME_ID)) {
        open_add_entity_popup(selected);
        if (ImGui::MenuItem("Delete")) {
            LOG_WARN("@TODO: not implement");
        }
        ImGui::EndPopup();
    }
}

}  // namespace my
