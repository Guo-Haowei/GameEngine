#include "ecs_systems.h"

#include "engine/core/base/random.h"
#include "engine/core/debugger/profiler.h"
#include "engine/scene/scene.h"
#include "engine/systems/job_system/job_system.h"

namespace my {

static constexpr uint32_t SMALL_SUBTASK_GROUP_SIZE = 64;

#define JS_FORCE_PARALLEL_FOR(TYPE, CTX, INDEX, SUBCOUNT, BODY) \
    CTX.Dispatch(                                               \
        static_cast<uint32_t>(p_scene.GetCount<TYPE>()),        \
        SUBCOUNT,                                               \
        [&](jobsystem::JobArgs args) { const uint32_t INDEX = args.jobIndex; do { BODY; } while(0); })

#define JS_NO_PARALLEL_FOR(TYPE, CTX, INDEX, SUBCOUNT, BODY)            \
    (void)(CTX);                                                        \
    for (size_t INDEX = 0; INDEX < p_scene.GetCount<TYPE>(); ++INDEX) { \
        BODY;                                                           \
    }

#if USING(ENABLE_JOB_SYSTEM)
#define JS_PARALLEL_FOR JS_FORCE_PARALLEL_FOR
#else
#define JS_PARALLEL_FOR JS_NO_PARALLEL_FOR
#endif

static void UpdateAnimation(Scene& p_scene, size_t p_index, float p_timestep) {
    AnimationComponent& animation = p_scene.GetComponentByIndex<AnimationComponent>(p_index);

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

        TransformComponent* targetTransform = p_scene.GetComponent<TransformComponent>(channel.targetId);
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
        animation.timer += p_timestep * animation.speed;
    }
}

