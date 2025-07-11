#include "propertiy_panel.h"

#include <ImGuizmo/ImGuizmo.h>

#include "editor/editor_layer.h"
#include "editor/widget.h"
#include "engine/runtime/asset_registry.h"
#include "engine/runtime/scene_manager.h"
#include "engine/core/string/string_utils.h"
#include "engine/renderer/graphics_dvars.h"

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
        bool open = ImGui::TreeNodeEx((void*)typeid(T).hash_code(), treeNodeFlags, "%s", p_name.c_str());
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

    DrawInputText("Name", name_component->GetNameRef());

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
        if (ImGui::MenuItem("Script")) {
            m_editor.AddComponent(ComponentType::SCRIPT, id);
        }
        ImGui::EndPopup();
    }

    TransformComponent* transform_component = p_scene.GetComponent<TransformComponent>(id);
    LightComponent* light_component = p_scene.GetComponent<LightComponent>(id);
    MeshRendererComponent* object_component = p_scene.GetComponent<MeshRendererComponent>(id);
    MeshComponent* mesh_component = object_component ? p_scene.GetComponent<MeshComponent>(object_component->meshId) : nullptr;
    MaterialComponent* material_component = p_scene.GetComponent<MaterialComponent>(id);
    RigidBodyComponent* rigid_body_component = p_scene.GetComponent<RigidBodyComponent>(id);
    AnimationComponent* animation_component = p_scene.GetComponent<AnimationComponent>(id);
    ParticleEmitterComponent* particle_emitter_component = p_scene.GetComponent<ParticleEmitterComponent>(id);
    MeshEmitterComponent* mesh_emitter_component = p_scene.GetComponent<MeshEmitterComponent>(id);
    ForceFieldComponent* force_field_component = p_scene.GetComponent<ForceFieldComponent>(id);
    LuaScriptComponent* script_component = p_scene.GetComponent<LuaScriptComponent>(id);
    CameraComponent* camera_component = p_scene.GetComponent<CameraComponent>(id);
    EnvironmentComponent* environment_component = p_scene.GetComponent<EnvironmentComponent>(id);
    VoxelGiComponent* voxel_gi_component = p_scene.GetComponent<VoxelGiComponent>(id);

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
        const Matrix4x4f old_transform = transform.GetLocalMatrix();
        Vector3f translation;
        Vector3f rotation;
        Vector3f scale;

        // @TODO: fix
        // DO NOT USE IMGUIZMO
        ImGuizmo::DecomposeMatrixToComponents(glm::value_ptr(old_transform), &translation.x,
                                              &rotation.x,
                                              &scale.x);

        bool dirty = false;
        CommandType command_type{};
        if (DrawVec3ControlDisabled(disable_translation, "translation", translation)) {
            dirty = true;
            command_type = COMMAND_TYPE_ENTITY_TRANSLATE;
        }
        if (DrawVec3ControlDisabled(disable_rotation, "rotation", rotation)) {
            dirty = true;
            command_type = COMMAND_TYPE_ENTITY_ROTATE;
        }
        if (DrawVec3ControlDisabled(disable_scale, "scale", scale, 1.0f)) {
            dirty = true;
            command_type = COMMAND_TYPE_ENTITY_SCALE;
        }
        if (dirty) {
            Matrix4x4f new_transform;
            ImGuizmo::RecomposeMatrixFromComponents(&translation.x,
                                                    &rotation.x,
                                                    &scale.x,
                                                    glm::value_ptr(new_transform));

            auto command = std::make_shared<EntityTransformCommand>(command_type, p_scene, id, old_transform, new_transform);
            m_editor.BufferCommand(command);
        }
    });

    DrawComponent("Light", light_component, [&](LightComponent& p_light) {
        bool dirty = false;
        unused(dirty);

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

    DrawComponent("Environment", environment_component, [](EnvironmentComponent& p_environment) {
        DrawInputText("texture", p_environment.sky.texturePath);
        ImGui::BeginDisabled(p_environment.sky.texturePath.empty());
        ImGui::EndDisabled();
        DrawColorPicker3("ambient", &p_environment.ambient.color.x);
    });

    DrawComponent("Script", script_component, [](LuaScriptComponent& p_script) {
        DrawInputText("class_name", p_script.GetClassNameRef());
        DrawInputText("path", p_script.GetPathRef());
    });

    DrawComponent("VoxelGi", voxel_gi_component, [](VoxelGiComponent& p_voxel_gi) {
        DrawCheckBoxBitflag("enabled", p_voxel_gi.flags, VoxelGiComponent::ENABLED);
        DrawCheckBoxBitflag("show_debug_box", p_voxel_gi.flags, VoxelGiComponent::SHOW_DEBUG_BOX);

        ImGui::Checkbox("debug", (bool*)(DVAR_GET_POINTER(gfx_debug_vxgi)));
        int value = DVAR_GET_INT(gfx_debug_vxgi_voxel);
        ImGui::RadioButton("lighting", &value, 0);
        ImGui::SameLine();
        ImGui::RadioButton("normal", &value, 1);
        DVAR_SET_INT(gfx_debug_vxgi_voxel, value);
    });

    DrawComponent("RigidBody", rigid_body_component, [](RigidBodyComponent& p_rigid_body) {
        const auto& size = p_rigid_body.size;
        switch (p_rigid_body.shape) {
            case RigidBodyComponent::SHAPE_CUBE: {
                ImGui::Text("shape: box");
                ImGui::Text("half size: %.2f, %.2f, %.2f", size.x, size.y, size.z);
            } break;
            case RigidBodyComponent::SHAPE_SPHERE: {
                ImGui::Text("shape: sphere");
                ImGui::Text("radius: %.2f", size.x);
            } break;
            default:
                break;
        }
    });

    DrawComponent("Material", material_component, [](MaterialComponent& p_material) {
        DrawColorPicker3("base color", &p_material.baseColor.x);
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

            // auto image = AssetRegistry::GetSingleton().Request(texture.path);
            //     //<ImageAsset>(texture.path);
            // auto gpu_texture = image ? image->gpu_texture : nullptr;
            // if (gpu_texture) {
            //     ImGui::Image((ImTextureID)gpu_texture->GetHandle(), ImVec2(128, 128));
            // }
        }
    });

    DrawComponent("Camera", camera_component, [&](CameraComponent& p_camera) {
        const bool is_editor = p_camera.IsEditorCamera();
        bool is_main = p_camera.IsPrimary();
        ImGui::BeginDisabled(is_editor);
        if (ImGui::Checkbox("main camera", &is_main)) {
            p_camera.SetPrimary(is_main);
        }
        ImGui::EndDisabled();

        bool is_ortho = p_camera.IsOrtho();
        if (ToggleButton("ortho", &is_ortho)) {
            p_camera.SetOrtho(is_ortho);
        }

        float near = p_camera.GetNear();
        if (DrawDragFloat("near", near, 0.1f, 0.1f, 9.0f)) {
            p_camera.SetNear(near);
        }
        float far = p_camera.GetFar();
        if (DrawDragFloat("far", far, 1.0f, 10.0f, 10000.0f)) {
            p_camera.SetFar(far);
        }
        float fovy = p_camera.GetFovy().GetDegree();
        if (DrawDragFloat("fov", fovy, 0.1f, 30.0f, 120.0f)) {
            p_camera.SetFovy(Degree(fovy));
        }
    });

    DrawComponent("Object", object_component, [&](MeshRendererComponent& p_object) {
        bool hide = !(p_object.flags & MeshRendererComponent::FLAG_RENDERABLE);
        bool cast_shadow = p_object.flags & MeshRendererComponent::FLAG_CAST_SHADOW;
        ImGui::Checkbox("Hide", &hide);
        ImGui::Checkbox("Cast shadow", &cast_shadow);
        p_object.flags = (hide ? 0 : MeshRendererComponent::FLAG_RENDERABLE);
        p_object.flags |= (cast_shadow ? MeshRendererComponent::FLAG_CAST_SHADOW : 0);
    });

    DrawComponent("Mesh", mesh_component, [&](MeshComponent& mesh) {
        ImGui::Text("%zu triangles", mesh.indices.size() / 3);
        ImGui::Text("v:%zu, n:%zu, u:%zu, b:%zu", mesh.positions.size(), mesh.normals.size(),
                    mesh.texcoords_0.size(), mesh.weights_0.size());
    });

    DrawComponent("Animation", animation_component, [&](AnimationComponent& p_animation) {
        if (!p_animation.IsPlaying()) {
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

    DrawComponent("ParticleEmitter", particle_emitter_component, [](ParticleEmitterComponent& p_emitter) {
        const float width = 100.0f;
        ImGui::Checkbox("Gravity", &p_emitter.gravity);
        DrawVec3Control("Velocity", p_emitter.startingVelocity, 0.0f, width);
        DrawDragInt("Max count", p_emitter.maxParticleCount, 1000.f, 1000, MAX_PARTICLE_COUNT, width);
        DrawDragInt("Emit per frame", p_emitter.particlesPerFrame, 10.0f, 1, 10000, width);
        DrawDragFloat("Scaling", p_emitter.particleScale, 0.01f, 0.01f, 10.0f, width);
        DrawDragFloat("Life span", p_emitter.particleLifeSpan, 0.1f, 0.1f, 10.0f, width);
        ImGui::Separator();
        DrawColorPicker3("base color", &p_emitter.color.x, width);
        DrawInputText("texture", p_emitter.texture, width);
    });

    DrawComponent("MeshEmitter", mesh_emitter_component, [](MeshEmitterComponent& p_emitter) {
        // const float width = 100.0f;
        if (ImGui::Button("reset")) {
            p_emitter.Reset();
        }

        DrawCheckBoxBitflag("run", p_emitter.flags, MeshEmitterComponent::RUNNING);
        DrawCheckBoxBitflag("recycle", p_emitter.flags, MeshEmitterComponent::RECYCLE);
    });

    DrawComponent("Force Field", force_field_component, [](ForceFieldComponent& p_force_field) {
        const float width = 120.0f;
        DrawDragFloat("Strength", p_force_field.strength, 0.1f, -10.0f, 10.0f, width);
        DrawDragFloat("Radius", p_force_field.radius, 0.1f, 0.1f, 100.0f, width);
    });
}

}  // namespace my
