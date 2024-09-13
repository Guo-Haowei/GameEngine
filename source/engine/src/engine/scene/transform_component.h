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

    bool isDirty() const { return m_flags & DIRTY; }
    void setDirty(bool p_dirty = true) { p_dirty ? m_flags |= DIRTY : m_flags &= ~DIRTY; }

    const vec3& getTranslation() const { return m_translation; }
    void setTranslation(const vec3& p_translation) { m_translation = p_translation; }
    void increaseTranslation(const vec3& p_delta) { m_translation += p_delta; }

    const vec4& getRotation() const { return m_rotation; }
    void setRotation(const vec4& p_rotation) { m_rotation = p_rotation; }

    const vec3& getScale() const { return m_scale; }
    void setScale(const vec3& p_scale) { m_scale = p_scale; }

    const mat4& getWorldMatrix() const { return m_world_matrix; }

    void setWorldMatrix(const mat4& p_matrix) { m_world_matrix = p_matrix; }

    mat4 getLocalMatrix() const;

    void updateTransform();
    void scale(const vec3& p_scale);
    void translate(const vec3& p_translation);
    void rotate(const vec3& p_euler);

    void setLocalTransform(const mat4& p_matrix);
    void matrixTransform(const mat4& p_matrix);

    void updateTransformParented(const TransformComponent& p_parent);

    void serialize(Archive& p_archive, uint32_t p_version);

private:
    uint32_t m_flags = DIRTY;

    vec3 m_scale{ 1 };              // local scale
    vec3 m_translation{ 0 };        // local translation
    vec4 m_rotation{ 0, 0, 0, 1 };  // local rotation

    // Non-serialized attributes
    mat4 m_world_matrix{ 1 };
};

}  // namespace my