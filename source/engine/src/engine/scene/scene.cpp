#include "Scene.h"

#include "core/io/archive.h"
#include "core/math/geometry.h"
#include "core/systems/job_system.h"
#include "rendering/render_manager.h"

namespace my {

using ecs::Entity;
using jobsystem::Context;

static constexpr uint32_t SMALL_SUBTASK_GROUP_SIZE = 64;
// SCENE_VERSION history
// version 2: don't serialize scene.m_bound
// version 3: light component atten
// version 4: light component flags
// version 5: add validation
// version 6: add collider component
// version 7: add enabled to material
// version 8: add particle emitter
// version 9: add ParticleEmitterComponent.gravity
// version 10: add ForceFieldComponent
static constexpr uint32_t SCENE_VERSION = 10;
static constexpr uint32_t SCENE_MAGIC = 'xScn';

// @TODO: refactor
#if 1
#define JS_PARALLEL_FOR(CTX, INDEX, COUNT, SUBCOUNT, BODY) \
    CTX.Dispatch(                                          \
        static_cast<uint32_t>(COUNT),                      \
        SUBCOUNT,                                          \
        [&](jobsystem::JobArgs args) { const uint32_t INDEX = args.jobIndex; do { BODY; } while(0); })
#else
#define JS_PARALLEL_FOR(CTX, INDEX, COUNT, SUBCOUNT, BODY)                    \
    (void)(CTX);                                                              \
    for (uint32_t INDEX = 0; INDEX < static_cast<uint32_t>(COUNT); ++INDEX) { \
        BODY;                                                                 \
    }
#endif

void Scene::Update(float p_elapsedTime) {
    m_elapsedTime = p_elapsedTime;

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
    // particle
    RunParticleEmitterUpdateSystem(ctx);
    // armature
    RunArmatureUpdateSystem(ctx);
    ctx.Wait();

    // update bounding box
    RunObjectUpdateSystem(ctx);

    if (m_camera) {
        m_camera->Update();
    }
}

void Scene::Merge(Scene& p_other) {
    for (auto& entry : m_componentLib.m_entries) {
        entry.second.m_manager->Merge(*p_other.m_componentLib.m_entries[entry.first].m_manager);
    }
    if (p_other.m_root.IsValid()) {
        AttachComponent(p_other.m_root, m_root);
    }

    m_bound.UnionBox(p_other.m_bound);
}

void Scene::CreateCamera(int p_width,
                         int p_height,
                         float p_near_plane,
                         float p_far_plane,
                         Degree p_fovy) {
    m_camera = std::make_shared<Camera>();
    m_camera->m_width = p_width;
    m_camera->m_height = p_height;
    m_camera->m_near = p_near_plane;
    m_camera->m_far = p_far_plane;
    m_camera->m_fovy = p_fovy;
    m_camera->m_pitch = Degree{ -10.0f };
    m_camera->m_yaw = Degree{ -90.0f };
    m_camera->m_position = vec3{ 0, 4, 10 };
    m_camera->SetDirty();
}

Entity Scene::CreateNameEntity(const std::string& p_name) {
    Entity entity = Entity::Create();
    Create<NameComponent>(entity).SetName(p_name);
    return entity;
}

Entity Scene::CreateTransformEntity(const std::string& p_name) {
    Entity entity = CreateNameEntity(p_name);
    Create<TransformComponent>(entity);
    return entity;
}

Entity Scene::CreateObjectEntity(const std::string& p_name) {
    Entity entity = CreateNameEntity(p_name);
    Create<ObjectComponent>(entity);
    Create<TransformComponent>(entity);
    return entity;
}

Entity Scene::CreateMeshEntity(const std::string& p_name) {
    Entity entity = CreateNameEntity(p_name);
    Create<MeshComponent>(entity);
    return entity;
}

Entity Scene::CreateMaterialEntity(const std::string& p_name) {
    Entity entity = CreateNameEntity(p_name);
    Create<MaterialComponent>(entity);
    return entity;
}

Entity Scene::CreatePointLightEntity(const std::string& p_name,
                                     const vec3& p_position,
                                     const vec3& p_color,
                                     const float p_emissive) {
    Entity entity = CreateObjectEntity(p_name);

    LightComponent& light = Create<LightComponent>(entity);
    light.SetType(LIGHT_TYPE_POINT);
    light.m_atten.constant = 1.0f;
    light.m_atten.linear = 0.2f;
    light.m_atten.quadratic = 0.05f;

    MaterialComponent& material = Create<MaterialComponent>(entity);
    material.base_color = vec4(p_color, 1.0f);
    material.emissive = p_emissive;

    TransformComponent& transform = *GetComponent<TransformComponent>(entity);
    ObjectComponent& object = *GetComponent<ObjectComponent>(entity);
    transform.SetTranslation(p_position);
    transform.SetDirty();

    Entity mesh_id = CreateMeshEntity(p_name + ":mesh");
    object.meshId = mesh_id;
    object.flags = ObjectComponent::RENDERABLE;

    MeshComponent& mesh = *GetComponent<MeshComponent>(mesh_id);
    mesh = MakeSphereMesh(0.1f, 40, 40);
    mesh.subsets[0].material_id = entity;
    return entity;
}

Entity Scene::CreateAreaLightEntity(const std::string& p_name,
                                    const vec3& p_color,
                                    const float p_emissive) {
    Entity entity = CreateObjectEntity(p_name);

    // light
    LightComponent& light = Create<LightComponent>(entity);
    light.SetType(LIGHT_TYPE_AREA);
    // light.m_atten.constant = 1.0f;
    // light.m_atten.linear = 0.09f;
    // light.m_atten.quadratic = 0.032f;

    // material
    MaterialComponent& material = Create<MaterialComponent>(entity);
    material.base_color = vec4(p_color, 1.0f);
    material.emissive = p_emissive;

    ObjectComponent& object = *GetComponent<ObjectComponent>(entity);

    Entity mesh_id = CreateMeshEntity(p_name + ":mesh");
    object.meshId = mesh_id;
    object.flags = ObjectComponent::RENDERABLE;

    MeshComponent& mesh = *GetComponent<MeshComponent>(mesh_id);
    mesh = MakePlaneMesh();
    mesh.subsets[0].material_id = entity;
    return entity;
}

Entity Scene::CreateInfiniteLightEntity(const std::string& p_name,
                                        const vec3& p_color,
                                        const float p_emissive) {
    Entity entity = CreateNameEntity(p_name);

    Create<TransformComponent>(entity);

    LightComponent& light = Create<LightComponent>(entity);
    light.SetType(LIGHT_TYPE_INFINITE);
    light.m_atten.constant = 1.0f;
    light.m_atten.linear = 0.0f;
    light.m_atten.quadratic = 0.0f;

    MaterialComponent& material = Create<MaterialComponent>(entity);
    material.base_color = vec4(p_color, 1.0f);
    material.emissive = p_emissive;
    return entity;
}

Entity Scene::CreatePlaneEntity(const std::string& p_name,
                                const vec3& p_scale,
                                const mat4& p_transform) {
    Entity material_id = CreateMaterialEntity(p_name + ":mat");
    return CreatePlaneEntity(p_name, material_id, p_scale, p_transform);
}

Entity Scene::CreatePlaneEntity(const std::string& p_name,
                                Entity p_material_id,
                                const vec3& p_scale,
                                const mat4& p_transform) {
    ecs::Entity entity = CreateObjectEntity(p_name);
    TransformComponent& trans = *GetComponent<TransformComponent>(entity);
    ObjectComponent& object = *GetComponent<ObjectComponent>(entity);
    trans.MatrixTransform(p_transform);

    ecs::Entity mesh_id = CreateMeshEntity(p_name + ":mesh");
    object.meshId = mesh_id;

    MeshComponent& mesh = *GetComponent<MeshComponent>(mesh_id);
    mesh = MakePlaneMesh(p_scale);
    mesh.subsets[0].material_id = p_material_id;

    return entity;
}

Entity Scene::CreateCubeEntity(const std::string& p_name,
                               const vec3& p_scale,
                               const mat4& p_transform) {
    Entity material_id = CreateMaterialEntity(p_name + ":mat");
    return CreateCubeEntity(p_name, material_id, p_scale, p_transform);
}

Entity Scene::CreateCubeEntity(const std::string& p_name,
                               Entity p_material_id,
                               const vec3& p_scale,
                               const mat4& p_transform) {
    ecs::Entity entity = CreateObjectEntity(p_name);
    TransformComponent& trans = *GetComponent<TransformComponent>(entity);
    ObjectComponent& object = *GetComponent<ObjectComponent>(entity);
    trans.MatrixTransform(p_transform);

    ecs::Entity mesh_id = CreateMeshEntity(p_name + ":mesh");
    object.meshId = mesh_id;

    MeshComponent& mesh = *GetComponent<MeshComponent>(mesh_id);
    mesh = MakeCubeMesh(p_scale);
    mesh.subsets[0].material_id = p_material_id;

    return entity;
}

Entity Scene::CreateSphereEntity(const std::string& p_name,
                                 float p_radius,
                                 const mat4& p_transform) {
    Entity material_id = CreateMaterialEntity(p_name + ":mat");
    return CreateSphereEntity(p_name, material_id, p_radius, p_transform);
}

Entity Scene::CreateSphereEntity(const std::string& p_name,
                                 Entity p_material_id,
                                 float p_radius,
                                 const mat4& p_transform) {
    Entity entity = CreateObjectEntity(p_name);
    TransformComponent& transform = *GetComponent<TransformComponent>(entity);
    transform.MatrixTransform(p_transform);

    ObjectComponent& object = *GetComponent<ObjectComponent>(entity);
    Entity mesh_id = CreateMeshEntity(p_name + ":mesh");
    object.meshId = mesh_id;

    MeshComponent& mesh = *GetComponent<MeshComponent>(mesh_id);
    mesh = MakeSphereMesh(p_radius);
    mesh.subsets[0].material_id = p_material_id;

    return entity;
}

Entity Scene::CreateCylinderEntity(const std::string& p_name,
                                   float p_radius,
                                   float p_height,
                                   const mat4& p_transform) {
    Entity material_id = CreateMaterialEntity(p_name + ":mat");
    return CreateCylinderEntity(p_name, material_id, p_radius, p_height, p_transform);
}

Entity Scene::CreateCylinderEntity(const std::string& p_name,
                                   Entity p_material_id,
                                   float p_radius,
                                   float p_height,
                                   const mat4& p_transform) {
    Entity entity = CreateObjectEntity(p_name);
    TransformComponent& transform = *GetComponent<TransformComponent>(entity);
    transform.MatrixTransform(p_transform);

    ObjectComponent& object = *GetComponent<ObjectComponent>(entity);
    Entity mesh_id = CreateMeshEntity(p_name + ":mesh");
    object.meshId = mesh_id;

    MeshComponent& mesh = *GetComponent<MeshComponent>(mesh_id);
    mesh = MakeCylinder(p_radius, p_height);
    mesh.subsets[0].material_id = p_material_id;

    return entity;
}

Entity Scene::CreateTorusEntity(const std::string& p_name,
                                float p_radius,
                                float p_tube_radius,
                                const mat4& p_transform) {
    Entity material_id = CreateMaterialEntity(p_name + ":mat");
    return CreateTorusEntity(p_name, material_id, p_radius, p_tube_radius, p_transform);
}

Entity Scene::CreateTorusEntity(const std::string& p_name,
                                ecs::Entity p_material_id,
                                float p_radius,
                                float p_tube_radius,
                                const mat4& p_transform) {
    p_radius = 0.4f;
    p_tube_radius = 0.1f;

    Entity entity = CreateObjectEntity(p_name);
    TransformComponent& transform = *GetComponent<TransformComponent>(entity);
    transform.MatrixTransform(p_transform);

    ObjectComponent& object = *GetComponent<ObjectComponent>(entity);
    Entity mesh_id = CreateMeshEntity(p_name + ":mesh");
    object.meshId = mesh_id;

    MeshComponent& mesh = *GetComponent<MeshComponent>(mesh_id);
    mesh = MakeTorus(p_radius, p_tube_radius);
    mesh.subsets[0].material_id = p_material_id;

    return entity;
}

Entity Scene::CreateEmitterEntity(const std::string& p_name, const mat4& p_transform) {
    Entity entity = CreateTransformEntity(p_name);
    Create<ParticleEmitterComponent>(entity);

    TransformComponent& transform = *GetComponent<TransformComponent>(entity);
    transform.MatrixTransform(p_transform);

    return entity;
}

Entity Scene::CreateForceFieldEntity(const std::string& p_name, const mat4& p_transform) {
    Entity entity = CreateTransformEntity(p_name);
    Create<ForceFieldComponent>(entity);

    TransformComponent& transform = *GetComponent<TransformComponent>(entity);
    transform.MatrixTransform(p_transform);

    return entity;
}

void Scene::AttachComponent(Entity p_child, Entity p_parent) {
    DEV_ASSERT(p_child != p_parent);
    DEV_ASSERT(p_parent.IsValid());

    // if child already has a parent, detach it
    if (Contains<HierarchyComponent>(p_child)) {
        CRASH_NOW_MSG("Unlikely to happen at this point");
    }

    HierarchyComponent& hier = Create<HierarchyComponent>(p_child);
    hier.m_parentId = p_parent;
}

void Scene::RemoveEntity(Entity p_entity) {
    LightComponent* light = GetComponent<LightComponent>(p_entity);
    if (light) {
        auto shadow_handle = light->GetShadowMapIndex();
        if (shadow_handle != INVALID_POINT_SHADOW_HANDLE) {
            RenderManager::GetSingleton().free_point_light_shadowMap(shadow_handle);
        }
        m_LightComponents.Remove(p_entity);
    }
    m_HierarchyComponents.Remove(p_entity);
    m_TransformComponents.Remove(p_entity);
    m_ObjectComponents.Remove(p_entity);
}

void Scene::UpdateLight(uint32_t p_index) {
    Entity id = GetEntity<LightComponent>(p_index);
    const TransformComponent* transform = GetComponent<TransformComponent>(id);
    DEV_ASSERT(transform);
    m_LightComponents[p_index].Update(*transform);
}

void Scene::UpdateAnimation(uint32_t p_index) {
    AnimationComponent& animation = m_AnimationComponents[p_index];
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

        for (int k = 0; k < (int)sampler.keyframeTmes.size(); ++k) {
            const float time = sampler.keyframeTmes[k];
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

        const float left = sampler.keyframeTmes[key_left];
        const float right = sampler.keyframeTmes[key_right];

        float t = 0;
        if (key_left != key_right) {
            t = (animation.timer - left) / (right - left);
        }
        t = Saturate(t);

        TransformComponent* targetTransform = GetComponent<TransformComponent>(channel.targetId);
        DEV_ASSERT(targetTransform);
        switch (channel.path) {
            case AnimationComponent::Channel::PATH_SCALE: {
                DEV_ASSERT(sampler.keyframeData.size() == sampler.keyframeTmes.size() * 3);
                const vec3* data = (const vec3*)sampler.keyframeData.data();
                const vec3& vLeft = data[key_left];
                const vec3& vRight = data[key_right];
                targetTransform->SetScale(glm::mix(vLeft, vRight, t));
                break;
            }
            case AnimationComponent::Channel::PATH_TRANSLATION: {
                DEV_ASSERT(sampler.keyframeData.size() == sampler.keyframeTmes.size() * 3);
                const vec3* data = (const vec3*)sampler.keyframeData.data();
                const vec3& vLeft = data[key_left];
                const vec3& vRight = data[key_right];
                targetTransform->SetTranslation(glm::mix(vLeft, vRight, t));
                break;
            }
            case AnimationComponent::Channel::PATH_ROTATION: {
                DEV_ASSERT(sampler.keyframeData.size() == sampler.keyframeTmes.size() * 4);
                const vec4* data = (const vec4*)sampler.keyframeData.data();
                const vec4& vLeft = data[key_left];
                const vec4& vRight = data[key_right];
                targetTransform->SetRotation(glm::mix(vLeft, vRight, t));
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
        // @TODO: set elapsed time
        animation.timer += m_elapsedTime * animation.speed;
    }
}

void Scene::UpdateHierarchy(uint32_t p_index) {
    Entity self_id = GetEntity<HierarchyComponent>(p_index);
    TransformComponent* self_transform = GetComponent<TransformComponent>(self_id);

    if (!self_transform) {
        return;
    }

    mat4 world_matrix = self_transform->GetLocalMatrix();
    const HierarchyComponent* hierarchy = &m_HierarchyComponents[p_index];
    Entity parent = hierarchy->m_parentId;

    while (parent.IsValid()) {
        TransformComponent* parent_transform = GetComponent<TransformComponent>(parent);
        DEV_ASSERT(parent_transform);
        world_matrix = parent_transform->GetLocalMatrix() * world_matrix;

        if ((hierarchy = GetComponent<HierarchyComponent>(parent)) != nullptr) {
            parent = hierarchy->m_parentId;
            DEV_ASSERT(parent.IsValid());
        } else {
            parent.MakeInvalid();
        }
    }

    self_transform->SetWorldMatrix(world_matrix);
    self_transform->SetDirty(false);
}

void Scene::UpdateArmature(uint32_t p_index) {
    Entity id = m_ArmatureComponents.GetEntity(p_index);
    ArmatureComponent& armature = m_ArmatureComponents[p_index];
    TransformComponent* transform = GetComponent<TransformComponent>(id);
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

    const mat4 R = glm::inverse(transform->GetWorldMatrix());
    const size_t numBones = armature.boneCollection.size();
    if (armature.boneTransforms.size() != numBones) {
        armature.boneTransforms.resize(numBones);
    }

    int idx = 0;
    for (Entity boneID : armature.boneCollection) {
        const TransformComponent* boneTransform = GetComponent<TransformComponent>(boneID);
        DEV_ASSERT(boneTransform);

        const mat4& B = armature.inverseBindMatrices[idx];
        const mat4& W = boneTransform->GetWorldMatrix();
        const mat4 M = R * W * B;
        armature.boneTransforms[idx] = M;
        ++idx;

        // @TODO: armature animation
    }
};

bool Scene::Serialize(Archive& p_archive) {
    uint32_t version = UINT_MAX;
    bool is_read_mode = !p_archive.IsWriteMode();
    if (is_read_mode) {
        uint32_t magic;
        uint32_t seed = Entity::MAX_ID;

        p_archive >> magic;
        ERR_FAIL_COND_V_MSG(magic != SCENE_MAGIC, false, "file corrupted");
        p_archive >> version;
        ERR_FAIL_COND_V_MSG(version > SCENE_MAGIC, false, std::format("file version {} is greater than max version {}", version, SCENE_VERSION));
        p_archive >> seed;
        Entity::SetSeed(seed);

    } else {
        p_archive << SCENE_MAGIC;
        p_archive << SCENE_VERSION;
        p_archive << Entity::GetSeed();
    }

    m_root.Serialize(p_archive);
    if (is_read_mode) {
        m_camera = std::make_shared<Camera>();
    }
    m_camera->Serialize(p_archive, version);

    constexpr uint64_t has_next_flag = 6368519827137030510;
    if (p_archive.IsWriteMode()) {
        for (const auto& it : m_componentLib.m_entries) {
            p_archive << has_next_flag;
            p_archive << it.first;  // write name
            it.second.m_manager->Serialize(p_archive, version);
        }
        p_archive << uint64_t(0);
        return true;
    } else {
        for (;;) {
            uint64_t has_next = 0;
            p_archive >> has_next;
            if (has_next != has_next_flag) {
                return true;
            }

            std::string key;
            p_archive >> key;
            auto it = m_componentLib.m_entries.find(key);
            if (it == m_componentLib.m_entries.end()) {
                LOG_ERROR("scene corrupted");
                return false;
            }
            if (!it->second.m_manager->Serialize(p_archive, version)) {
                return false;
            }
        }
    }
}

bool Scene::RayObjectIntersect(Entity p_object_id, Ray& p_ray) {
    ObjectComponent* object = GetComponent<ObjectComponent>(p_object_id);
    MeshComponent* mesh = GetComponent<MeshComponent>(object->meshId);
    TransformComponent* transform = GetComponent<TransformComponent>(p_object_id);
    DEV_ASSERT(mesh && transform);

    if (!transform || !mesh) {
        return false;
    }

    mat4 inversedModel = glm::inverse(transform->GetWorldMatrix());
    Ray inversedRay = p_ray.Inverse(inversedModel);
    Ray inversedRayAABB = inversedRay;  // make a copy, we don't want dist to be modified by AABB
    // Perform aabb test
    if (!inversedRayAABB.Intersects(mesh->localBound)) {
        return false;
    }

    // @TODO: test submesh intersection

    // Test every single triange
    for (size_t i = 0; i < mesh->indices.size(); i += 3) {
        const vec3& A = mesh->positions[mesh->indices[i]];
        const vec3& B = mesh->positions[mesh->indices[i + 1]];
        const vec3& C = mesh->positions[mesh->indices[i + 2]];
        if (inversedRay.Intersects(A, B, C)) {
            p_ray.CopyDist(inversedRay);
            return true;
        }
    }
    return false;
}

Scene::RayIntersectionResult Scene::Intersects(Ray& p_ray) {
    RayIntersectionResult result;

    // @TODO: box collider
    for (int object_idx = 0; object_idx < GetCount<ObjectComponent>(); ++object_idx) {
        Entity entity = GetEntity<ObjectComponent>(object_idx);
        if (RayObjectIntersect(entity, p_ray)) {
            result.entity = entity;
        }
    }

    return result;
}

void Scene::RunLightUpdateSystem(Context& p_context) {
    JS_PARALLEL_FOR(p_context, index, GetCount<LightComponent>(), SMALL_SUBTASK_GROUP_SIZE, UpdateLight(index));
}

void Scene::RunTransformationUpdateSystem(Context& p_context) {
    JS_PARALLEL_FOR(p_context, index, GetCount<TransformComponent>(), SMALL_SUBTASK_GROUP_SIZE, m_TransformComponents[index].UpdateTransform());
}

void Scene::RunAnimationUpdateSystem(Context& p_context) {
    JS_PARALLEL_FOR(p_context, index, GetCount<AnimationComponent>(), 1, UpdateAnimation(index));
}

void Scene::RunArmatureUpdateSystem(Context& p_context) {
    JS_PARALLEL_FOR(p_context, index, GetCount<ArmatureComponent>(), 1, UpdateArmature(index));
}

void Scene::RunHierarchyUpdateSystem(Context& p_context) {
    JS_PARALLEL_FOR(p_context, index, GetCount<HierarchyComponent>(), SMALL_SUBTASK_GROUP_SIZE, UpdateHierarchy(index));
}

void Scene::RunObjectUpdateSystem(jobsystem::Context& p_context) {
    unused(p_context);

    m_bound.MakeInvalid();

    for (auto [entity, obj] : m_ObjectComponents) {
        if (!Contains<TransformComponent>(entity)) {
            continue;
        }

        const TransformComponent& transform = *GetComponent<TransformComponent>(entity);
        DEV_ASSERT(Contains<MeshComponent>(obj.meshId));
        const MeshComponent& mesh = *GetComponent<MeshComponent>(obj.meshId);

        mat4 M = transform.GetWorldMatrix();
        AABB aabb = mesh.localBound;
        aabb.ApplyMatrix(M);
        m_bound.UnionBox(aabb);
    }
}

void Scene::RunParticleEmitterUpdateSystem(jobsystem::Context& p_context) {
    unused(p_context);

    for (auto [entity, emitter] : m_ParticleEmitterComponents) {
        emitter.Update(m_elapsedTime);
    }
}

}  // namespace my