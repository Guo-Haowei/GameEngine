#include "hierarchy_panel.h"

#include <IconsFontAwesome/IconsFontAwesome6.h>
#include <imgui/imgui_internal.h>

#include "editor/editor_layer.h"
#include "engine/core/framework/scene_manager.h"

namespace my {
using ecs::Entity;

#define POPUP_NAME_ID "SCENE_PANEL_POPUP"

// @TODO: do not traverse every frame
class HierarchyCreator {
public:
    struct HierarchyNode {
        HierarchyNode* parent = nullptr;
        Entity entity;

        std::vector<HierarchyNode*> children;
    };

    HierarchyCreator(EditorLayer& p_editor) : m_editorLayer(p_editor) {}

    void Update(const Scene& scene) {
        if (Build(scene)) {
            DEV_ASSERT(m_root);
            DrawNode(scene, m_root, ImGuiTreeNodeFlags_DefaultOpen);
        }
    }

private:
    bool Build(const Scene& p_scene);
    void DrawNode(const Scene& p_scene, HierarchyNode* p_node, ImGuiTreeNodeFlags p_flags = 0);

    std::map<Entity, std::shared_ptr<HierarchyNode>> m_nodes;
    HierarchyNode* m_root = nullptr;
    EditorLayer& m_editorLayer;
};

static bool TreeNodeHelper(const Scene& p_scene,
                             Entity p_id,
                             ImGuiTreeNodeFlags p_flags,
                             std::function<void()> p_on_left_click,
                             std::function<void()> p_on_right_click) {

    const NameComponent* name_component = p_scene.GetComponent<NameComponent>(p_id);
    std::string name = name_component->GetName();
    if (name.empty()) {
        name = "Untitled";
    }
    
    const char* icon = ICON_FA_FOLDER;
    if (p_flags & ImGuiTreeNodeFlags_Leaf) {
        icon = ICON_FA_CUBE;
    }
    auto node_name = std::format("##{}", p_id.GetId());
    auto tag = std::format("{} {}{}", icon, name, node_name);

    p_flags |= ImGuiTreeNodeFlags_NoTreePushOnOpen;

    const bool expanded = ImGui::TreeNodeEx(node_name.c_str(), p_flags);
    ImGui::SameLine();
    if (!p_on_left_click && !p_on_right_click) {
        ImGui::Text("%s", tag.c_str());
    } else {
        ImGui::Selectable(tag.c_str());
        if (ImGui::IsItemHovered()) {
            if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                if (p_on_left_click) {
                    p_on_left_click();
                }
            } else if (ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
                if (p_on_right_click) {
                    p_on_right_click();
                }
            }
        }
    }
    return expanded;
}

// @TODO: make it an widget
void HierarchyCreator::DrawNode(const Scene& p_scene, HierarchyNode* p_hier, ImGuiTreeNodeFlags p_flags) {
    DEV_ASSERT(p_hier);
    Entity id = p_hier->entity;
    const ObjectComponent* object_component = p_scene.GetComponent<ObjectComponent>(id);
    const MeshComponent* mesh_component = object_component ? p_scene.GetComponent<MeshComponent>(object_component->meshId) : nullptr;

    p_flags |= (p_hier->children.empty() && !mesh_component) ? ImGuiTreeNodeFlags_Leaf : 0;
    p_flags |= m_editorLayer.GetSelectedEntity() == id ? ImGuiTreeNodeFlags_Selected : 0;

    const bool expanded = TreeNodeHelper(
        p_scene, id, p_flags,
        [&]() {
            m_editorLayer.SelectEntity(id);
        },
        [&]() {
            m_editorLayer.SelectEntity(id);
            ImGui::OpenPopup(POPUP_NAME_ID);
        });

    if (expanded) {
        float indentWidth = 8.f;
        ImGui::Indent(indentWidth);

        if (mesh_component) {
            for (const auto& subset : mesh_component->subsets) {
                const MaterialComponent* material = p_scene.GetComponent<MaterialComponent>(subset.material_id);
                if (material) {
                    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_NoTreePushOnOpen | ImGuiTreeNodeFlags_Leaf;
                    TreeNodeHelper(
                        p_scene, subset.material_id, flags,
                        [&]() {
                            m_editorLayer.SelectEntity(subset.material_id);
                        },
                        nullptr);
                }
            }
            Entity armature_id = mesh_component->armatureId;
            if (armature_id.IsValid()) {
                ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_NoTreePushOnOpen | ImGuiTreeNodeFlags_Leaf;
                TreeNodeHelper(
                    p_scene, armature_id, flags, [&]() {
                        m_editorLayer.SelectEntity(armature_id);
                    },
                    nullptr);
            }
        }

        for (auto& child : p_hier->children) {
            DrawNode(p_scene, child);
        }
        ImGui::Unindent(indentWidth);
    }
}

bool HierarchyCreator::Build(const Scene& p_scene) {
    // @TODO: on scene change instead of build every frame
    const size_t hierarchy_count = p_scene.GetCount<HierarchyComponent>();
    if (hierarchy_count == 0) {
        return false;
    }

    for (auto [self_id, hier] : p_scene.m_HierarchyComponents) {
        auto find_or_create = [this](ecs::Entity id) {
            auto it = m_nodes.find(id);
            if (it == m_nodes.end()) {
                m_nodes[id] = std::make_shared<HierarchyNode>();
                return m_nodes[id].get();
            }
            return it->second.get();
        };

        const ecs::Entity parent_id = hier.GetParent();
        HierarchyNode* parent_node = find_or_create(parent_id);
        HierarchyNode* self_node = find_or_create(self_id);
        parent_node->children.push_back(self_node);
        parent_node->entity = parent_id;
        self_node->parent = parent_node;
        self_node->entity = self_id;
    }

    int nodes_without_parent = 0;
    for (auto& it : m_nodes) {
        if (!it.second->parent) {
            ++nodes_without_parent;
            m_root = it.second.get();
        }
    }
    if (nodes_without_parent != 1) {
        static int s_nodes_without_parent = 0;
        if (nodes_without_parent != s_nodes_without_parent) {
            LOG_ERROR("{} orphan nodes detected", nodes_without_parent - 1);
            s_nodes_without_parent = nodes_without_parent;
        }
    }
    return true;
}

void HierarchyPanel::UpdateInternal(Scene& p_scene) {
    // @TODO: on scene change, rebuild hierarchy
    HierarchyCreator creator(m_editor);

    DrawPopup(p_scene);

    creator.Update(p_scene);
}

void HierarchyPanel::DrawPopup(Scene&) {
    auto selected = m_editor.GetSelectedEntity();
    // @TODO: save commands for undo

    if (ImGui::BeginPopup(POPUP_NAME_ID)) {
        OpenAddEntityPopup(selected);
        if (ImGui::MenuItem("Delete")) {
            if (selected.IsValid()) {
                m_editor.SelectEntity(Entity::INVALID);
                m_editor.RemoveEntity(selected);
            }
        }
        ImGui::EndPopup();
    }
}

}  // namespace my
