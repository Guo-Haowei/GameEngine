#include "scene_component.h"

#include "engine/core/framework/asset_registry.h"
#include "engine/core/io/archive.h"
#include "engine/core/math/matrix_transform.h"

namespace my {

#pragma region NAME_COMPONENT
void NameComponent::Serialize(Archive& p_archive, uint32_t) {
    if (p_archive.IsWriteMode()) {
        p_archive << m_name;
    } else {
        p_archive >> m_name;
    }
}
#pragma endregion NAME_COMPONENT

#pragma region ANIMATION_COMPONENT
void AnimationComponent::Serialize(Archive& p_archive, uint32_t) {
    if (p_archive.IsWriteMode()) {
        p_archive << flags;
        p_archive << start;
        p_archive << end;
        p_archive << timer;
        p_archive << amount;
        p_archive << speed;
        p_archive << channels;

        uint64_t num_samplers = samplers.size();
        p_archive << num_samplers;
        for (uint64_t i = 0; i < num_samplers; ++i) {
            p_archive << samplers[i].keyframeTmes;
            p_archive << samplers[i].keyframeData;
        }
    } else {
        p_archive >> flags;
        p_archive >> start;
        p_archive >> end;
        p_archive >> timer;
        p_archive >> amount;
        p_archive >> speed;
        p_archive >> channels;

        uint64_t num_samplers = 0;
        p_archive >> num_samplers;
        samplers.resize(num_samplers);
        for (uint64_t i = 0; i < num_samplers; ++i) {
            p_archive >> samplers[i].keyframeTmes;
            p_archive >> samplers[i].keyframeData;
        }
    }
}
#pragma endregion ANIMATION_COMPONENT

#pragma region ARMATURE_COMPONENT
void ArmatureComponent::Serialize(Archive& p_archive, uint32_t) {
    if (p_archive.IsWriteMode()) {
        p_archive << flags;
        p_archive << boneCollection;
        p_archive << inverseBindMatrices;
    } else {
        p_archive >> flags;
        p_archive >> boneCollection;
        p_archive >> inverseBindMatrices;
    }
}
#pragma endregion ARMATURE_COMPONENT

#pragma region CAMERA_COMPONENT
void PerspectiveCameraComponent::Update() {
    if (IsDirty()) {
        m_front.x = m_yaw.Cos() * m_pitch.Cos();
        m_front.y = m_pitch.Sin();
        m_front.z = m_yaw.Sin() * m_pitch.Cos();

        m_right = glm::cross(m_front, Vector3f(0, 1, 0));

        m_viewMatrix = glm::lookAt(m_position, m_position + m_front, Vector3f(0, 1, 0));
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

void PerspectiveCameraComponent::Serialize(Archive& p_archive, uint32_t) {
    if (p_archive.IsWriteMode()) {
        p_archive << m_flags;
        p_archive << m_near;
        p_archive << m_far;
        p_archive << m_fovy;
        p_archive << m_width;
        p_archive << m_height;
        p_archive << m_pitch;
        p_archive << m_yaw;
        p_archive << m_position;
    } else {
        p_archive >> m_flags;
        p_archive >> m_near;
        p_archive >> m_far;
        p_archive >> m_fovy;
        p_archive >> m_width;
        p_archive >> m_height;
        p_archive >> m_pitch;
        p_archive >> m_yaw;
        p_archive >> m_position;

        SetDirty();
    }
}
#pragma endregion CAMERA_COMPONENT

#pragma region LUA_SCRIPT_COMPONENT
std::string& LuaScriptComponent::GetScriptRef() {
    return m_path;
}

void LuaScriptComponent::SetScript(const std::string& p_path) {
    if (p_path.empty()) {
        return;
    }

    if (p_path == m_path) {
        return;
    }

    m_path = p_path;
    m_asset = nullptr;
    AssetRegistry::GetSingleton().RequestAssetAsync(p_path);
}

const char* LuaScriptComponent::GetSource() const {
    return m_asset ? m_asset->source.c_str() : nullptr;
}

void LuaScriptComponent::Serialize(Archive& p_archive, uint32_t p_version) {
    unused(p_version);

    if (p_archive.IsWriteMode()) {
        p_archive << m_path;
    } else {
        std::string path;
        p_archive >> path;
        SetScript(path);
    }
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

void NativeScriptComponent::Serialize(Archive& p_archive, uint32_t p_version) {
    unused(p_archive);
    unused(p_version);
    CRASH_NOW();
}
#pragma endregion NATIVE_SCRIPT_COMPONENT

#pragma region RIGID_BODY_COMPONENT
void CollisionObjectBase::Serialize(Archive& p_archive, uint32_t p_version) {
    unused(p_version);

    if (p_archive.IsWriteMode()) {
        p_archive << collisionType;
        p_archive << collisionMask;
    } else {
        p_archive >> collisionType;
        p_archive >> collisionMask;
    }
}

RigidBodyComponent& RigidBodyComponent::InitCube(const Vector3f& p_half_size) {
    shape = SHAPE_CUBE;
    param.box.half_size = p_half_size;
    return *this;
}

RigidBodyComponent& RigidBodyComponent::InitSphere(float p_radius) {
    shape = SHAPE_SPHERE;
    param.sphere.radius = p_radius;
    return *this;
}

RigidBodyComponent& RigidBodyComponent::InitGhost() {
    objectType = GHOST;
    mass = 1.0f;
    return *this;
}

void RigidBodyComponent::Serialize(Archive& p_archive, uint32_t p_version) {
    CollisionObjectBase::Serialize(p_archive, p_version);

    if (p_archive.IsWriteMode()) {
        p_archive << shape;
        p_archive << objectType;
        p_archive << param;
        p_archive << mass;
    } else {
        p_archive >> shape;
        p_archive >> objectType;
        p_archive >> param;
        p_archive >> mass;
    }
}
#pragma endregion RIGID_BODY_COMPONENT

#pragma region SOFT_BODY_COMPONENT
void ClothComponent::Serialize(Archive& p_archive, uint32_t p_version) {
    CollisionObjectBase::Serialize(p_archive, p_version);

    CRASH_NOW();
    if (p_archive.IsWriteMode()) {
    } else {
    }
}
#pragma endregion SOFT_BODY_COMPONENT

#pragma region ENVIRONMENT_COMPONENT
void EnvironmentComponent::Serialize(Archive& p_archive, uint32_t p_version) {
    unused(p_archive);
    unused(p_version);
    CRASH_NOW();
}
#pragma endregion ENVIRONMENT_COMPONENT

}  // namespace my
