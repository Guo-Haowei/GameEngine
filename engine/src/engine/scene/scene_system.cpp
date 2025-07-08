#include "scene_system.h"

#include "engine/core/base/random.h"
#include "engine/math/matrix_transform.h"
#include "engine/renderer/base_graphics_manager.h"
#include "engine/renderer/renderer.h"

namespace my {

void UpdateMeshEmitter(float p_timestep,
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

void UpdateLight(float p_timestep,
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
            // @TODO: get rid of the
            if (p_light.m_shadowMapIndex == INVALID_POINT_SHADOW_HANDLE) {
                p_light.m_shadowMapIndex = renderer::AllocatePointLightShadowMap();
            }
        } else {
            if (p_light.m_shadowMapIndex != INVALID_POINT_SHADOW_HANDLE) {
                renderer::FreePointLightShadowMap(p_light.m_shadowMapIndex);
            }
        }

        // update light space matrices
        if (p_light.CastShadow()) {
            switch (p_light.m_type) {
                case LIGHT_TYPE_POINT: {
                    constexpr float near_plane = LIGHT_SHADOW_MIN_DISTANCE;
                    const float far_plane = p_light.m_maxDistance;
                    const bool is_opengl = IGraphicsManager::GetSingleton().GetBackend() == Backend::OPENGL;
                    auto matrices = is_opengl ? BuildOpenGlPointLightCubeMapViewProjectionMatrix(p_light.m_position, near_plane, far_plane)
                                              : BuildPointLightCubeMapViewProjectionMatrix(p_light.m_position, near_plane, far_plane);

                    for (size_t i = 0; i < matrices.size(); ++i) {
                        p_light.m_lightSpaceMatrices[i] = matrices[i];
                    }
                } break;
                default:
                    break;
            }
        }

        // @TODO: query if transformation is dirty, so don't update shadow map unless necessary
        p_light.SetDirty(false);
    }
}

}  // namespace my
