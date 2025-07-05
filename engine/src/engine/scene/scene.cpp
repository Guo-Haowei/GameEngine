#include "scene.h"

#include "engine/core/debugger/profiler.h"
#include "engine/core/io/archive.h"
#include "engine/math/geometry.h"
#include "engine/renderer/renderer.h"
#include "engine/runtime/asset_registry.h"
#include "engine/scene/scene_system.h"
#include "engine/systems/ecs/component_manager.inl"
#include "engine/systems/job_system/job_system.h"

// @TODO: refactor
#include "engine/renderer/graphics_dvars.h"
#include "engine/renderer/path_tracer/bvh_accel.h"

namespace my::ecs {

// instantiate ComponentManagers
#define REGISTER_COMPONENT(TYPE, ...) template class ComponentManager<TYPE>;
REGISTER_COMPONENT_LIST
#undef REGISTER_COMPONENT

}  // namespace my::ecs

namespace my {

using jobsystem::Context;

static constexpr uint32_t SMALL_SUBTASK_GROUP_SIZE = 64;

#define JS_FORCE_PARALLEL_FOR(TYPE, CTX, INDEX, SUBCOUNT, BODY) \
    CTX.Dispatch(                                               \
        static_cast<uint32_t>(GetCount<TYPE>()),                \
        SUBCOUNT,                                               \
        [&](jobsystem::JobArgs args) { const uint32_t INDEX = args.jobIndex; do { BODY; } while(0); })

#define JS_NO_PARALLEL_FOR(TYPE, CTX, INDEX, SUBCOUNT, BODY)    \
    (void)(CTX);                                                \
    for (size_t INDEX = 0; INDEX < GetCount<TYPE>(); ++INDEX) { \
        BODY;                                                   \
    }

#if USING(ENABLE_JOB_SYSTEM)
#define JS_PARALLEL_FOR JS_FORCE_PARALLEL_FOR
#else
#define JS_PARALLEL_FOR JS_NO_PARALLEL_FOR
#endif

void Scene::Update(float p_time_step) {
    HBN_PROFILE_EVENT();

    m_timestep = p_time_step;
    m_dirtyFlags.store(0);

    Context ctx;
    // animation
    RunLightUpdateSystem(ctx);
    RunAnimationUpdateSystem(ctx);
    ctx.Wait();
    // transform, update local matrix from position, rotation and scale
    RunTransformationUpdateSystem(ctx);
    ctx.Wait();
    // hierarchy, update world matrix based on hierarchy
    RunHierarchyUpdateSystem(ctx);
    ctx.Wait();
    // mesh particles
    RunMeshEmitterUpdateSystem(ctx);
    // particle
    RunParticleEmitterUpdateSystem(ctx);
    // armature
    RunArmatureUpdateSystem(ctx);
    ctx.Wait();

    // update bounding box
    RunObjectUpdateSystem(ctx);

    // @TODO: refactor
    for (auto [entity, camera] : m_PerspectiveCameraComponents) {
        if (camera.Update()) {
            m_dirtyFlags.fetch_or(SCENE_DIRTY_CAMERA);
        }
    }

    for (auto [entity, voxel_gi] : m_VoxelGiComponents) {
        auto transform = GetComponent<TransformComponent>(entity);
        if (DEV_VERIFY(transform)) {
            const auto& matrix = transform->GetWorldMatrix();
            Vector3f center{ matrix[3].x, matrix[3].y, matrix[3].z };
            Vector3f scale = transform->GetScale();
            const float size = glm::max(scale.x, glm::max(scale.y, scale.z));
            voxel_gi.region = AABB::FromCenterSize(center, Vector3f(size));
        }
    }

    // @TODO: refactor
    if (DVAR_GET_BOOL(gfx_bvh_generate)) {
        for (auto [entity, mesh] : m_MeshComponents) {
            if (!mesh.bvh) {
                mesh.bvh = BvhAccel::Construct(mesh.indices, mesh.positions);
            }
        }
        DVAR_SET_BOOL(gfx_bvh_generate, false);
    }
}

void Scene::Copy(Scene& p_other) {
    for (auto& entry : m_componentLib.m_entries) {
        auto& manager = *p_other.m_componentLib.m_entries[entry.first].m_manager;
        entry.second.m_manager->Copy(manager);
    }

    m_root = p_other.m_root;
    m_bound = p_other.m_bound;
    m_timestep = p_other.m_timestep;
    m_physicsMode = p_other.m_physicsMode;
}

void Scene::Merge(Scene& p_other) {
    for (auto& entry : m_componentLib.m_entries) {
        auto& manager = *p_other.m_componentLib.m_entries[entry.first].m_manager;
        entry.second.m_manager->Merge(manager);
    }
    if (p_other.m_root.IsValid()) {
        AttachChild(p_other.m_root, m_root);
    }

    m_bound.UnionBox(p_other.m_bound);
}

ecs::Entity Scene::GetMainCamera() {
    for (auto [entity, camera] : m_PerspectiveCameraComponents) {
        if (camera.IsPrimary()) {
            return entity;
        }
    }

    return ecs::Entity::INVALID;
}

ecs::Entity Scene::GetEditorCamera() {
    for (auto [entity, camera] : m_PerspectiveCameraComponents) {
        if (camera.IsEditorCamera()) {
            return entity;
        }
    }

    return ecs::Entity::INVALID;
}

ecs::Entity Scene::CreatePerspectiveCameraEntity(const std::string& p_name,
                                                 int p_width,
                                                 int p_height,
                                                 float p_near_plane,
                                                 float p_far_plane,
                                                 Degree p_fovy) {
    auto entity = CreateNameEntity(p_name);
    PerspectiveCameraComponent& camera = Create<PerspectiveCameraComponent>(entity);

    camera.m_width = p_width;
    camera.m_height = p_height;
    camera.m_near = p_near_plane;
    camera.m_far = p_far_plane;
    camera.m_fovy = p_fovy;
    camera.m_pitch = Degree{ -10.0f };
    camera.m_yaw = Degree{ -90.0f };
    camera.SetDirty();
    return entity;
}

ecs::Entity Scene::CreateNameEntity(const std::string& p_name) {
    auto entity = ecs::Entity::Create();
    Create<NameComponent>(entity).SetName(p_name);
    return entity;
}

ecs::Entity Scene::CreateTransformEntity(const std::string& p_name) {
    auto entity = CreateNameEntity(p_name);
    Create<TransformComponent>(entity);
    return entity;
}

ecs::Entity Scene::CreateObjectEntity(const std::string& p_name) {
    auto entity = CreateNameEntity(p_name);
    Create<ObjectComponent>(entity);
    Create<TransformComponent>(entity);
    return entity;
}

ecs::Entity Scene::CreateMeshEntity(const std::string& p_name) {
    auto entity = CreateNameEntity(p_name);
    Create<MeshComponent>(entity);
    return entity;
}

ecs::Entity Scene::CreateMaterialEntity(const std::string& p_name) {
    auto entity = CreateNameEntity(p_name);
    Create<MaterialComponent>(entity);
    return entity;
}

ecs::Entity Scene::CreatePointLightEntity(const std::string& p_name,
                                          const Vector3f& p_position,
                                          const Vector3f& p_color,
                                          const float p_emissive) {
    auto entity = CreateObjectEntity(p_name);

    LightComponent& light = Create<LightComponent>(entity);
    light.SetType(LIGHT_TYPE_POINT);
    light.m_atten.constant = 1.0f;
    light.m_atten.linear = 0.2f;
    light.m_atten.quadratic = 0.05f;

    MaterialComponent& material = Create<MaterialComponent>(entity);
    material.baseColor = Vector4f(p_color, 1.0f);
    material.emissive = p_emissive;

    TransformComponent& transform = *GetComponent<TransformComponent>(entity);
    ObjectComponent& object = *GetComponent<ObjectComponent>(entity);
    transform.SetTranslation(p_position);
    transform.SetDirty();

    auto mesh_id = CreateMeshEntity(p_name + ":mesh");
    object.meshId = mesh_id;
    object.flags = ObjectComponent::FLAG_RENDERABLE;

    MeshComponent& mesh = *GetComponent<MeshComponent>(mesh_id);
    mesh = MakeSphereMesh(0.1f, 40, 40);
    mesh.subsets[0].material_id = entity;
    return entity;
}

ecs::Entity Scene::CreateAreaLightEntity(const std::string& p_name,
                                         const Vector3f& p_color,
                                         const float p_emissive) {
    auto entity = CreateObjectEntity(p_name);

    // light
    LightComponent& light = Create<LightComponent>(entity);
    light.SetType(LIGHT_TYPE_AREA);
    // light.m_atten.constant = 1.0f;
    // light.m_atten.linear = 0.09f;
    // light.m_atten.quadratic = 0.032f;

    // material
    MaterialComponent& material = Create<MaterialComponent>(entity);
    material.baseColor = Vector4f(p_color, 1.0f);
    material.emissive = p_emissive;

    ObjectComponent& object = *GetComponent<ObjectComponent>(entity);

    auto mesh_id = CreateMeshEntity(p_name + ":mesh");
    object.meshId = mesh_id;
    object.flags = ObjectComponent::FLAG_RENDERABLE;

    MeshComponent& mesh = *GetComponent<MeshComponent>(mesh_id);
    mesh = MakePlaneMesh();
    mesh.subsets[0].material_id = entity;
    return entity;
}

ecs::Entity Scene::CreateInfiniteLightEntity(const std::string& p_name,
                                             const Vector3f& p_color,
                                             const float p_emissive) {
    auto entity = CreateNameEntity(p_name);

    Create<TransformComponent>(entity);

    LightComponent& light = Create<LightComponent>(entity);
    light.SetType(LIGHT_TYPE_INFINITE);
    light.m_atten.constant = 1.0f;
    light.m_atten.linear = 0.0f;
    light.m_atten.quadratic = 0.0f;

    MaterialComponent& material = Create<MaterialComponent>(entity);
    material.baseColor = Vector4f(p_color, 1.0f);
    material.emissive = p_emissive;
    return entity;
}

ecs::Entity Scene::CreateEnvironmentEntity(const std::string& p_name) {
    auto entity = CreateNameEntity(p_name);
    Create<EnvironmentComponent>(entity);
    return entity;
}

ecs::Entity Scene::CreateVoxelGiEntity(const std::string& p_name) {
    auto entity = CreateNameEntity(p_name);
    Create<VoxelGiComponent>(entity);
    Create<TransformComponent>(entity);
    return entity;
}

ecs::Entity Scene::CreatePlaneEntity(const std::string& p_name,
                                     const Vector3f& p_scale,
                                     const Matrix4x4f& p_transform) {
    auto material_id = CreateMaterialEntity(p_name + ":mat");
    return CreatePlaneEntity(p_name, material_id, p_scale, p_transform);
}

ecs::Entity Scene::CreatePlaneEntity(const std::string& p_name,
                                     ecs::Entity p_material_id,
                                     const Vector3f& p_scale,
                                     const Matrix4x4f& p_transform) {
    auto entity = CreateObjectEntity(p_name);
    TransformComponent& trans = *GetComponent<TransformComponent>(entity);
    ObjectComponent& object = *GetComponent<ObjectComponent>(entity);
    trans.MatrixTransform(p_transform);

    auto mesh_id = CreateMeshEntity(p_name + ":mesh");
    object.meshId = mesh_id;

    MeshComponent& mesh = *GetComponent<MeshComponent>(mesh_id);
    mesh = MakePlaneMesh(p_scale);
    mesh.subsets[0].material_id = p_material_id;

    return entity;
}

ecs::Entity Scene::CreateCubeEntity(const std::string& p_name,
                                    const Vector3f& p_scale,
                                    const Matrix4x4f& p_transform) {
    auto material_id = CreateMaterialEntity(p_name + ":mat");
    return CreateCubeEntity(p_name, material_id, p_scale, p_transform);
}

ecs::Entity Scene::CreateCubeEntity(const std::string& p_name,
                                    ecs::Entity p_material_id,
                                    const Vector3f& p_scale,
                                    const Matrix4x4f& p_transform) {
    auto entity = CreateObjectEntity(p_name);
    TransformComponent& trans = *GetComponent<TransformComponent>(entity);
    ObjectComponent& object = *GetComponent<ObjectComponent>(entity);
    trans.MatrixTransform(p_transform);

    auto mesh_id = CreateMeshEntity(p_name + ":mesh");
    object.meshId = mesh_id;

    MeshComponent& mesh = *GetComponent<MeshComponent>(mesh_id);
    mesh = MakeCubeMesh(p_scale);
    mesh.subsets[0].material_id = p_material_id;

    return entity;
}

ecs::Entity Scene::CreateMeshEntity(const std::string& p_name,
                                    ecs::Entity p_material_id,
                                    MeshComponent&& p_mesh) {
    auto entity = CreateObjectEntity(p_name);
    ObjectComponent& object = *GetComponent<ObjectComponent>(entity);

    auto mesh_id = CreateMeshEntity(p_name + ":mesh");
    object.meshId = mesh_id;

    MeshComponent& mesh = *GetComponent<MeshComponent>(mesh_id);
    mesh = std::move(p_mesh);
    mesh.subsets[0].material_id = p_material_id;
    return entity;
}

ecs::Entity Scene::CreateSphereEntity(const std::string& p_name,
                                      float p_radius,
                                      const Matrix4x4f& p_transform) {
    auto material_id = CreateMaterialEntity(p_name + ":mat");
    return CreateSphereEntity(p_name, material_id, p_radius, p_transform);
}

ecs::Entity Scene::CreateSphereEntity(const std::string& p_name,
                                      ecs::Entity p_material_id,
                                      float p_radius,
                                      const Matrix4x4f& p_transform) {
    auto entity = CreateObjectEntity(p_name);
    TransformComponent& transform = *GetComponent<TransformComponent>(entity);
    transform.MatrixTransform(p_transform);

    ObjectComponent& object = *GetComponent<ObjectComponent>(entity);
    auto mesh_id = CreateMeshEntity(p_name + ":mesh");
    object.meshId = mesh_id;

    MeshComponent& mesh = *GetComponent<MeshComponent>(mesh_id);
    mesh = MakeSphereMesh(p_radius);
    mesh.subsets[0].material_id = p_material_id;

    return entity;
}

ecs::Entity Scene::CreateCylinderEntity(const std::string& p_name,
                                        float p_radius,
                                        float p_height,
                                        const Matrix4x4f& p_transform) {
    auto material_id = CreateMaterialEntity(p_name + ":mat");
    return CreateCylinderEntity(p_name, material_id, p_radius, p_height, p_transform);
}

ecs::Entity Scene::CreateCylinderEntity(const std::string& p_name,
                                        ecs::Entity p_material_id,
                                        float p_radius,
                                        float p_height,
                                        const Matrix4x4f& p_transform) {
    auto entity = CreateObjectEntity(p_name);
    TransformComponent& transform = *GetComponent<TransformComponent>(entity);
    transform.MatrixTransform(p_transform);

    ObjectComponent& object = *GetComponent<ObjectComponent>(entity);
    auto mesh_id = CreateMeshEntity(p_name + ":mesh");
    object.meshId = mesh_id;

    MeshComponent& mesh = *GetComponent<MeshComponent>(mesh_id);
    mesh = MakeCylinderMesh(p_radius, p_height);
    mesh.subsets[0].material_id = p_material_id;

    return entity;
}

ecs::Entity Scene::CreateTorusEntity(const std::string& p_name,
                                     float p_radius,
                                     float p_tube_radius,
                                     const Matrix4x4f& p_transform) {
    auto material_id = CreateMaterialEntity(p_name + ":mat");
    return CreateTorusEntity(p_name, material_id, p_radius, p_tube_radius, p_transform);
}

ecs::Entity Scene::CreateTorusEntity(const std::string& p_name,
                                     ecs::Entity p_material_id,
                                     float p_radius,
                                     float p_tube_radius,
                                     const Matrix4x4f& p_transform) {
    // @TODO: fix this
    p_radius = 0.4f;
    p_tube_radius = 0.1f;

    auto entity = CreateObjectEntity(p_name);
    TransformComponent& transform = *GetComponent<TransformComponent>(entity);
    transform.MatrixTransform(p_transform);

    ObjectComponent& object = *GetComponent<ObjectComponent>(entity);
    auto mesh_id = CreateMeshEntity(p_name + ":mesh");
    object.meshId = mesh_id;

    MeshComponent& mesh = *GetComponent<MeshComponent>(mesh_id);
    mesh = MakeTorusMesh(p_radius, p_tube_radius);
    mesh.subsets[0].material_id = p_material_id;

    return entity;
}

ecs::Entity Scene::CreateClothEntity(const std::string& p_name,
                                     ecs::Entity p_material_id,
                                     const Vector3f& p_point_0,
                                     const Vector3f& p_point_1,
                                     const Vector3f& p_point_2,
                                     const Vector3f& p_point_3,
                                     const Vector2i& p_res,
                                     ClothFixFlag p_fixed_flags,
                                     const Matrix4x4f& p_transform) {
    // @TODO: fix
    unused(p_transform);

    auto entity = CreateObjectEntity(p_name);
    // TransformComponent& transform = *GetComponent<TransformComponent>(entity);
    // transform.MatrixTransform(p_transform);

    ClothComponent& cloth = Create<ClothComponent>(entity);
    cloth.point_0 = p_point_0;
    cloth.point_1 = p_point_1;
    cloth.point_2 = p_point_2;
    cloth.point_3 = p_point_3;
    cloth.res = p_res;
    cloth.fixedFlags = p_fixed_flags;

    ObjectComponent& object = *GetComponent<ObjectComponent>(entity);
    auto mesh_id = CreateMeshEntity(p_name + ":mesh");
    object.meshId = mesh_id;

    MeshComponent& mesh = *GetComponent<MeshComponent>(mesh_id);
    mesh = MakePlaneMesh(p_point_0,
                         p_point_1,
                         p_point_2,
                         p_point_3);
    mesh.subsets[0].material_id = p_material_id;

    return entity;
}

ecs::Entity Scene::CreateEmitterEntity(const std::string& p_name, const Matrix4x4f& p_transform) {
    auto entity = CreateTransformEntity(p_name);
    Create<ParticleEmitterComponent>(entity);

    TransformComponent& transform = *GetComponent<TransformComponent>(entity);
    transform.MatrixTransform(p_transform);
    return entity;
}

ecs::Entity Scene::CreateMeshEmitterEntity(const std::string& p_name, const Vector3f& p_translation) {
    auto entity = CreateNameEntity(p_name);
    Create<TransformComponent>(entity).SetTranslation(p_translation);
    Create<MeshEmitterComponent>(entity);
    return entity;
}

ecs::Entity Scene::CreateForceFieldEntity(const std::string& p_name, const Matrix4x4f& p_transform) {
    auto entity = CreateTransformEntity(p_name);
    Create<ForceFieldComponent>(entity);

    TransformComponent& transform = *GetComponent<TransformComponent>(entity);
    transform.MatrixTransform(p_transform);

    return entity;
}

ecs::Entity Scene::FindEntityByName(const char* p_name) {
    for (auto [entity, name] : m_NameComponents) {
        if (name.GetName() == p_name) {
            return entity;
        }
    }
    return ecs::Entity::INVALID;
}

void Scene::AttachChild(ecs::Entity p_child, ecs::Entity p_parent) {
    DEV_ASSERT(p_child != p_parent);
    DEV_ASSERT(p_parent.IsValid());

    // if child already has a parent, detach it
    if (Contains<HierarchyComponent>(p_child)) {
        CRASH_NOW_MSG("Unlikely to happen at this point");
    }

    HierarchyComponent& hier = Create<HierarchyComponent>(p_child);
    hier.m_parentId = p_parent;
}

void Scene::RemoveEntity(ecs::Entity p_entity) {
    std::vector<ecs::Entity> children;
    for (auto [child, hierarchy] : m_HierarchyComponents) {
        if (hierarchy.GetParent() == p_entity) {
            children.emplace_back(child);
        }
    }
    for (auto child : children) {
        RemoveEntity(child);
    }

    LightComponent* light = GetComponent<LightComponent>(p_entity);
    if (light) {
        auto shadow_handle = light->GetShadowMapIndex();
        if (shadow_handle != renderer::INVALID_POINT_SHADOW_HANDLE) {
            renderer::FreePointLightShadowMap(shadow_handle);
        }
        m_LightComponents.Remove(p_entity);
    }
    m_HierarchyComponents.Remove(p_entity);
    m_TransformComponents.Remove(p_entity);
    m_ObjectComponents.Remove(p_entity);
    m_ParticleEmitterComponents.Remove(p_entity);
    m_ForceFieldComponents.Remove(p_entity);
    m_NameComponents.Remove(p_entity);
}

void Scene::UpdateAnimation(size_t p_index) {
    AnimationComponent& animation = GetComponentByIndex<AnimationComponent>(p_index);

    if (!animation.IsPlaying()) {
        return;
    }

    for (const AnimationComponent::Channel& channel : animation.channels) {
        if (channel.path == AnimationComponent::Channel::PATH_UNKNOWN) {
            continue;
        }
        DEV_ASSERT(channel.samplerIndex < (int)animation.samplers.size());
        const AnimationComponent::Sampler& sampler = animation.samplers[channel.samplerIndex];

        int key_left = 0;
        int key_right = 0;
        float time_first = std::numeric_limits<float>::min();
        float time_last = std::numeric_limits<float>::min();
        float time_left = std::numeric_limits<float>::min();
        float time_right = std::numeric_limits<float>::max();

        for (int k = 0; k < (int)sampler.keyframeTimes.size(); ++k) {
            const float time = sampler.keyframeTimes[k];
            if (time < time_first) {
                time_first = time;
            }
            if (time > time_last) {
                time_last = time;
            }
            if (time <= animation.timer && time > time_left) {
                time_left = time;
                key_left = k;
            }
            if (time >= animation.timer && time < time_right) {
                time_right = time;
                key_right = k;
            }
        }

        if (animation.timer < time_first) {
            continue;
        }

        const float left = sampler.keyframeTimes[key_left];
        const float right = sampler.keyframeTimes[key_right];

        float t = 0;
        if (key_left != key_right) {
            t = (animation.timer - left) / (right - left);
        }
        t = Saturate(t);

        TransformComponent* targetTransform = GetComponent<TransformComponent>(channel.targetId);
        DEV_ASSERT(targetTransform);
        auto dummy_mix = [](const Vector3f& a, const Vector3f& b, float t) {
            glm::vec3 tmp = glm::mix(glm::vec3(a.x, a.y, a.z), glm::vec3(b.x, b.y, b.z), t);
            return Vector3f(tmp.x, tmp.y, tmp.z);
        };
        auto dummy_mix_4 = [](const Vector4f& a, const Vector4f& b, float t) {
            glm::vec4 tmp = glm::mix(glm::vec4(a.x, a.y, a.z, a.w), glm::vec4(b.x, b.y, b.z, b.w), t);
            return Vector4f(tmp.x, tmp.y, tmp.z, tmp.w);
        };
        switch (channel.path) {
            case AnimationComponent::Channel::PATH_SCALE: {
                DEV_ASSERT(sampler.keyframeData.size() == sampler.keyframeTimes.size() * 3);
                const Vector3f* data = (const Vector3f*)sampler.keyframeData.data();
                const Vector3f& vLeft = data[key_left];
                const Vector3f& vRight = data[key_right];
                targetTransform->SetScale(dummy_mix(vLeft, vRight, t));
                break;
            }
            case AnimationComponent::Channel::PATH_TRANSLATION: {
                DEV_ASSERT(sampler.keyframeData.size() == sampler.keyframeTimes.size() * 3);
                const Vector3f* data = (const Vector3f*)sampler.keyframeData.data();
                const Vector3f& vLeft = data[key_left];
                const Vector3f& vRight = data[key_right];
                targetTransform->SetTranslation(dummy_mix(vLeft, vRight, t));
                break;
            }
            case AnimationComponent::Channel::PATH_ROTATION: {
                DEV_ASSERT(sampler.keyframeData.size() == sampler.keyframeTimes.size() * 4);
                const Vector4f* data = (const Vector4f*)sampler.keyframeData.data();
                const Vector4f& vLeft = data[key_left];
                const Vector4f& vRight = data[key_right];
                targetTransform->SetRotation(dummy_mix_4(vLeft, vRight, t));
                break;
            }
            default:
                CRASH_NOW();
                break;
        }
        targetTransform->SetDirty();
    }

    if (animation.IsLooped() && animation.timer > animation.end) {
        animation.timer = animation.start;
    }

    if (animation.IsPlaying()) {
        animation.timer += m_timestep * animation.speed;
    }
}

void Scene::UpdateHierarchy(size_t p_index) {
    ecs::Entity self_id = GetEntityByIndex<HierarchyComponent>(p_index);
    TransformComponent* self_transform = GetComponent<TransformComponent>(self_id);

    if (!self_transform) {
        return;
    }

    Matrix4x4f world_matrix = self_transform->GetLocalMatrix();
    const HierarchyComponent* hierarchy = &GetComponentByIndex<HierarchyComponent>(p_index);
    ecs::Entity parent = hierarchy->m_parentId;

    while (parent.IsValid()) {
        TransformComponent* parent_transform = GetComponent<TransformComponent>(parent);
        if (DEV_VERIFY(parent_transform)) {
            world_matrix = parent_transform->GetLocalMatrix() * world_matrix;

            if ((hierarchy = GetComponent<HierarchyComponent>(parent)) != nullptr) {
                parent = hierarchy->m_parentId;
                DEV_ASSERT(parent.IsValid());
            } else {
                parent.MakeInvalid();
            }
        } else {
            break;
        }
    }

    self_transform->SetWorldMatrix(world_matrix);
    self_transform->SetDirty(false);
}

void Scene::UpdateArmature(size_t p_index) {
    TransformComponent* transform = GetComponent<TransformComponent>(GetEntityByIndex<ArmatureComponent>(p_index));
    DEV_ASSERT(transform);

    // The transform world matrices are in world space, but skinning needs them in armature-local space,
    //	so that the skin is reusable for instanced meshes.
    //	We remove the armature's world matrix from the bone world matrix to obtain the bone local transform
    //	These local bone matrices will only be used for skinning, the actual transform components for the bones
    //	remain unchanged.
    //
    //	This is useful for an other thing too:
    //	If a whole transform tree is transformed by some parent (even gltf import does that to convert from RH
    // to LH space) 	then the inverseBindMatrices are not reflected in that because they are not contained in
    // the hierarchy system. 	But this will correct them too.

    ArmatureComponent& armature = GetComponentByIndex<ArmatureComponent>(p_index);
    const Matrix4x4f R = glm::inverse(transform->GetWorldMatrix());
    const size_t numBones = armature.boneCollection.size();
    if (armature.boneTransforms.size() != numBones) {
        armature.boneTransforms.resize(numBones);
    }

    int idx = 0;
    for (ecs::Entity boneID : armature.boneCollection) {
        const TransformComponent* boneTransform = GetComponent<TransformComponent>(boneID);
        DEV_ASSERT(boneTransform);

        const Matrix4x4f& B = armature.inverseBindMatrices[idx];
        const Matrix4x4f& W = boneTransform->GetWorldMatrix();
        const Matrix4x4f M = R * W * B;
        armature.boneTransforms[idx] = M;
        ++idx;

        // @TODO: armature animation
    }
};

bool Scene::RayObjectIntersect(ecs::Entity p_object_id, Ray& p_ray) {
    ObjectComponent* object = GetComponent<ObjectComponent>(p_object_id);
    MeshComponent* mesh = GetComponent<MeshComponent>(object->meshId);
    TransformComponent* transform = GetComponent<TransformComponent>(p_object_id);
    DEV_ASSERT(mesh && transform);

    if (!transform || !mesh) {
        return false;
    }

    Matrix4x4f inversedModel = glm::inverse(transform->GetWorldMatrix());
    Ray inversedRay = p_ray.Inverse(inversedModel);
    Ray inversedRayAABB = inversedRay;  // make a copy, we don't want dist to be modified by AABB
    // Perform aabb test
    if (!inversedRayAABB.Intersects(mesh->localBound)) {
        return false;
    }

    // @TODO: test submesh intersection

    // Test every single triange
    for (size_t i = 0; i < mesh->indices.size(); i += 3) {
        const Vector3f& A = mesh->positions[mesh->indices[i]];
        const Vector3f& B = mesh->positions[mesh->indices[i + 1]];
        const Vector3f& C = mesh->positions[mesh->indices[i + 2]];
#define CC(a) Vector3f(a.x, a.y, a.z)
        if (inversedRay.Intersects(CC(A), CC(B), CC(C))) {
#undef CC
            p_ray.CopyDist(inversedRay);
            return true;
        }
    }
    return false;
}

Scene::RayIntersectionResult Scene::Intersects(Ray& p_ray) {
    RayIntersectionResult result;

    // @TODO: box collider
    for (size_t object_idx = 0; object_idx < GetCount<ObjectComponent>(); ++object_idx) {
        ecs::Entity entity = GetEntity<ObjectComponent>(object_idx);
        if (RayObjectIntersect(entity, p_ray)) {
            result.entity = entity;
        }
    }

    return result;
}

void Scene::RunLightUpdateSystem(Context& p_context) {
    HBN_PROFILE_EVENT();
    unused(p_context);

    for (auto [id, light] : m_LightComponents) {
        const TransformComponent* transform = GetComponent<TransformComponent>(id);
        if (DEV_VERIFY(transform)) {
            UpdateLight(m_timestep, *transform, light);
        }
    }
}

void Scene::RunTransformationUpdateSystem(Context& p_context) {
    HBN_PROFILE_EVENT();
    JS_PARALLEL_FOR(TransformComponent, p_context, index, SMALL_SUBTASK_GROUP_SIZE, {
        if (GetComponentByIndex<TransformComponent>(index).UpdateTransform()) {
            m_dirtyFlags.fetch_or(SCENE_DIRTY_WORLD);
        }
    });
}

void Scene::RunAnimationUpdateSystem(Context& p_context) {
    HBN_PROFILE_EVENT();
    JS_PARALLEL_FOR(AnimationComponent, p_context, index, 1, UpdateAnimation(index));
}

void Scene::RunArmatureUpdateSystem(Context& p_context) {
    HBN_PROFILE_EVENT();
    JS_PARALLEL_FOR(ArmatureComponent, p_context, index, 1, UpdateArmature(index));
}

void Scene::RunHierarchyUpdateSystem(Context& p_context) {
    HBN_PROFILE_EVENT();
    JS_PARALLEL_FOR(HierarchyComponent, p_context, index, SMALL_SUBTASK_GROUP_SIZE, UpdateHierarchy(index));
}

void Scene::RunObjectUpdateSystem(jobsystem::Context& p_context) {
    HBN_PROFILE_EVENT();
    unused(p_context);

    m_bound.MakeInvalid();

    for (auto [entity, obj] : m_ObjectComponents) {
        if (!Contains<TransformComponent>(entity)) {
            continue;
        }

        const TransformComponent& transform = *GetComponent<TransformComponent>(entity);
        DEV_ASSERT(Contains<MeshComponent>(obj.meshId));
        const MeshComponent& mesh = *GetComponent<MeshComponent>(obj.meshId);

        Matrix4x4f M = transform.GetWorldMatrix();
        AABB aabb = mesh.localBound;
        aabb.ApplyMatrix(M);
        m_bound.UnionBox(aabb);
    }
}

void Scene::RunParticleEmitterUpdateSystem(jobsystem::Context& p_context) {
    HBN_PROFILE_EVENT();
    unused(p_context);

    for (auto [entity, emitter] : m_ParticleEmitterComponents) {
        emitter.aliveBufferIndex = 1 - emitter.aliveBufferIndex;
    }
}

void Scene::RunMeshEmitterUpdateSystem(jobsystem::Context& p_context) {
    HBN_PROFILE_EVENT();

    unused(p_context);
    for (auto [id, emitter] : m_MeshEmitterComponents) {
        const TransformComponent* transform = GetComponent<TransformComponent>(id);
        if (DEV_VERIFY(transform)) {
            UpdateMeshEmitter(m_timestep, *transform, emitter);
        }
    }
}

}  // namespace my
