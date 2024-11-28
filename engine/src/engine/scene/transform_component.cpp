#include "transform_component.h"

#include "engine/core/io/archive.h"

namespace my {

Matrix4x4f TransformComponent::GetLocalMatrix() const {
    Matrix4x4f rotationMatrix = glm::toMat4(Quaternion(m_rotation.w, m_rotation.x, m_rotation.y, m_rotation.z));
    Matrix4x4f translationMatrix = glm::translate(m_translation);
    Matrix4x4f scaleMatrix = glm::scale(m_scale);
    return translationMatrix * rotationMatrix * scaleMatrix;
}

void TransformComponent::UpdateTransform() {
    if (IsDirty()) {
        SetDirty(false);
        m_worldMatrix = GetLocalMatrix();
    }
}

void TransformComponent::Scale(const Vector3f& p_scale) {
    SetDirty();
    m_scale.x *= p_scale.x;
    m_scale.y *= p_scale.y;
    m_scale.z *= p_scale.z;
}

void TransformComponent::Translate(const Vector3f& p_translation) {
    SetDirty();
    m_translation.x += p_translation.x;
    m_translation.y += p_translation.y;
    m_translation.z += p_translation.z;
}

void TransformComponent::Rotate(const Vector3f& p_euler) {
    SetDirty();
    glm::quat quaternion(m_rotation.w, m_rotation.x, m_rotation.y, m_rotation.z);
    quaternion = glm::quat(p_euler) * quaternion;

    m_rotation.x = quaternion.x;
    m_rotation.y = quaternion.y;
    m_rotation.z = quaternion.z;
    m_rotation.w = quaternion.w;
}

void TransformComponent::SetLocalTransform(const Matrix4x4f& p_matrix) {
    SetDirty();
    Decompose(p_matrix, m_scale, m_rotation, m_translation);
}

void TransformComponent::MatrixTransform(const Matrix4x4f& p_matrix) {
    SetDirty();
    Decompose(p_matrix * GetLocalMatrix(), m_scale, m_rotation, m_translation);
}

void TransformComponent::UpdateTransformParented(const TransformComponent& p_parent) {
    CRASH_NOW();
    Matrix4x4f worldMatrix = GetLocalMatrix();
    const Matrix4x4f& worldMatrixParent = p_parent.m_worldMatrix;
    m_worldMatrix = worldMatrixParent * worldMatrix;
}

void TransformComponent::Serialize(Archive& p_archive, uint32_t) {
    if (p_archive.IsWriteMode()) {
        p_archive << m_flags;
        p_archive << m_scale;
        p_archive << m_translation;
        p_archive << m_rotation;
    } else {
        p_archive >> m_flags;
        p_archive >> m_scale;
        p_archive >> m_translation;
        p_archive >> m_rotation;
        SetDirty();
    }
}

}  // namespace my
