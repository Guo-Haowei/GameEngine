#include "propertiy_panel.h"

// @TODO: remove this
#include "ImGuizmo/ImGuizmo.h"
#include "core/framework/scene_manager.h"
#include "core/string/string_utils.h"
#include "editor/editor_layer.h"
#include "editor/panels/panel_util.h"
#include "editor/widget.h"

namespace my {

template<typename T, typename UIFunction>
static void DrawComponent(const std::string& p_name, T* p_component, UIFunction p_function) {
    const ImGuiTreeNodeFlags treeNodeFlags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed |
                                             ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowItemOverlap |
                                             ImGuiTreeNodeFlags_FramePadding;
    if (p_component) {
        ImVec2 contentRegionAvailable = ImGui::GetContentRegionAvail();

        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 4, 4 });
        float line_height = GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.0f;
        ImGui::Separator();
        bool open = ImGui::TreeNodeEx((void*)typeid(T).hash_code(), treeNodeFlags, p_name.c_str());
        ImGui::PopStyleVar();
        ImGui::SameLine(contentRegionAvailable.x - line_height * 0.5f);
        if (ImGui::Button("-", ImVec2{ line_height, line_height })) {
            ImGui::OpenPopup("ComponentSettings");
        }

        bool should_remove_component = false;
        if (ImGui::BeginPopup("ComponentSettings")) {
            if (ImGui::MenuItem("remove component")) {
                should_remove_component = true;
            }

            ImGui::EndPopup();
        }

        if (open) {
            p_function(*p_component);
            ImGui::TreePop();
        }

        if (should_remove_component) {
            LOG_ERROR("TODO: implement remove component");
        }
    }
}

template<typename... Args>
static bool DrawVec3ControlDisabled(bool disabled, Args&&... args) {
    if (disabled) {
        PushDisabled();
    }
    bool dirty = DrawVec3Control(std::forward<Args>(args)...);
    if (disabled) {
        PopDisabled();
    }
    return dirty;
};

void PropertyPanel::UpdateInternal(Scene& p_scene) {
    ecs::Entity id = m_editor.GetSelectedEntity();

    if (!id.IsValid()) {
        return;
    }

    NameComponent* name_component = p_scene.GetComponent<NameComponent>(id);
    // @NOTE: when loading another scene, the selected entity will expire, thus don't have name
    if (!name_component) {
        // LOG_WARN("Entity {} does not have name", id.get_id());
        return;
    }

    panel_util::EditName(name_component);

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
                m_editor.AddComponent(COMPONENT_TYPE_BOX_COLLIDER, id);
            }
            if (ImGui::MenuItem("Mesh Collider")) {
                m_editor.AddComponent(COMPONENT_TYPE_MESH_COLLIDER, id);
            }
            ImGui::EndMenu();
        }
        ImGui::EndPopup();
    }

    TransformComponent* transform_component = p_scene.GetComponent<TransformComponent>(id);
    LightComponent* light_component = p_scene.GetComponent<LightComponent>(id);
    ObjectComponent* object_component = p_scene.GetComponent<ObjectComponent>(id);
    MeshComponent* mesh_component = object_component ? p_scene.GetComponent<MeshComponent>(object_component->meshId) : nullptr;
    MaterialComponent* material_component = p_scene.GetComponent<MaterialComponent>(id);
    RigidBodyComponent* rigid_body_component = p_scene.GetComponent<RigidBodyComponent>(id);
    BoxColliderComponent* box_collider = p_scene.GetComponent<BoxColliderComponent>(id);
    MeshColliderComponent* mesh_collider = p_scene.GetComponent<MeshColliderComponent>(id);
    AnimationComponent* animation_component = p_scene.GetComponent<AnimationComponent>(id);

    bool disable_translation = false;
    bool disable_rotation = false;
    bool disable_scale = false;
    if (light_component) {
        switch (light_component->GetType()) {
            case LIGHT_TYPE_INFINITE:
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
        mat4 transformMatrix = transform.GetLocalMatrix();
        vec3 translation;
        vec3 rotation;
        vec3 scale;

        // @TODO: fix
        // DO NOT USE IMGUIZMO
        ImGuizmo::DecomposeMatrixToComponents(glm::value_ptr(transformMatrix), glm::value_ptr(translation),
                                              glm::value_ptr(rotation), glm::value_ptr(scale));

        bool dirty = false;

        dirty |= DrawVec3ControlDisabled(disable_translation, "translation", translation);
        dirty |= DrawVec3ControlDisabled(disable_rotation, "rotation", rotation);
        dirty |= DrawVec3ControlDisabled(disable_scale, "scale", scale, 1.0f);
        if (dirty) {
            ImGuizmo::RecomposeMatrixFromComponents(glm::value_ptr(translation), glm::value_ptr(rotation),
                                                    glm::value_ptr(scale), glm::value_ptr(transformMatrix));
            // @TODO: change position, scale and rotation instead
            transform.SetLocalTransform(transformMatrix);
        }
    });

    DrawComponent("Light", light_component, [&](LightComponent& p_light) {
        bool dirty = false;

        switch (p_light.GetType()) {
            case LIGHT_TYPE_INFINITE:
                ImGui::Text("infinite light");
                break;
            case LIGHT_TYPE_POINT:
                ImGui::Text("point light");
                break;
            default:
                break;
        }

        bool cast_shadow = p_light.CastShadow();
        ImGui::Checkbox("Cast shadow", &cast_shadow);
        if (cast_shadow != p_light.CastShadow()) {
            p_light.SetCastShadow(cast_shadow);
            p_light.SetDirty();
        }

        dirty |= DrawDragFloat("constant", p_light.m_atten.constant, 0.1f, 0.0f, 1.0f);
        dirty |= DrawDragFloat("linear", p_light.m_atten.linear, 0.1f, 0.0f, 1.0f);
        dirty |= DrawDragFloat("quadratic", p_light.m_atten.quadratic, 0.1f, 0.0f, 1.0f);
        ImGui::Text("max distance: %0.3f", p_light.GetMaxDistance());
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
        if (DrawColorControl("Color", color)) {
            p_material.base_color = vec4(color, p_material.base_color.a);
        }
        DrawDragFloat("metallic", p_material.metallic, 0.01f, 0.0f, 1.0f);
        DrawDragFloat("roughness", p_material.roughness, 0.01f, 0.0f, 1.0f);
        DrawDragFloat("emissive:", p_material.emissive, 0.1f, 0.0f, 100.0f);
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
        if (DrawVec3Control("size", size)) {
            collider.box = AABB::fromCenterSize(center, size);
        }
    });

    DrawComponent("Mesh Collider", mesh_collider, [&](MeshColliderComponent& collider) {
        char buffer[256];
        StringUtils::Sprintf(buffer, "%d", collider.objectId.GetId());
        ImGui::Columns(2);
        ImGui::SetColumnWidth(0, DEFAULT_COLUMN_WIDTH);
        ImGui::Text("Mesh ID");
        ImGui::NextColumn();
        ImGui::InputText("##ID", buffer, sizeof(buffer));
        ImGui::Columns(1);
        ecs::Entity entity{ (uint32_t)std::stoi(buffer) };
        if (p_scene.GetComponent<ObjectComponent>(entity) != nullptr) {
            collider.objectId = entity;
        }
    });

    DrawComponent("Animation", animation_component, [&](AnimationComponent& p_animation) {
        if (!p_animation.isPlaying()) {
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
