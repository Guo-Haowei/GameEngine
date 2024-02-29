#pragma once
#include "core/math/geomath.h"
#include "hlsl/shader_defines.h"

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

    bool is_dirty() const { return m_flags & DIRTY; }
    void set_dirty(bool p_dirty = true) { p_dirty ? m_flags |= DIRTY : m_flags &= ~DIRTY; }

    bool cast_shadow() const { return m_flags & CAST_SHADOW; }
    void set_cast_shadow(bool p_cast = true) { p_cast ? m_flags |= CAST_SHADOW : m_flags &= ~CAST_SHADOW; }

    int get_type() const { return m_type; }
    void set_type(int p_type) { m_type = p_type; }

    float get_max_distance() const { return m_max_distance; }
    int get_shadow_map_index() const { return m_shadow_map_index; }

    void update(const TransformComponent& p_transform);

    void serialize(Archive& archive, uint32_t version);

    vec3 m_color = vec3(1);
    float m_energy = 10.0f;
    struct {
        float constant;
        float linear;
        float quadratic;
    } m_atten;

    const auto& get_matrices() const { return m_light_space_matrices; }
    const vec3& get_position() const { return m_position; }

private:
    uint32_t m_flags = DIRTY;
    int m_type = LIGHT_TYPE_OMNI;

    // Non-serialized
    float m_max_distance;
    vec3 m_position;
    int m_shadow_map_index = -1;
    std::array<mat4, 6> m_light_space_matrices;
};

}  // namespace my