static void UpdateHierarchy(Scene& p_scene, size_t p_index, float p_timestep) {
    unused(p_timestep);

    ecs::Entity self_id = p_scene.GetEntityByIndex<HierarchyComponent>(p_index);
    TransformComponent* self_transform = p_scene.GetComponent<TransformComponent>(self_id);

    if (!self_transform) {
        return;
    }

    Matrix4x4f world_matrix = self_transform->GetLocalMatrix();
    const HierarchyComponent* hierarchy = &p_scene.GetComponentByIndex<HierarchyComponent>(p_index);
    ecs::Entity parent = hierarchy->GetParent();

    while (parent.IsValid()) {
        TransformComponent* parent_transform = p_scene.GetComponent<TransformComponent>(parent);
        if (DEV_VERIFY(parent_transform)) {
            world_matrix = parent_transform->GetLocalMatrix() * world_matrix;

            if ((hierarchy = p_scene.GetComponent<HierarchyComponent>(parent)) != nullptr) {
                parent = hierarchy->GetParent();
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

static void UpdateArmature(Scene& p_scene, size_t p_index, float) {
    TransformComponent* transform = p_scene.GetComponent<TransformComponent>(p_scene.GetEntityByIndex<ArmatureComponent>(p_index));
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

    ArmatureComponent& armature = p_scene.GetComponentByIndex<ArmatureComponent>(p_index);
    const Matrix4x4f R = glm::inverse(transform->GetWorldMatrix());
    const size_t numBones = armature.boneCollection.size();
    if (armature.boneTransforms.size() != numBones) {
        armature.boneTransforms.resize(numBones);
    }

    int idx = 0;
    for (ecs::Entity boneID : armature.boneCollection) {
        const TransformComponent* boneTransform = p_scene.GetComponent<TransformComponent>(boneID);
        DEV_ASSERT(boneTransform);

        const Matrix4x4f& B = armature.inverseBindMatrices[idx];
        const Matrix4x4f& W = boneTransform->GetWorldMatrix();
        const Matrix4x4f M = R * W * B;
        armature.boneTransforms[idx] = M;
        ++idx;

        // @TODO: armature animation
    }
};

static void UpdateLight(float p_timestep,
                        const TransformComponent& p_transform,
                        LightComponent& p_light) {
    unused(p_timestep);

    p_light.m_position = p_transform.GetTranslation();

    if (p_light.IsDirty() || p_transform.IsDirty()) {
        // update max distance
        constexpr float atten_factor_inv = 1.0f / 0.03f;
        if (p_light.m_atten.linear == 0.0f && p_light.m_atten.quadratic == 0.0f) {
            p_light.m_maxDistance = 1000.0f;
        } else {
            // (constant + linear * x + quad * x^2) * atten_factor = 1
            // quad * x^2 + linear * x + constant - 1.0 / atten_factor = 0
            const float a = p_light.m_atten.quadratic;
            const float b = p_light.m_atten.linear;
            const float c = p_light.m_atten.constant - atten_factor_inv;

            float discriminant = b * b - 4 * a * c;
            if (discriminant < 0.0f) {
                CRASH_NOW_MSG("TODO: fix");
            }

            float sqrt_d = glm::sqrt(discriminant);
            float root1 = (-b + sqrt_d) / (2 * a);
            float root2 = (-b - sqrt_d) / (2 * a);
            p_light.m_maxDistance = root1 > 0.0f ? root1 : root2;
            p_light.m_maxDistance = glm::max(LIGHT_SHADOW_MIN_DISTANCE + 1.0f, p_light.m_maxDistance);
        }

        // update shadow map
        if (p_light.CastShadow()) {
            // @TODO: [SCRUM-178] shadow atlas
        }

        // update light space matrices
        if (p_light.CastShadow()) {
            switch (p_light.m_type) {
                case LIGHT_TYPE_POINT: {
                    CRASH_NOW();
#if 0
                    constexpr float near_plane = LIGHT_SHADOW_MIN_DISTANCE;
                    const float far_plane = p_light.m_maxDistance;
                    const bool is_opengl = IGraphicsManager::GetSingleton().GetBackend() == Backend::OPENGL;
                    auto matrices = is_opengl ? BuildOpenGlPointLightCubeMapViewProjectionMatrix(p_light.m_position, near_plane, far_plane)
                                              : BuildPointLightCubeMapViewProjectionMatrix(p_light.m_position, near_plane, far_plane);

                    for (size_t i = 0; i < matrices.size(); ++i) {
                        p_light.m_lightSpaceMatrices[i] = matrices[i];
                    }
#endif
                } break;
                default:
                    break;
            }
        }

        // @TODO: query if transformation is dirty, so don't update shadow map unless necessary
        p_light.SetDirty(false);
    }
}

void RunLightUpdateSystem(Scene& p_scene, jobsystem::Context& p_context, float p_timestep) {
    HBN_PROFILE_EVENT();
    unused(p_context);

    for (auto [id, light] : p_scene.m_LightComponents) {
        const TransformComponent* transform = p_scene.GetComponent<TransformComponent>(id);
        if (DEV_VERIFY(transform)) {
            UpdateLight(p_timestep, *transform, light);
        }
    }
}

void RunTransformationUpdateSystem(Scene& p_scene, jobsystem::Context& p_context, float) {
    HBN_PROFILE_EVENT();

    JS_PARALLEL_FOR(TransformComponent, p_context, index, SMALL_SUBTASK_GROUP_SIZE, {
        if (p_scene.GetComponentByIndex<TransformComponent>(index).UpdateTransform()) {
            p_scene.m_dirtyFlags.fetch_or(SCENE_DIRTY_WORLD);
        }
    });
}

void RunAnimationUpdateSystem(Scene& p_scene, jobsystem::Context& p_context, float p_timestep) {
    HBN_PROFILE_EVENT();
    JS_PARALLEL_FOR(AnimationComponent, p_context, index, 1, UpdateAnimation(p_scene, index, p_timestep));
}

void RunArmatureUpdateSystem(Scene& p_scene, jobsystem::Context& p_context, float p_timestep) {
    HBN_PROFILE_EVENT();
    JS_PARALLEL_FOR(ArmatureComponent, p_context, index, 1, UpdateArmature(p_scene, index, p_timestep));
}

void RunHierarchyUpdateSystem(Scene& p_scene, jobsystem::Context& p_context, float p_timestep) {
    HBN_PROFILE_EVENT();
    JS_PARALLEL_FOR(HierarchyComponent, p_context, index, SMALL_SUBTASK_GROUP_SIZE, UpdateHierarchy(p_scene, index, p_timestep));
}

void RunObjectUpdateSystem(Scene& p_scene, jobsystem::Context& p_context, float) {
    HBN_PROFILE_EVENT();
    unused(p_context);

    AABB bound;

    for (auto [entity, obj] : p_scene.m_MeshRendererComponents) {
        if (!p_scene.Contains<TransformComponent>(entity)) {
            continue;
        }

        const TransformComponent& transform = *p_scene.GetComponent<TransformComponent>(entity);
        DEV_ASSERT(p_scene.Contains<MeshComponent>(obj.meshId));
        const MeshComponent& mesh = *p_scene.GetComponent<MeshComponent>(obj.meshId);

        Matrix4x4f M = transform.GetWorldMatrix();
        AABB aabb = mesh.localBound;
        aabb.ApplyMatrix(M);
        bound.UnionBox(aabb);
    }

    p_scene.m_bound = bound;
}

void RunParticleEmitterUpdateSystem(Scene& p_scene, jobsystem::Context& p_context, float) {
    HBN_PROFILE_EVENT();
    unused(p_context);

    for (auto [entity, emitter] : p_scene.m_ParticleEmitterComponents) {
        emitter.aliveBufferIndex = 1 - emitter.aliveBufferIndex;
    }
}

static void UpdateMeshEmitter(float p_timestep,
                              const TransformComponent& p_transform,
                              MeshEmitterComponent& p_emitter) {
    // initialize
    if (p_emitter.particles.empty()) {
        p_emitter.Reset();
    }

    if (!p_emitter.IsRunning()) {
        return;
    }

    // 1. emit new particles
    const int emission_count = min(p_emitter.emissionPerFrame, (int)p_emitter.deadList.size());
    p_emitter.aliveList.reserve(p_emitter.aliveList.size() + emission_count);
    const auto& position = p_transform.GetTranslation();
    for (int i = 0; i < emission_count; ++i) {
        DEV_ASSERT(!p_emitter.deadList.empty());
        const auto free_index = p_emitter.deadList.back();
        p_emitter.deadList.pop_back();
        p_emitter.aliveList.push_back(free_index);

        DEV_ASSERT(free_index.v < p_emitter.particles.size());
        auto& p = p_emitter.particles[free_index.v];

        Vector3f initial_speed{ 0 };
        initial_speed.x += Random::Float(p_emitter.vxRange.x, p_emitter.vxRange.y);
        initial_speed.y += Random::Float(p_emitter.vyRange.x, p_emitter.vyRange.y);
        initial_speed.z += Random::Float(p_emitter.vzRange.x, p_emitter.vzRange.y);
        Vector3f initial_rotation{
            Random::Float(-HalfPi(), HalfPi()),
            Random::Float(-HalfPi(), HalfPi()),
            Random::Float(-HalfPi(), HalfPi()),
        };

        p.Init(Random::Float(p_emitter.lifetimeRange.x, p_emitter.lifetimeRange.y),
               position,
               initial_speed,
               initial_rotation,
               p_emitter.scale);
    }

    // 2. update alive ones
    for (const auto index : p_emitter.aliveList) {
        p_emitter.UpdateParticle(index, p_timestep);
    }

    // 3. recycle
    std::vector<MeshEmitterComponent::Index> tmp;
    tmp.reserve(p_emitter.aliveList.size());
    const bool recycle = p_emitter.IsRecycle();
    for (int i = (int)p_emitter.aliveList.size() - 1; i >= 0; --i) {
        const auto index = p_emitter.aliveList[i];
        auto& p = p_emitter.particles[index.v];
        if (p.lifespan <= 0.0f) {
            p_emitter.aliveList.pop_back();
            if (recycle) {
                p_emitter.deadList.push_back(index);
            }
        } else {
            tmp.push_back(index);
        }
    }
    p_emitter.aliveList = std::move(tmp);
}

void RunMeshEmitterUpdateSystem(Scene& p_scene, jobsystem::Context& p_context, float p_timestep) {
    HBN_PROFILE_EVENT();

    unused(p_context);
    for (auto [id, emitter] : p_scene.m_MeshEmitterComponents) {
        const TransformComponent* transform = p_scene.GetComponent<TransformComponent>(id);
        if (DEV_VERIFY(transform)) {
            UpdateMeshEmitter(p_timestep, *transform, emitter);
        }
    }
}

}  // namespace my
