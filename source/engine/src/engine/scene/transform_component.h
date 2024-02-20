#pragma once
#include "core/math/geomath.h"

namespace my {

class Archive;

class TransformComponent {
public:
    enum : uint32_t {
        NONE = 0,
        DIRTY = 1 << 0,
    };

    bool is_dirty() const { return m_flags & DIRTY; }
    void set_dirty(bool dirty = true) { dirty ? m_flags |= DIRTY : m_flags &= ~DIRTY; }

    const vec3& get_translation() const { return m_translation; }
    void set_translation(const vec3& t) { m_translation = t; }
    void increase_translation(const vec3& t) { m_translation += t; }

    const vec4& get_rotation() const { return m_rotation; }
    void set_rotation(const vec4& r) { m_rotation = r; }

    const vec3& get_scale() const { return m_scale; }
    void set_scale(const vec3& s) { m_scale = s; }

    const mat4& get_world_matrix() const { return m_world_matrix; }

    void set_world_matrix(const mat4& matrix) { m_world_matrix = matrix; }

    mat4 get_local_matrix() const;

    void update_transform();
    void scale(const vec3& scale);
    void translate(const vec3& translation);
    void rotate(const vec3& euler);

    void set_local_transform(const mat4& matrix);
    void matrix_transform(const mat4& matrix);

    void update_transform_parented(const TransformComponent& parent);

    void serialize(Archive& archive, uint32_t version);

private:
    uint32_t m_flags = DIRTY;

    vec3 m_scale{ 1 };              // local scale
    vec3 m_translation{ 0 };        // local translation
    vec4 m_rotation{ 0, 0, 0, 1 };  // local rotation

    // Non-serialized attributes
    mat4 m_world_matrix = mat4(1);
};

}  // namespace my