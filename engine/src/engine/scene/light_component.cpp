#include "light_component.h"

#include "engine/core/framework/graphics_manager.h"
#include "engine/core/io/archive.h"
#include "engine/core/math/matrix_transform.h"
#include "engine/renderer/renderer.h"
#include "engine/scene/transform_component.h"

namespace my {

void LightComponent::Update(const TransformComponent& p_transform) {
    m_position = p_transform.GetTranslation();

    if (IsDirty() || p_transform.IsDirty()) {
        // update max distance
        constexpr float atten_factor_inv = 1.0f / 0.03f;
        if (m_atten.linear == 0.0f && m_atten.quadratic == 0.0f) {
            m_maxDistance = 1000.0f;
        } else {
            // (constant + linear * x + quad * x^2) * atten_factor = 1
            // quad * x^2 + linear * x + constant - 1.0 / atten_factor = 0
            const float a = m_atten.quadratic;
            const float b = m_atten.linear;
            const float c = m_atten.constant - atten_factor_inv;

            float discriminant = b * b - 4 * a * c;
            if (discriminant < 0.0f) {
                CRASH_NOW_MSG("TODO: fix");
            }

            float sqrt_d = glm::sqrt(discriminant);
            float root1 = (-b + sqrt_d) / (2 * a);
            float root2 = (-b - sqrt_d) / (2 * a);
            m_maxDistance = root1 > 0.0f ? root1 : root2;
            m_maxDistance = glm::max(LIGHT_SHADOW_MIN_DISTANCE + 1.0f, m_maxDistance);
        }

        // update shadow map
        if (CastShadow()) {
            // @TODO: get rid of the
            if (m_shadowMapIndex == renderer::INVALID_POINT_SHADOW_HANDLE) {
                m_shadowMapIndex = renderer::AllocatePointLightShadowMap();
            }
        } else {
            if (m_shadowMapIndex != renderer::INVALID_POINT_SHADOW_HANDLE) {
                renderer::FreePointLightShadowMap(m_shadowMapIndex);
            }
        }

        // update light space matrices
        if (CastShadow()) {
            switch (m_type) {
                case LIGHT_TYPE_POINT: {
                    constexpr float near_plane = LIGHT_SHADOW_MIN_DISTANCE;
                    const float far_plane = m_maxDistance;
                    const bool is_opengl = GraphicsManager::GetSingleton().GetBackend() == Backend::OPENGL;
                    auto matrices = is_opengl ? BuildOpenGlPointLightCubeMapViewProjectionMatrix(m_position, near_plane, far_plane) : BuildPointLightCubeMapViewProjectionMatrix(m_position, near_plane, far_plane);

                    for (size_t i = 0; i < matrices.size(); ++i) {
                        m_lightSpaceMatrices[i] = matrices[i];
                    }
                } break;
                default:
                    break;
            }
        }

        // @TODO: query if transformation is dirty, so don't update shadow map unless necessary
        SetDirty(false);
    }
}

void LightComponent::Serialize(Archive& p_archive, uint32_t p_version) {
    unused(p_version);

    if (p_archive.IsWriteMode()) {
        p_archive << m_flags;
        p_archive << m_type;
        p_archive << m_atten;
        p_archive << m_shadowRegion;
    } else {
        p_archive >> m_flags;
        p_archive >> m_type;
        p_archive >> m_atten;
        if (p_version > 14) {
            p_archive >> m_shadowRegion;
        } else {
            m_flags &= ~(SHADOW_REGION);
        }

        m_flags |= DIRTY;
    }
}

}  // namespace my
