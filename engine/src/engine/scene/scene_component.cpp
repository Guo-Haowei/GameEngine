#include "scene_component.h"

#include "engine/core/framework/asset_registry.h"
#include "engine/core/io/archive.h"
#include "engine/core/math/matrix_transform.h"

namespace my {

#pragma region NAME_COMPONENT
#pragma endregion NAME_COMPONENT

#pragma region ANIMATION_COMPONENT
#pragma endregion ANIMATION_COMPONENT

#pragma region ARMATURE_COMPONENT
#pragma endregion ARMATURE_COMPONENT

#pragma region OBJECT_COMPONENT
#pragma endregion OBJECT_COMPONENT

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
#pragma endregion RIGID_BODY_COMPONENT

#pragma region SOFT_BODY_COMPONENT
#pragma endregion SOFT_BODY_COMPONENT

#pragma region ENVIRONMENT_COMPONENT
#pragma endregion ENVIRONMENT_COMPONENT

}  // namespace my
