#include "light_component.h"

#include "core/io/archive.h"
#include "rendering/renderer.h"
#include "scene/transform_component.h"

namespace my {

void LightComponent::update(const TransformComponent& p_transform) {
    m_position = p_transform.get_translation();

    if (is_dirty() || p_transform.is_dirty()) {
        // update max distance
        constexpr float atten_factor_inv = 1.0f / 0.03f;
        if (m_atten.linear == 0.0f && m_atten.quadratic == 0.0f) {
            m_max_distance = 1000.0f;
        } else {
            // (constant + linear * x + quad * x^2) * atten_factor = 1
            // quad * x^2 + linear * x + constant - 1.0 / atten_factor = 0
            const float a = m_atten.quadratic;
            const float b = m_atten.linear;
            const float c = m_atten.constant - atten_factor_inv;

            float discriminant = b * b - 4 * a * c;
            if (discriminant < 0.0f) {
                __debugbreak();
            }

            float sqrt_d = glm::sqrt(discriminant);
            float root1 = (-b + sqrt_d) / (2 * a);
            float root2 = (-b - sqrt_d) / (2 * a);
            m_max_distance = root1 > 0.0f ? root1 : root2;
            m_max_distance = glm::max(LIGHT_SHADOW_MIN_DISTANCE + 1.0f, m_max_distance);
        }

        // update shadow map
        if (cast_shadow()) {
            // @TODO: get rid of the
            if (m_shadow_map_index == INVALID_POINT_SHADOW_HANDLE) {
                m_shadow_map_index = renderer::allocate_point_light_shadow_map();
            }
        } else {
            if (m_shadow_map_index != INVALID_POINT_SHADOW_HANDLE) {
                renderer::free_point_light_shadow_map(m_shadow_map_index);
            }
        }

        // update light space matrices
        if (cast_shadow()) {
            switch (m_type) {
                case LIGHT_TYPE_POINT: {
                    constexpr float near_plane = LIGHT_SHADOW_MIN_DISTANCE;
                    const float far_plane = m_max_distance;
                    const glm::mat4 projection = glm::perspective(glm::radians(90.0f), 1.0f, near_plane, far_plane);
                    auto view_matrices = renderer::cube_map_view_matrices(m_position);
                    for (size_t i = 0; i < view_matrices.size(); ++i) {
                        m_light_space_matrices[i] = projection * view_matrices[i];
                    }
                } break;
                default:
                    break;
            }
        }

        // @TODO: query if transformation is dirty, so don't update shadow map unless necessary
        set_dirty(false);
    }
}

void LightComponent::serialize(Archive& archive, uint32_t version) {
    unused(version);

    if (archive.is_write_mode()) {
        archive << m_flags;
        archive << m_type;
        archive << m_color;
        archive << m_energy;
        archive << m_atten.constant;
        archive << m_atten.linear;
        archive << m_atten.quadratic;
    } else {
        if (version >= 4) {
            archive >> m_flags;
        }
        archive >> m_type;
        archive >> m_color;
        archive >> m_energy;
        archive >> m_atten.constant;
        archive >> m_atten.linear;
        archive >> m_atten.quadratic;

        m_flags |= DIRTY;
    }
}

}  // namespace my
