#include "content_browser.h"

#include "editor/panels/panel_util.h"

namespace my {

class FolderWindow : public EditorWindow {
public:
    FolderWindow(EditorLayer& p_editor, ContentBrowser& p_parent) : EditorWindow("Content Browser", p_editor), m_parent(p_parent) {}

protected:
    void update_internal(Scene& p_scene) override;

    ContentBrowser& m_parent;
};

class AssetWindow : public EditorWindow {
public:
    AssetWindow(EditorLayer& p_editor, ContentBrowser& p_parent) : EditorWindow("Asset", p_editor), m_parent(p_parent) {}

protected:
    void update_internal(Scene& p_scene) override;

    ContentBrowser& m_parent;
};

ContentBrowser::ContentBrowser(EditorLayer& p_editor) : EditorCompositeWindow(p_editor) {
    add_window(std::make_shared<FolderWindow>(p_editor, *this));
    add_window(std::make_shared<AssetWindow>(p_editor, *this));
}

void ContentBrowser::set_selected(ecs::Entity p_entity) {
    m_selected = p_entity;
    LOG("asset {} selected!", p_entity.get_id());
}

template<typename T>
static void list_asset(const char* type, const Scene& p_scene, ContentBrowser& p_content_browser) {
    constexpr float indent_width = 8.f;
    if (ImGui::TreeNodeEx(type, ImGuiTreeNodeFlags_NoTreePushOnOpen)) {
        ImGui::Indent(indent_width);
        for (size_t idx = 0; idx < p_scene.get_count<T>(); ++idx) {
            auto id = p_scene.get_entity<T>(idx);
            const NameComponent* name = p_scene.get_component<NameComponent>(id);
            auto string_id = std::format("##{}", id.get_id());
            auto asset_name = std::format("{} (id: {})", name->get_name(), id.get_id());
            DEV_ASSERT(name);
            ImGui::TreeNodeEx(string_id.c_str(), ImGuiTreeNodeFlags_NoTreePushOnOpen | ImGuiTreeNodeFlags_Leaf);
            ImGui::SameLine();
            ImGui::Selectable(asset_name.c_str());
            if (ImGui::IsItemHovered()) {
                if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                    p_content_browser.set_selected(id);
                }
            }
        }
        ImGui::Unindent(indent_width);
    }
}

void AssetWindow::update_internal(Scene& p_scene) {
    ecs::Entity id = m_parent.get_selected();
    if (!id.is_valid()) {
        return;
    }
    NameComponent* name = p_scene.get_component<NameComponent>(id);
    ImGui::Text("Asset Name");
    panel_util::edit_name(name);
    ImGui::Separator();
    if (auto mesh = p_scene.get_component<MeshComponent>(id); mesh) {
    }
    if (auto material = p_scene.get_component<MaterialComponent>(id); material) {
        ImGui::DragFloat("metallic", &material->metallic, 0.01f, 0.0f, 1.0f);
        ImGui::DragFloat("roughness", &material->roughness, 0.01f, 0.0f, 1.0f);
        for (int i = 0; i < MaterialComponent::TEXTURE_MAX; ++i) {
            auto& texture = material->textures[i];
            ImGui::Text("path: %s", texture.path.c_str());
            // @TODO: safer
            auto check_box_id = std::format("Enabled##{}", i);
            ImGui::Checkbox(check_box_id.c_str(), &texture.enabled);
            Image* image = texture.image ? texture.image->get() : nullptr;
            uint64_t handle = image ? image->texture.handle : 0;
            if (handle) {
                ImGui::Image((ImTextureID)handle, ImVec2(256, 256));
            }
        }
    }
    if (auto animation = p_scene.get_component<AnimationComponent>(id); animation) {
        ImGui::Text("Animation %s", name->get_name().c_str());
        if (!animation->is_playing()) {
            if (ImGui::Button("play")) {
                animation->flags |= AnimationComponent::PLAYING;
            }
        } else {
            if (ImGui::Button("stop")) {
                animation->flags &= ~AnimationComponent::PLAYING;
            }
        }
        if (ImGui::SliderFloat("Frame", &animation->timer, animation->start, animation->end)) {
            animation->flags |= AnimationComponent::PLAYING;
        }
        ImGui::Separator();
    }
}

void FolderWindow::update_internal(Scene& p_scene) {
    list_asset<MeshComponent>("Mesh", p_scene, m_parent);
    list_asset<MaterialComponent>("Material", p_scene, m_parent);
    list_asset<AnimationComponent>("Animation", p_scene, m_parent);
}

}  // namespace my
