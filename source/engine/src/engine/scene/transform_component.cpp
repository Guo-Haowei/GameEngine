#include "transform_component.h"

#include "core/io/archive.h"

namespace my {

mat4 TransformComponent::getLocalMatrix() const {
    mat4 rotationMatrix = glm::toMat4(quat(m_rotation.w, m_rotation.x, m_rotation.y, m_rotation.z));
    mat4 translationMatrix = glm::translate(m_translation);
    mat4 scaleMatrix = glm::scale(m_scale);
    return translationMatrix * rotationMatrix * scaleMatrix;
}

void TransformComponent::updateTransform() {
    if (isDirty()) {
        setDirty(false);
        m_world_matrix = getLocalMatrix();
    }
}

void TransformComponent::scale(const vec3& p_scale) {
    setDirty();
    m_scale.x *= p_scale.x;
    m_scale.y *= p_scale.y;
    m_scale.z *= p_scale.z;
}

void TransformComponent::translate(const vec3& p_translation) {
    setDirty();
    m_translation.x += p_translation.x;
    m_translation.y += p_translation.y;
    m_translation.z += p_translation.z;
}

void TransformComponent::rotate(const vec3& p_euler) {
    setDirty();
    glm::quat quaternion(m_rotation.w, m_rotation.x, m_rotation.y, m_rotation.z);
    quaternion = glm::quat(p_euler) * quaternion;

    m_rotation.x = quaternion.x;
    m_rotation.y = quaternion.y;
    m_rotation.z = quaternion.z;
    m_rotation.w = quaternion.w;
}

void TransformComponent::setLocalTransform(const mat4& p_matrix) {
    setDirty();
    decompose(p_matrix, m_scale, m_rotation, m_translation);
}

void TransformComponent::matrixTransform(const mat4& p_matrix) {
    setDirty();
    decompose(p_matrix * getLocalMatrix(), m_scale, m_rotation, m_translation);
}

void TransformComponent::updateTransformParented(const TransformComponent& p_parent) {
    CRASH_NOW();
    mat4 worldMatrix = getLocalMatrix();
    const mat4& worldMatrixParent = p_parent.m_world_matrix;
    m_world_matrix = worldMatrixParent * worldMatrix;
}

void TransformComponent::serialize(Archive& p_archive, uint32_t) {
    if (p_archive.isWriteMode()) {
        p_archive << m_flags;
        p_archive << m_scale;
        p_archive << m_translation;
        p_archive << m_rotation;
    } else {
        p_archive >> m_flags;
        p_archive >> m_scale;
        p_archive >> m_translation;
        p_archive >> m_rotation;
        setDirty();
    }
}

}  // namespace my
