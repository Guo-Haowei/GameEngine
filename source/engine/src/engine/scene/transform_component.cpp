#include "transform_component.h"

#include "core/io/archive.h"

namespace my {

mat4 TransformComponent::get_local_matrix() const {
    mat4 rotationMatrix = glm::toMat4(quat(m_rotation.w, m_rotation.x, m_rotation.y, m_rotation.z));
    mat4 translationMatrix = glm::translate(m_translation);
    mat4 scaleMatrix = glm::scale(m_scale);
    return translationMatrix * rotationMatrix * scaleMatrix;
}

void TransformComponent::update_transform() {
    if (is_dirty()) {
        set_dirty(false);
        m_world_matrix = get_local_matrix();
    }
}

void TransformComponent::scale(const vec3& scale) {
    set_dirty();
    m_scale.x *= scale.x;
    m_scale.y *= scale.y;
    m_scale.z *= scale.z;
}

void TransformComponent::translate(const vec3& translation) {
    set_dirty();
    m_translation.x += translation.x;
    m_translation.y += translation.y;
    m_translation.z += translation.z;
}

void TransformComponent::rotate(const vec3& euler) {
    set_dirty();
    glm::quat quaternion(m_rotation.w, m_rotation.x, m_rotation.y, m_rotation.z);
    quaternion = glm::quat(euler) * quaternion;

    m_rotation.x = quaternion.x;
    m_rotation.y = quaternion.y;
    m_rotation.z = quaternion.z;
    m_rotation.w = quaternion.w;
}

void TransformComponent::set_local_transform(const mat4& matrix) {
    set_dirty();
    decompose(matrix, m_scale, m_rotation, m_translation);
}

void TransformComponent::matrix_transform(const mat4& matrix) {
    set_dirty();
    decompose(matrix * get_local_matrix(), m_scale, m_rotation, m_translation);
}

void TransformComponent::update_transform_parented(const TransformComponent& parent) {
    CRASH_NOW();
    mat4 worldMatrix = get_local_matrix();
    const mat4& worldMatrixParent = parent.m_world_matrix;
    m_world_matrix = worldMatrixParent * worldMatrix;
}

void TransformComponent::serialize(Archive& archive, uint32_t) {
    if (archive.is_write_mode()) {
        archive << m_flags;
        archive << m_scale;
        archive << m_translation;
        archive << m_rotation;
    } else {
        archive >> m_flags;
        archive >> m_scale;
        archive >> m_translation;
        archive >> m_rotation;
        set_dirty();
    }
}

}  // namespace my
