#include "propertiy_panel.h"

// @TODO: remove this
#include "ImGuizmo/ImGuizmo.h"
#include "core/framework/scene_manager.h"
#include "editor/editor_layer.h"
#include "editor/panels/panel_util.h"
#include "editor/widget.h"

namespace my {

template<typename T, typename UIFunction>
static void DrawComponent(const std::string& name, T* component, UIFunction uiFunction) {
    const ImGuiTreeNodeFlags treeNodeFlags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed |
                                             ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowItemOverlap |
                                             ImGuiTreeNodeFlags_FramePadding;
    if (component) {
        ImVec2 contentRegionAvailable = ImGui::GetContentRegionAvail();

        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 4, 4 });
        float lineHeight = GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.0f;
        ImGui::Separator();
        bool open = ImGui::TreeNodeEx((void*)typeid(T).hash_code(), treeNodeFlags, name.c_str());
        ImGui::PopStyleVar();
        ImGui::SameLine(contentRegionAvailable.x - lineHeight * 0.5f);
        if (ImGui::Button("-", ImVec2{ lineHeight, lineHeight })) {
            ImGui::OpenPopup("ComponentSettings");
        }

        bool removeComponent = false;
        if (ImGui::BeginPopup("ComponentSettings")) {
            if (ImGui::MenuItem("remove component")) {
                removeComponent = true;
            }

            ImGui::EndPopup();
        }

        if (open) {
            uiFunction(*component);
            ImGui::TreePop();
        }

        if (removeComponent) {
            LOG_ERROR("TODO: implement remove component");
        }
    }
}

template<typename... Args>
static bool draw_vec3_control_disabled(bool disabled, Args&&... args) {
    if (disabled) {
        push_disabled();
    }
    bool dirty = draw_vec3_control(std::forward<Args>(args)...);
    if (disabled) {
        pop_disabled();
    }
    return dirty;
};

