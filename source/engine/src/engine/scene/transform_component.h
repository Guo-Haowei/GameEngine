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

    const Vector3f& GetTranslation() const { return m_translation; }
    void SetTranslation(const Vector3f& p_translation) { m_translation = p_translation; }
    void IncreaseTranslation(const Vector3f& p_delta) { m_translation += p_delta; }

    const Vector4f& GetRotation() const { return m_rotation; }
    void SetRotation(const Vector4f& p_rotation) { m_rotation = p_rotation; }

    const Vector3f& GetScale() const { return m_scale; }
    void SetScale(const Vector3f& p_scale) { m_scale = p_scale; }

    const Matrix4x4f& GetWorldMatrix() const { return m_worldMatrix; }

    void SetWorldMatrix(const Matrix4x4f& p_matrix) { m_worldMatrix = p_matrix; }

    Matrix4x4f GetLocalMatrix() const;

    void UpdateTransform();
    void Scale(const Vector3f& p_scale);
    void Translate(const Vector3f& p_translation);
    void Rotate(const Vector3f& p_euler);

    void SetLocalTransform(const Matrix4x4f& p_matrix);
    void MatrixTransform(const Matrix4x4f& p_matrix);

    void UpdateTransformParented(const TransformComponent& p_parent);

    void Serialize(Archive& p_archive, uint32_t p_version);

private:
    uint32_t m_flags = DIRTY;

    Vector3f m_scale{ 1 };              // local scale
    Vector3f m_translation{ 0 };        // local translation
    Vector4f m_rotation{ 0, 0, 0, 1 };  // local rotation

    // Non-serialized attributes
    Matrix4x4f m_worldMatrix{ 1 };
};

}  // namespace my