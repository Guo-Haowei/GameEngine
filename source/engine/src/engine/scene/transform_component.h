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

    bool IsDirty() const { return m_flags & DIRTY; }
    void SetDirty(bool p_dirty = true) { p_dirty ? m_flags |= DIRTY : m_flags &= ~DIRTY; }

    const vec3& GetTranslation() const { return m_translation; }
    void SetTranslation(const vec3& p_translation) { m_translation = p_translation; }
    void IncreaseTranslation(const vec3& p_delta) { m_translation += p_delta; }

    const vec4& GetRotation() const { return m_rotation; }
    void SetRotation(const vec4& p_rotation) { m_rotation = p_rotation; }

    const vec3& GetScale() const { return m_scale; }
    void SetScale(const vec3& p_scale) { m_scale = p_scale; }

    const mat4& GetWorldMatrix() const { return m_worldMatrix; }

    void SetWorldMatrix(const mat4& p_matrix) { m_worldMatrix = p_matrix; }

    mat4 GetLocalMatrix() const;

    void UpdateTransform();
    void Scale(const vec3& p_scale);
    void Translate(const vec3& p_translation);
    void Rotate(const vec3& p_euler);

    void SetLocalTransform(const mat4& p_matrix);
    void MatrixTransform(const mat4& p_matrix);

    void UpdateTransformParented(const TransformComponent& p_parent);

    void Serialize(Archive& p_archive, uint32_t p_version);

private:
    uint32_t m_flags = DIRTY;

    vec3 m_scale{ 1 };              // local scale
    vec3 m_translation{ 0 };        // local translation
    vec4 m_rotation{ 0, 0, 0, 1 };  // local rotation

    // Non-serialized attributes
    mat4 m_worldMatrix{ 1 };
};

}  // namespace my