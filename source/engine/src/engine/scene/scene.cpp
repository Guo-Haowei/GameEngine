#include "Scene.h"

#include "core/io/archive.h"
#include "core/math/geometry.h"
#include "core/systems/job_system.h"
#include "rendering/render_manager.h"

namespace my {

using ecs::Entity;
using jobsystem::Context;

static constexpr uint32_t kSmallSubtaskGroupSize = 64;
// kSceneVersion history
// version 2: don't serialize scene.m_bound
// version 3: light component atten
// version 4: light component flags
// version 5: add validation
// version 6: add collider component
// version 7: add enabled to material
static constexpr uint32_t kSceneVersion = 7;
static constexpr uint32_t kSceneMagicNumber = 'xScn';

// @TODO: refactor
#if 1
#define JS_PARALLEL_FOR(CTX, INDEX, COUNT, SUBCOUNT, BODY) \
    CTX.dispatch(                                          \
        static_cast<uint32_t>(COUNT),                      \
        SUBCOUNT,                                          \
        [&](jobsystem::JobArgs args) { const uint32_t INDEX = args.job_index; do { BODY; } while(0); })
#else
#define JS_PARALLEL_FOR(CTX, INDEX, COUNT, SUBCOUNT, BODY)                    \
    (void)(CTX);                                                              \
    for (uint32_t INDEX = 0; INDEX < static_cast<uint32_t>(COUNT); ++INDEX) { \
        BODY;                                                                 \
    }
#endif

void Scene::update(float p_delta_time) {
    m_delta_time = p_delta_time;

    Context ctx;

    // animation
    runLightUpdateSystem(ctx);
    runAnimationUpdateSystem(ctx);
    ctx.wait();
    // transform, update local matrix from position, rotation and scale
    runTransformationUpdateSystem(ctx);
    ctx.wait();
    // hierarchy, update world matrix based on hierarchy
    runHierarchyUpdateSystem(ctx);
    ctx.wait();
    // armature
    runArmatureUpdateSystem(ctx);
    ctx.wait();

    // update bounding box
    runObjectUpdateSystem(ctx);

    if (m_camera) {
        m_camera->update();
    }
}

void Scene::merge(Scene& p_other) {
    for (auto& entry : m_component_lib.m_entries) {
        entry.second.m_manager->merge(*p_other.m_component_lib.m_entries[entry.first].m_manager);
    }
    if (p_other.m_root.isValid()) {
        attachComponent(p_other.m_root, m_root);
    }

    m_bound.unionBox(p_other.m_bound);
}

void Scene::createCamera(int p_width,
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
    m_camera->setDirty();
}

Entity Scene::createNameEntity(const std::string& p_name) {
    Entity entity = Entity::create();
    create<NameComponent>(entity).setName(p_name);
    return entity;
}

Entity Scene::createTransformEntity(const std::string& p_name) {
    Entity entity = createNameEntity(p_name);
    create<TransformComponent>(entity);
    return entity;
}

Entity Scene::createObjectEntity(const std::string& p_name) {
    Entity entity = createNameEntity(p_name);
    create<ObjectComponent>(entity);
    create<TransformComponent>(entity);
    return entity;
}

Entity Scene::createMeshEntity(const std::string& p_name) {
    Entity entity = createNameEntity(p_name);
    create<MeshComponent>(entity);
    return entity;
}

Entity Scene::createMaterialEntity(const std::string& p_name) {
    Entity entity = createNameEntity(p_name);
    create<MaterialComponent>(entity);
    return entity;
}

Entity Scene::createPointLightEntity(const std::string& p_name,
                                     const vec3& p_position,
                                     const vec3& p_color,
                                     const float p_emissive) {
    Entity entity = createObjectEntity(p_name);

    LightComponent& light = create<LightComponent>(entity);
    light.setType(LIGHT_TYPE_POINT);
    light.m_atten.constant = 1.0f;
    light.m_atten.linear = 0.2f;
    light.m_atten.quadratic = 0.05f;

    MaterialComponent& material = create<MaterialComponent>(entity);
    material.base_color = vec4(p_color, 1.0f);
    material.emissive = p_emissive;

    TransformComponent& transform = *getComponent<TransformComponent>(entity);
    ObjectComponent& object = *getComponent<ObjectComponent>(entity);
    transform.setTranslation(p_position);
    transform.setDirty();

    Entity mesh_id = createMeshEntity(p_name + ":mesh");
    object.mesh_id = mesh_id;
    object.flags = ObjectComponent::RENDERABLE;

    MeshComponent& mesh = *getComponent<MeshComponent>(mesh_id);
    mesh = makeSphereMesh(0.1f, 40, 40);
    mesh.subsets[0].material_id = entity;
    return entity;
}

Entity Scene::createAreaLightEntity(const std::string& p_name,
                                    const vec3& p_color,
                                    const float p_emissive) {
    Entity entity = createObjectEntity(p_name);

    // light
    LightComponent& light = create<LightComponent>(entity);
    light.setType(LIGHT_TYPE_AREA);
    // light.m_atten.constant = 1.0f;
    // light.m_atten.linear = 0.09f;
    // light.m_atten.quadratic = 0.032f;

    // material
    MaterialComponent& material = create<MaterialComponent>(entity);
    material.base_color = vec4(p_color, 1.0f);
    material.emissive = p_emissive;

    ObjectComponent& object = *getComponent<ObjectComponent>(entity);

    Entity mesh_id = createMeshEntity(p_name + ":mesh");
    object.mesh_id = mesh_id;
    object.flags = ObjectComponent::RENDERABLE;

    MeshComponent& mesh = *getComponent<MeshComponent>(mesh_id);
    mesh = makePlaneMesh();
    mesh.subsets[0].material_id = entity;
    return entity;
}

Entity Scene::createOmniLightEntity(const std::string& p_name,
                                    const vec3& p_color,
                                    const float p_emissive) {
    Entity entity = createNameEntity(p_name);

    create<TransformComponent>(entity);

    LightComponent& light = create<LightComponent>(entity);
    light.setType(LIGHT_TYPE_OMNI);
    light.m_atten.constant = 1.0f;
    light.m_atten.linear = 0.0f;
    light.m_atten.quadratic = 0.0f;

    MaterialComponent& material = create<MaterialComponent>(entity);
    material.base_color = vec4(p_color, 1.0f);
    material.emissive = p_emissive;
    return entity;
}

Entity Scene::createPlaneEntity(const std::string& p_name,
                                const vec3& p_scale,
                                const mat4& p_transform) {
    Entity material_id = createMaterialEntity(p_name + ":mat");
    return createPlaneEntity(p_name, material_id, p_scale, p_transform);
}

Entity Scene::createPlaneEntity(const std::string& p_name,
                                Entity p_material_id,
                                const vec3& p_scale,
                                const mat4& p_transform) {
    ecs::Entity entity = createObjectEntity(p_name);
    TransformComponent& trans = *getComponent<TransformComponent>(entity);
    ObjectComponent& object = *getComponent<ObjectComponent>(entity);
    trans.matrixTransform(p_transform);

    ecs::Entity mesh_id = createMeshEntity(p_name + ":mesh");
    object.mesh_id = mesh_id;

    MeshComponent& mesh = *getComponent<MeshComponent>(mesh_id);
    mesh = makePlaneMesh(p_scale);
    mesh.subsets[0].material_id = p_material_id;

    return entity;
}

Entity Scene::createCubeEntity(const std::string& p_name,
                               const vec3& p_scale,
                               const mat4& p_transform) {
    Entity material_id = createMaterialEntity(p_name + ":mat");
    return createCubeEntity(p_name, material_id, p_scale, p_transform);
}

Entity Scene::createCubeEntity(const std::string& p_name,
                               Entity p_material_id,
                               const vec3& p_scale,
                               const mat4& p_transform) {
    ecs::Entity entity = createObjectEntity(p_name);
    TransformComponent& trans = *getComponent<TransformComponent>(entity);
    ObjectComponent& object = *getComponent<ObjectComponent>(entity);
    trans.matrixTransform(p_transform);

    ecs::Entity mesh_id = createMeshEntity(p_name + ":mesh");
    object.mesh_id = mesh_id;

    MeshComponent& mesh = *getComponent<MeshComponent>(mesh_id);
    mesh = makeCubeMesh(p_scale);
    mesh.subsets[0].material_id = p_material_id;

    return entity;
}

Entity Scene::createSphereEntity(const std::string& p_name,
                                 float p_radius,
                                 const mat4& p_transform) {
    Entity material_id = createMaterialEntity(p_name + ":mat");
    return createSphereEntity(p_name, material_id, p_radius, p_transform);
}

Entity Scene::createSphereEntity(const std::string& p_name,
                                 Entity p_material_id,
                                 float p_radius,
                                 const mat4& p_transform) {
    ecs::Entity entity = createObjectEntity(p_name);
    TransformComponent& trans = *getComponent<TransformComponent>(entity);
    ObjectComponent& object = *getComponent<ObjectComponent>(entity);
    trans.matrixTransform(p_transform);

    ecs::Entity mesh_id = createMeshEntity(p_name + ":mesh");
    object.mesh_id = mesh_id;

    MeshComponent& mesh = *getComponent<MeshComponent>(mesh_id);
    mesh = makeSphereMesh(p_radius);
    mesh.subsets[0].material_id = p_material_id;

    return entity;
}

void Scene::attachComponent(Entity p_child, Entity p_parent) {
    DEV_ASSERT(p_child != p_parent);
    DEV_ASSERT(p_parent.isValid());

    // if child already has a parent, detach it
    if (contains<HierarchyComponent>(p_child)) {
        CRASH_NOW_MSG("Unlikely to happen at this point");
    }

    HierarchyComponent& hier = create<HierarchyComponent>(p_child);
    hier.m_parent_id = p_parent;
}

void Scene::removeEntity(Entity p_entity) {
    LightComponent* light = getComponent<LightComponent>(p_entity);
    if (light) {
        auto shadow_handle = light->getShadowMapIndex();
        if (shadow_handle != INVALID_POINT_SHADOW_HANDLE) {
            RenderManager::singleton().free_point_light_shadow_map(shadow_handle);
        }
        m_LightComponents.remove(p_entity);
    }
    m_HierarchyComponents.remove(p_entity);
    m_TransformComponents.remove(p_entity);
    m_ObjectComponents.remove(p_entity);
}

void Scene::updateLight(uint32_t p_index) {
    Entity id = getEntity<LightComponent>(p_index);
    const TransformComponent* transform = getComponent<TransformComponent>(id);
    DEV_ASSERT(transform);
    m_LightComponents[p_index].update(*transform);
}

void Scene::updateAnimation(uint32_t p_index) {
    AnimationComponent& animation = m_AnimationComponents[p_index];
    if (!animation.isPlaying()) {
        return;
    }

    for (const AnimationComponent::Channel& channel : animation.channels) {
        if (channel.path == AnimationComponent::Channel::PATH_UNKNOWN) {
            continue;
        }
        DEV_ASSERT(channel.sampler_index < (int)animation.samplers.size());
        const AnimationComponent::Sampler& sampler = animation.samplers[channel.sampler_index];

        int key_left = 0;
        int key_right = 0;
        float time_first = std::numeric_limits<float>::min();
        float time_last = std::numeric_limits<float>::min();
        float time_left = std::numeric_limits<float>::min();
        float time_right = std::numeric_limits<float>::max();

        for (int k = 0; k < (int)sampler.keyframe_times.size(); ++k) {
            const float time = sampler.keyframe_times[k];
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

        const float left = sampler.keyframe_times[key_left];
        const float right = sampler.keyframe_times[key_right];

        float t = 0;
        if (key_left != key_right) {
            t = (animation.timer - left) / (right - left);
        }
        t = saturate(t);

        TransformComponent* targetTransform = getComponent<TransformComponent>(channel.target_id);
        DEV_ASSERT(targetTransform);
        switch (channel.path) {
            case AnimationComponent::Channel::PATH_SCALE: {
                DEV_ASSERT(sampler.keyframe_data.size() == sampler.keyframe_times.size() * 3);
                const vec3* data = (const vec3*)sampler.keyframe_data.data();
                const vec3& vLeft = data[key_left];
                const vec3& vRight = data[key_right];
                targetTransform->setScale(glm::mix(vLeft, vRight, t));
                break;
            }
            case AnimationComponent::Channel::PATH_TRANSLATION: {
                DEV_ASSERT(sampler.keyframe_data.size() == sampler.keyframe_times.size() * 3);
                const vec3* data = (const vec3*)sampler.keyframe_data.data();
                const vec3& vLeft = data[key_left];
                const vec3& vRight = data[key_right];
                targetTransform->setTranslation(glm::mix(vLeft, vRight, t));
                break;
            }
            case AnimationComponent::Channel::PATH_ROTATION: {
                DEV_ASSERT(sampler.keyframe_data.size() == sampler.keyframe_times.size() * 4);
                const vec4* data = (const vec4*)sampler.keyframe_data.data();
                const vec4& vLeft = data[key_left];
                const vec4& vRight = data[key_right];
                targetTransform->setRotation(glm::mix(vLeft, vRight, t));
                break;
            }
            default:
                CRASH_NOW();
                break;
        }
        targetTransform->setDirty();
    }

    if (animation.isLooped() && animation.timer > animation.end) {
        animation.timer = animation.start;
    }

    if (animation.isPlaying()) {
        // @TODO: set delta time
        animation.timer += m_delta_time * animation.speed;
    }
}

void Scene::updateHierarchy(uint32_t p_index) {
    Entity self_id = getEntity<HierarchyComponent>(p_index);
    TransformComponent* self_transform = getComponent<TransformComponent>(self_id);

    if (!self_transform) {
        return;
    }

    mat4 world_matrix = self_transform->getLocalMatrix();
    const HierarchyComponent* hierarchy = &m_HierarchyComponents[p_index];
    Entity parent = hierarchy->m_parent_id;

    while (parent.isValid()) {
        TransformComponent* parent_transform = getComponent<TransformComponent>(parent);
        DEV_ASSERT(parent_transform);
        world_matrix = parent_transform->getLocalMatrix() * world_matrix;

        if ((hierarchy = getComponent<HierarchyComponent>(parent)) != nullptr) {
            parent = hierarchy->m_parent_id;
            DEV_ASSERT(parent.isValid());
        } else {
            parent.makeInvalid();
        }
    }

    self_transform->setWorldMatrix(world_matrix);
    self_transform->setDirty(false);
}

void Scene::updateArmature(uint32_t p_index) {
    Entity id = m_ArmatureComponents.getEntity(p_index);
    ArmatureComponent& armature = m_ArmatureComponents[p_index];
    TransformComponent* transform = getComponent<TransformComponent>(id);
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

    const mat4 R = glm::inverse(transform->getWorldMatrix());
    const size_t numBones = armature.bone_collection.size();
    if (armature.bone_transforms.size() != numBones) {
        armature.bone_transforms.resize(numBones);
    }

    int idx = 0;
    for (Entity boneID : armature.bone_collection) {
        const TransformComponent* boneTransform = getComponent<TransformComponent>(boneID);
        DEV_ASSERT(boneTransform);

        const mat4& B = armature.inverse_bind_matrices[idx];
        const mat4& W = boneTransform->getWorldMatrix();
        const mat4 M = R * W * B;
        armature.bone_transforms[idx] = M;
        ++idx;

        // @TODO: armature animation
    }
};

bool Scene::serialize(Archive& p_archive) {
    uint32_t version = UINT_MAX;
    bool is_read_mode = !p_archive.isWriteMode();
    if (is_read_mode) {
        uint32_t magic;
        uint32_t seed = Entity::MAX_ID;

        p_archive >> magic;
        ERR_FAIL_COND_V_MSG(magic != kSceneMagicNumber, false, "file corrupted");
        p_archive >> version;
        ERR_FAIL_COND_V_MSG(version > kSceneMagicNumber, false, std::format("file version {} is greater than max version {}", version, kSceneVersion));
        p_archive >> seed;
        Entity::setSeed(seed);

    } else {
        p_archive << kSceneMagicNumber;
        p_archive << kSceneVersion;
        p_archive << Entity::getSeed();
    }

    m_root.serialize(p_archive);
    if (is_read_mode) {
        m_camera = std::make_shared<Camera>();
    }
    m_camera->serialize(p_archive, version);

    constexpr uint64_t has_next_flag = 6368519827137030510;
    if (p_archive.isWriteMode()) {
        for (const auto& it : m_component_lib.m_entries) {
            p_archive << has_next_flag;
            p_archive << it.first;  // write name
            it.second.m_manager->serialize(p_archive, version);
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
            auto it = m_component_lib.m_entries.find(key);
            if (it == m_component_lib.m_entries.end()) {
                LOG_ERROR("scene corrupted");
                return false;
            }
            if (!it->second.m_manager->serialize(p_archive, version)) {
                return false;
            }
        }
    }
}

bool Scene::rayObjectIntersect(Entity p_object_id, Ray& p_ray) {
    ObjectComponent* object = getComponent<ObjectComponent>(p_object_id);
    MeshComponent* mesh = getComponent<MeshComponent>(object->mesh_id);
    TransformComponent* transform = getComponent<TransformComponent>(p_object_id);
    DEV_ASSERT(mesh && transform);

    if (!transform || !mesh) {
        return false;
    }

    mat4 inversedModel = glm::inverse(transform->getWorldMatrix());
    Ray inversedRay = p_ray.inverse(inversedModel);
    Ray inversedRayAABB = inversedRay;  // make a copy, we don't want dist to be modified by AABB
    // Perform aabb test
    if (!inversedRayAABB.intersects(mesh->local_bound)) {
        return false;
    }

    // @TODO: test submesh intersection

    // Test every single triange
    for (size_t i = 0; i < mesh->indices.size(); i += 3) {
        const vec3& A = mesh->positions[mesh->indices[i]];
        const vec3& B = mesh->positions[mesh->indices[i + 1]];
        const vec3& C = mesh->positions[mesh->indices[i + 2]];
        if (inversedRay.intersects(A, B, C)) {
            p_ray.copyDist(inversedRay);
            return true;
        }
    }
    return false;
}

Scene::RayIntersectionResult Scene::intersects(Ray& p_ray) {
    RayIntersectionResult result;

    // @TODO: box collider
    for (int object_idx = 0; object_idx < getCount<ObjectComponent>(); ++object_idx) {
        Entity entity = getEntity<ObjectComponent>(object_idx);
        if (rayObjectIntersect(entity, p_ray)) {
            result.entity = entity;
        }
    }

    return result;
}

void Scene::runLightUpdateSystem(Context& p_ctx) {
    JS_PARALLEL_FOR(p_ctx, index, getCount<LightComponent>(), kSmallSubtaskGroupSize, updateLight(index));
}

void Scene::runTransformationUpdateSystem(Context& p_ctx) {
    JS_PARALLEL_FOR(p_ctx, index, getCount<TransformComponent>(), kSmallSubtaskGroupSize, m_TransformComponents[index].updateTransform());
}

void Scene::runAnimationUpdateSystem(Context& p_ctx) {
    JS_PARALLEL_FOR(p_ctx, index, getCount<AnimationComponent>(), 1, updateAnimation(index));
}

void Scene::runArmatureUpdateSystem(Context& p_ctx) {
    JS_PARALLEL_FOR(p_ctx, index, getCount<ArmatureComponent>(), 1, updateArmature(index));
}

void Scene::runHierarchyUpdateSystem(Context& p_ctx) {
    JS_PARALLEL_FOR(p_ctx, index, getCount<HierarchyComponent>(), kSmallSubtaskGroupSize, updateHierarchy(index));
}

void Scene::runObjectUpdateSystem(jobsystem::Context&) {
    m_bound.makeInvalid();

    for (auto [entity, obj] : m_ObjectComponents) {
        if (!contains<TransformComponent>(entity)) {
            continue;
        }

        const TransformComponent& transform = *getComponent<TransformComponent>(entity);
        DEV_ASSERT(contains<MeshComponent>(obj.mesh_id));
        const MeshComponent& mesh = *getComponent<MeshComponent>(obj.mesh_id);

        mat4 M = transform.getWorldMatrix();
        AABB aabb = mesh.local_bound;
        aabb.applyMatrix(M);
        m_bound.unionBox(aabb);
    }
}

}  // namespace my