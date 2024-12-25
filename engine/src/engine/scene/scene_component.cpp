#include "scene_component.h"

#include <yaml-cpp/yaml.h>

#include "engine/core/framework/asset_registry.h"
#include "engine/core/io/archive.h"
#include "engine/math/matrix_transform.h"

namespace my {

#pragma region TRANSFORM_COMPONENT
Matrix4x4f TransformComponent::GetLocalMatrix() const {
    Matrix4x4f rotationMatrix = glm::toMat4(Quaternion(m_rotation.w, m_rotation.x, m_rotation.y, m_rotation.z));
    Matrix4x4f translationMatrix = math::Translate(m_translation);
    Matrix4x4f scaleMatrix = math::Scale(m_scale);
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
    glm::quat euler(glm::vec3(p_euler.x, p_euler.y, p_euler.z));
    quaternion = euler * quaternion;

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
#pragma endregion TRANSFORM_COMPONENT

#pragma region MESH_COMPONENT
template<typename T>
static void InitVertexAttrib(MeshComponent::VertexAttribute& p_attrib, const std::vector<T>& p_buffer) {
    p_attrib.offsetInByte = 0;
    p_attrib.strideInByte = sizeof(p_buffer[0]);
    p_attrib.elementCount = static_cast<uint32_t>(p_buffer.size());
}

void MeshComponent::CreateRenderData() {
    // AABB
    localBound.MakeInvalid();
    for (MeshSubset& subset : subsets) {
        subset.local_bound.MakeInvalid();
        for (uint32_t i = 0; i < subset.index_count; ++i) {
            const Vector3f& point = positions[indices[i + subset.index_offset]];
            subset.local_bound.ExpandPoint(reinterpret_cast<const Vector3f&>(point));
        }
        subset.local_bound.MakeValid();
        localBound.UnionBox(subset.local_bound);
    }
    // Attributes
    for (int i = 0; i < std::to_underlying(VertexAttributeName::COUNT); ++i) {
        attributes[i].attribName = static_cast<VertexAttributeName>(i);
    }

    InitVertexAttrib(attributes[std::to_underlying(VertexAttributeName::POSITION)], positions);
    InitVertexAttrib(attributes[std::to_underlying(VertexAttributeName::NORMAL)], normals);
    InitVertexAttrib(attributes[std::to_underlying(VertexAttributeName::TEXCOORD_0)], texcoords_0);
    InitVertexAttrib(attributes[std::to_underlying(VertexAttributeName::TEXCOORD_1)], texcoords_1);
    InitVertexAttrib(attributes[std::to_underlying(VertexAttributeName::TANGENT)], tangents);
    InitVertexAttrib(attributes[std::to_underlying(VertexAttributeName::JOINTS_0)], joints_0);
    InitVertexAttrib(attributes[std::to_underlying(VertexAttributeName::WEIGHTS_0)], weights_0);
    InitVertexAttrib(attributes[std::to_underlying(VertexAttributeName::COLOR_0)], color_0);
    return;
}
#pragma endregion MESH_COMPONENT

#pragma region MATERIAL_COMPONENT
#pragma endregion MATERIAL_COMPONENT

#pragma region CAMERA_COMPONENT
void PerspectiveCameraComponent::Update() {
    if (IsDirty()) {
        m_front.x = m_yaw.Cos() * m_pitch.Cos();
        m_front.y = m_pitch.Sin();
        m_front.z = m_yaw.Sin() * m_pitch.Cos();

        m_right = math::cross(m_front, Vector3f::UnitY);

        m_viewMatrix = LookAtRh(m_position, m_position + m_front, Vector3f::UnitY);
        m_projectionMatrix = BuildOpenGlPerspectiveRH(m_fovy.GetRadians(), GetAspect(), m_near, m_far);
        m_projectionViewMatrix = m_projectionMatrix * m_viewMatrix;

        SetDirty(false);
    }
}

void PerspectiveCameraComponent::SetDimension(int p_width, int p_height) {
    if (m_width != p_width || m_height != p_height) {
        m_width = p_width;
        m_height = p_height;
        SetDirty();
    }
}
#pragma endregion CAMERA_COMPONENT

#pragma region LUA_SCRIPT_COMPONENT
LuaScriptComponent& LuaScriptComponent::SetClassName(std::string_view p_class_name) {
    if (DEV_VERIFY(!p_class_name.empty())) {
        m_className = p_class_name;
    }

    return *this;
}

LuaScriptComponent& LuaScriptComponent::SetPath(std::string_view p_path) {
    if (p_path != m_path) {
        // LOG_VERBOSE("changing script '{}' to '{}'", m_path, p_path);
        m_path = p_path;
    }

    return *this;
}
#pragma endregion LUA_SCRIPT_COMPONENT

#pragma region NATIVE_SCRIPT_COMPONENT
NativeScriptComponent::~NativeScriptComponent() {
    // DON'T DELETE instance, because it could be copied from other script
    // Memory leak!!!
}

NativeScriptComponent::NativeScriptComponent(const NativeScriptComponent& p_rhs) {
    *this = p_rhs;
}

NativeScriptComponent& NativeScriptComponent::operator=(const NativeScriptComponent& p_rhs) {
    instantiateFunc = p_rhs.instantiateFunc;
    destroyFunc = p_rhs.destroyFunc;
    instance = p_rhs.instance;
    return *this;
}
#pragma endregion NATIVE_SCRIPT_COMPONENT

#pragma region RIGID_BODY_COMPONENT
RigidBodyComponent& RigidBodyComponent::InitCube(const Vector3f& p_half_size) {
    shape = SHAPE_CUBE;
    size = p_half_size;
    return *this;
}

RigidBodyComponent& RigidBodyComponent::InitSphere(float p_radius) {
    shape = SHAPE_SPHERE;
    size = Vector3f(p_radius);
    return *this;
}

RigidBodyComponent& RigidBodyComponent::InitGhost() {
    objectType = GHOST;
    mass = 1.0f;
    return *this;
}
#pragma endregion RIGID_BODY_COMPONENT

#pragma region MESH_EMITTER_COMPONENT
void MeshEmitterComponent::Reset() {
    if ((int)particles.size() != maxMeshCount) {
        particles.resize(maxMeshCount);
    }

    aliveList.clear();
    aliveList.reserve(maxMeshCount);
    deadList.clear();
    deadList.reserve(maxMeshCount);
    for (int i = 0; i < maxMeshCount; ++i) {
        deadList.emplace_back(i);
    }
}

void MeshEmitterComponent::UpdateParticle(Index p_index, float p_timestep) {
    DEV_ASSERT(p_index.v < particles.size());
    auto& p = particles[p_index.v];
    DEV_ASSERT(p.lifespan >= 0.0f);

    p.scale *= (1.0f - p_timestep);
    p.scale = math::max(p.scale, 0.1f);
    p.velocity += p_timestep * gravity;
    p.rotation += Vector3f(p_timestep);
    p.lifespan -= p_timestep;

    p.position += p_timestep * p.velocity;
}

#pragma endregion MESH_EMITTER_COMPONENT

#pragma region SOFT_BODY_COMPONENT
#pragma endregion SOFT_BODY_COMPONENT

#pragma region ENVIRONMENT_COMPONENT
#pragma endregion ENVIRONMENT_COMPONENT

}  // namespace my
