#include "content_browser.h"

namespace my {

constexpr float indentWidth = 8.f;

static void list_meshes(const Scene& p_scene, ImGuiTreeNodeFlags p_flags) {
    unused(p_flags);

    p_flags = ImGuiTreeNodeFlags_NoTreePushOnOpen;
    bool expanded = ImGui::TreeNodeEx("Mesh", p_flags);
    unused(expanded);

    if (expanded) {
        ImGui::Indent(indentWidth);
        for (size_t idx = 0; idx < p_scene.get_count<MeshComponent>(); ++idx) {
            auto id = p_scene.get_entity<MeshComponent>(idx);
            const NameComponent* name = p_scene.get_component<NameComponent>(id);
            DEV_ASSERT(name);
            ImGui::TreeNodeEx((void*)(uint64_t)id.get_id(), ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen, "%s {%u}", name->get_name().c_str(), id.get_id());
        }
        ImGui::Unindent(indentWidth);
    }
}

static void draw_node(const Scene& p_scene, ImGuiTreeNodeFlags p_flags) {
    // ecs::Entity id = p_hier->entity;
    // const NameComponent* name_component = p_scene.get_component<NameComponent>(id);
    // const char* name = name_component ? name_component->get_name().c_str() : "Untitled";

    // auto nodeTag = std::format("##{}", id.get_id());
    // auto tag = std::format("{}{}", name, nodeTag);

    std::string dummy = "Assets";

    p_flags |= ImGuiTreeNodeFlags_NoTreePushOnOpen;
    // p_flags |= p_hier->children.empty() ? ImGuiTreeNodeFlags_Leaf : 0;
    // p_flags |= m_editor_layer.get_selected_entity() == id ? ImGuiTreeNodeFlags_Selected : 0;
    bool expanded = ImGui::TreeNodeEx(dummy.c_str(), p_flags);
    if (ImGui::IsItemHovered()) {
        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
            // m_editor_layer.select_entity(id);
        } else if (ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
            // m_editor_layer.select_entity(id);
            // ImGui::OpenPopup(POPUP_NAME_ID);
        }
    }

    if (expanded) {
        ImGui::Indent(indentWidth);
        list_meshes(p_scene, 0);
        ImGui::Unindent(indentWidth);
    }
}

void ContentBrowser::update_internal(Scene& scene) {
    draw_node(scene, ImGuiTreeNodeFlags_DefaultOpen);
}

}  // namespace my