void PropertyPanel::update_internal(Scene& scene) {
    ecs::Entity id = m_editor.get_selected_entity();

    if (!id.isValid()) {
        return;
    }

    NameComponent* name_component = scene.getComponent<NameComponent>(id);
    // @NOTE: when loading another scene, the selected entity will expire, thus don't have name
    if (!name_component) {
        // LOG_WARN("Entity {} does not have name", id.get_id());
        return;
    }

    panel_util::edit_name(name_component);

    ImGui::SameLine();
    ImGui::PushItemWidth(-1);
    if (ImGui::Button("+")) {
        ImGui::OpenPopup("AddComponentPopup");
    }

    if (ImGui::BeginPopup("AddComponentPopup")) {
        if (ImGui::MenuItem("Rigid Body")) {
            LOG_ERROR("TODO: implement add component");
            ImGui::CloseCurrentPopup();
        }
        if (ImGui::BeginMenu("Selectable")) {
            // @TODO: check if exists, if exists, disable
            if (ImGui::MenuItem("Box Collider")) {
                m_editor.add_component(COMPONENT_TYPE_BOX_COLLIDER, id);
            }
            if (ImGui::MenuItem("Mesh Collider")) {
                m_editor.add_component(COMPONENT_TYPE_MESH_COLLIDER, id);
            }
            ImGui::EndMenu();
        }
        ImGui::EndPopup();
    }

    TransformComponent* transform_component = scene.getComponent<TransformComponent>(id);
    LightComponent* light_component = scene.getComponent<LightComponent>(id);
    ObjectComponent* object_component = scene.getComponent<ObjectComponent>(id);
    MeshComponent* mesh_component = object_component ? scene.getComponent<MeshComponent>(object_component->mesh_id) : nullptr;
    MaterialComponent* material_component = scene.getComponent<MaterialComponent>(id);
    RigidBodyComponent* rigid_body_component = scene.getComponent<RigidBodyComponent>(id);
    BoxColliderComponent* box_collider = scene.getComponent<BoxColliderComponent>(id);
    MeshColliderComponent* mesh_collider = scene.getComponent<MeshColliderComponent>(id);
    AnimationComponent* animation_component = scene.getComponent<AnimationComponent>(id);

    bool disable_translation = false;
    bool disable_rotation = false;
    bool disable_scale = false;
    if (light_component) {
        switch (light_component->get_type()) {
            case LIGHT_TYPE_OMNI:
                disable_translation = true;
                disable_scale = true;
                break;
            case LIGHT_TYPE_POINT:
                disable_rotation = true;
                disable_scale = true;
                break;
            case LIGHT_TYPE_AREA:
                break;
            default:
                CRASH_NOW();
                break;
        }
    }

    DrawComponent("Transform", transform_component, [&](TransformComponent& transform) {
        mat4 transformMatrix = transform.get_local_matrix();
        vec3 translation;
        vec3 rotation;
        vec3 scale;

        // @TODO: fix
        // DO NOT USE IMGUIZMO
        ImGuizmo::DecomposeMatrixToComponents(glm::value_ptr(transformMatrix), glm::value_ptr(translation),
                                              glm::value_ptr(rotation), glm::value_ptr(scale));

        bool dirty = false;

        dirty |= draw_vec3_control_disabled(disable_translation, "translation", translation);
        dirty |= draw_vec3_control_disabled(disable_rotation, "rotation", rotation);
        dirty |= draw_vec3_control_disabled(disable_scale, "scale", scale, 1.0f);
        if (dirty) {
            ImGuizmo::RecomposeMatrixFromComponents(glm::value_ptr(translation), glm::value_ptr(rotation),
                                                    glm::value_ptr(scale), glm::value_ptr(transformMatrix));
            // @TODO: change position, scale and rotation instead
            transform.set_local_transform(transformMatrix);
        }
    });

    DrawComponent("Light", light_component, [&](LightComponent& p_light) {
        bool dirty = false;

        switch (p_light.get_type()) {
            case LIGHT_TYPE_OMNI:
                ImGui::Text("omni light");
                break;
            case LIGHT_TYPE_POINT:
                ImGui::Text("point light");
                break;
            default:
                break;
        }

        bool cast_shadow = p_light.cast_shadow();
        ImGui::Checkbox("Cast shadow", &cast_shadow);
        if (cast_shadow != p_light.cast_shadow()) {
            p_light.set_cast_shadow(cast_shadow);
            p_light.set_dirty();
        }

        dirty |= draw_drag_float("constant", p_light.m_atten.constant, 0.1f, 0.0f, 1.0f);
        dirty |= draw_drag_float("linear", p_light.m_atten.linear, 0.1f, 0.0f, 1.0f);
        dirty |= draw_drag_float("quadratic", p_light.m_atten.quadratic, 0.1f, 0.0f, 1.0f);
        ImGui::Text("max distance: %0.3f", p_light.get_max_distance());
    });

    DrawComponent("RigidBody", rigid_body_component, [](RigidBodyComponent& rigidbody) {
        switch (rigidbody.shape) {
            case RigidBodyComponent::SHAPE_CUBE: {
                const auto& half = rigidbody.param.box.half_size;
                ImGui::Text("shape: box");
                ImGui::Text("half size: %.2f, %.2f, %.2f", half.x, half.y, half.z);
                break;
            }
            default:
                break;
        }
    });

    DrawComponent("Material", material_component, [](MaterialComponent& p_material) {
        vec3 color = p_material.base_color;
        if (draw_color_control("Color", color)) {
            p_material.base_color = vec4(color, p_material.base_color.a);
        }
        draw_drag_float("metallic", p_material.metallic, 0.01f, 0.0f, 1.0f);
        draw_drag_float("roughness", p_material.roughness, 0.01f, 0.0f, 1.0f);
        draw_drag_float("emissive:", p_material.emissive, 0.1f, 0.0f, 100.0f);
        for (int i = 0; i < MaterialComponent::TEXTURE_MAX; ++i) {
            auto& texture = p_material.textures[i];
            if (texture.path.empty()) {
                continue;
            }
            ImGui::Text("path: %s", texture.path.c_str());
            // @TODO: safer
            auto check_box_id = std::format("Enabled##{}", i);
            ImGui::Checkbox(check_box_id.c_str(), &texture.enabled);
            Image* image = texture.image ? texture.image->get() : nullptr;
            auto gpu_texture = image ? image->gpu_texture : nullptr;
            if (gpu_texture) {
                ImGui::Image((ImTextureID)gpu_texture->get_handle(), ImVec2(128, 128));
            }
        }
    });

    DrawComponent("Object", object_component, [&](ObjectComponent& object) {
        bool hide = !(object.flags & ObjectComponent::RENDERABLE);
        bool cast_shadow = object.flags & ObjectComponent::CAST_SHADOW;
        ImGui::Checkbox("Hide", &hide);
        ImGui::Checkbox("Cast shadow", &cast_shadow);
        object.flags = (hide ? 0 : ObjectComponent::RENDERABLE) | (cast_shadow ? ObjectComponent::CAST_SHADOW : 0);
    });

    DrawComponent("Mesh", mesh_component, [&](MeshComponent& mesh) {
        ImGui::Text("%zu triangles", mesh.indices.size() / 3);
        ImGui::Text("v:%zu, n:%zu, u:%zu, b:%zu", mesh.positions.size(), mesh.normals.size(),
                    mesh.texcoords_0.size(), mesh.weights_0.size());
    });

    DrawComponent("Box Collider", box_collider, [&](BoxColliderComponent& collider) {
        vec3 center = collider.box.center();
        vec3 size = collider.box.size();
        if (draw_vec3_control("size", size)) {
            collider.box = AABB::fromCenterSize(center, size);
        }
    });

    DrawComponent("Mesh Collider", mesh_collider, [&](MeshColliderComponent& collider) {
        char buffer[256];
        snprintf(buffer, sizeof(buffer), "%d", collider.object_id.getId());
        ImGui::Columns(2);
        ImGui::SetColumnWidth(0, kDefaultColumnWidth);
        ImGui::Text("Mesh ID");
        ImGui::NextColumn();
        ImGui::InputText("##ID", buffer, sizeof(buffer));
        ImGui::Columns(1);
        ecs::Entity entity{ (uint32_t)std::stoi(buffer) };
        if (scene.getComponent<ObjectComponent>(entity) != nullptr) {
            collider.object_id = entity;
        }
    });

    DrawComponent("Animation", animation_component, [&](AnimationComponent& p_animation) {
        if (!p_animation.is_playing()) {
            if (ImGui::Button("play")) {
                p_animation.flags |= AnimationComponent::PLAYING;
            }
        } else {
            if (ImGui::Button("stop")) {
                p_animation.flags &= ~AnimationComponent::PLAYING;
            }
        }
        if (ImGui::SliderFloat("Frame", &p_animation.timer, p_animation.start, p_animation.end)) {
            p_animation.flags |= AnimationComponent::PLAYING;
        }
        ImGui::Separator();
    });
}

}  // namespace my
