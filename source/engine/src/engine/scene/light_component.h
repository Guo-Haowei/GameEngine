#pragma once
#include "core/math/geomath.h"
#include "shader_defines.h"

namespace my {

class Archive;
class TransformComponent;

class LightComponent {
public:
    enum : uint32_t {
        NONE = BIT(0),
        DIRTY = BIT(1),
        CAST_SHADOW = BIT(2),
    };

    bool isDirty() const { return m_flags & DIRTY; }
    void setDirty(bool p_dirty = true) { p_dirty ? m_flags |= DIRTY : m_flags &= ~DIRTY; }

    bool castShadow() const { return m_flags & CAST_SHADOW; }
    void setCastShadow(bool p_cast = true) { p_cast ? m_flags |= CAST_SHADOW : m_flags &= ~CAST_SHADOW; }

    int getType() const { return m_type; }
    void setType(int p_type) { m_type = p_type; }

    float getMaxDistance() const { return m_max_distance; }
    int getShadowMapIndex() const { return m_shadow_map_index; }

    void update(const TransformComponent& p_transform);

    void Serialize(Archive& p_archive, uint32_t p_version);

    const auto& getMatrices() const { return m_light_space_matrices; }
    const vec3& getPosition() const { return m_position; }

    struct {
        float constant;
        float linear;
        float quadratic;
    } m_atten;

private:
    uint32_t m_flags = DIRTY;
    int m_type = LIGHT_TYPE_INFINITE;

    // Non-serialized
    float m_max_distance;
    vec3 m_position;
    int m_shadow_map_index = -1;
    std::array<mat4, 6> m_light_space_matrices;
};

}  // namespace my
