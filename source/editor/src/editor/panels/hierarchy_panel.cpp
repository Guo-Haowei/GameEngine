#include "hierarchy_panel.h"

#include <imgui/imgui_internal.h>

#include "core/framework/scene_manager.h"
#include "editor/editor_layer.h"

namespace my {

#define POPUP_NAME_ID "SCENE_PANEL_POPUP"

class HierarchyCreator {
public:
    struct HierarchyNode {
        HierarchyNode* parent = nullptr;
        ecs::Entity entity;

        std::vector<HierarchyNode*> children;
    };

    HierarchyCreator(EditorLayer& editor) : m_editor_layer(editor) {}

    void Draw(const Scene& scene) {
        if (Build(scene)) {
            DEV_ASSERT(m_root);
            DrawNode(scene, m_root, ImGuiTreeNodeFlags_DefaultOpen);
        }
    }

private:
    bool Build(const Scene& scene);
    void DrawNode(const Scene& scene, HierarchyNode* pNode, ImGuiTreeNodeFlags flags = 0);

    std::map<ecs::Entity, std::shared_ptr<HierarchyNode>> m_nodes;
    HierarchyNode* m_root = nullptr;
    EditorLayer& m_editor_layer;
};

void HierarchyCreator::DrawNode(const Scene& scene, HierarchyNode* pHier, ImGuiTreeNodeFlags flags) {
    DEV_ASSERT(pHier);
    ecs::Entity id = pHier->entity;
    const NameComponent* name_component = scene.get_component<NameComponent>(id);
    const char* name = name_component ? name_component->get_name().c_str() : "Untitled";

    auto nodeTag = std::format("##{}", id.get_id());
    auto tag = std::format("{}{}", name, nodeTag);

    flags |= ImGuiTreeNodeFlags_NoTreePushOnOpen;
    flags |= pHier->children.empty() ? ImGuiTreeNodeFlags_Leaf : 0;
    flags |= m_editor_layer.get_selected_entity() == id ? ImGuiTreeNodeFlags_Selected : 0;
    bool expanded = ImGui::TreeNodeEx(nodeTag.c_str(), flags);
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
        for (auto& child : pHier->children) {
            DrawNode(scene, child);
        }
        ImGui::Unindent(indentWidth);
    }
}

bool HierarchyCreator::Build(const Scene& scene) {
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

    creator.Draw(scene);
}

static std::string gen_name(std::string_view name) {
    static int s_counter = 0;
    return std::format("{}_{}", name, ++s_counter);
}

void HierarchyPanel::draw_popup(Scene& scene) {
    auto selected = m_editor.get_selected_entity();
    // @TODO: save commands for undo

    if (ImGui::BeginPopup(POPUP_NAME_ID)) {
        if (ImGui::BeginMenu("add child##" POPUP_NAME_ID)) {
            if (ImGui::MenuItem("add entity##" POPUP_NAME_ID)) {
                LOG_WARN("@TODO: implement");
            }
            if (ImGui::MenuItem("add light##" POPUP_NAME_ID)) {
                auto id = scene.create_pointlight_entity(gen_name("light"), vec3{ 0 });
                scene.attach_component(id, selected);
                // @TODO: set selected
                m_editor.select_entity(id);
                SceneManager::singleton().bump_revision();
            }
            if (ImGui::MenuItem("add cube##" POPUP_NAME_ID)) {
                auto id = scene.create_cube_entity(gen_name("cube"));
                scene.attach_component(id, selected);
                m_editor.select_entity(id);
                SceneManager::singleton().bump_revision();
            }

            ImGui::EndMenu();
        }
        if (ImGui::MenuItem("delete##" POPUP_NAME_ID)) {
            scene.remove_entity(selected);
        }
        ImGui::EndPopup();
    }
}

}  // namespace my
