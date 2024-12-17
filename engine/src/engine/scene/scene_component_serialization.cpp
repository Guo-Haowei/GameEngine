#include <yaml-cpp/yaml.h>

#include "engine/core/framework/asset_registry.h"
#include "engine/core/io/archive.h"
#include "engine/core/math/matrix_transform.h"
#include "scene_component.h"

namespace my {

static YAML::Node DumpVector3f(const Vector3f& p_vec) {
    YAML::Node node(YAML::NodeType::Sequence);
    node.push_back(p_vec.x);
    node.push_back(p_vec.y);
    node.push_back(p_vec.z);
    return node;
}

static YAML::Node DumpVector4f(const Vector4f& p_vec) {
    YAML::Node node(YAML::NodeType::Sequence);
    node.push_back(p_vec.x);
    node.push_back(p_vec.y);
    node.push_back(p_vec.z);
    node.push_back(p_vec.w);
    return node;
}

WARNING_PUSH()
WARNING_DISABLE(4100, "-Wunused-parameter")

void NameComponent::Serialize(Archive& p_archive, uint32_t) {
    if (p_archive.IsWriteMode()) {
        p_archive << m_name;
    } else {
        p_archive >> m_name;
    }
}

bool NameComponent::Dump(YAML::Emitter& p_emitter, Archive& p_archive, uint32_t p_version) const {
    p_emitter << YAML::Key << "name" << YAML::Value << m_name;
    return true;
}

bool NameComponent::Undump(const YAML::Node& p_node, Archive& p_archive, uint32_t p_version) {
    const auto& name = p_node["name"];
    m_name = name.as<std::string>();
    return true;
}

void HierarchyComponent::Serialize(Archive& p_archive, uint32_t) {
    if (p_archive.IsWriteMode()) {
        p_archive << m_parentId;
    } else {
        p_archive >> m_parentId;
    }
}

bool HierarchyComponent::Dump(YAML::Emitter& p_emitter, Archive& p_archive, uint32_t p_version) const {
    p_emitter << YAML::Key << "parent_id" << YAML::Value << m_parentId.GetId();
    return true;
}

bool HierarchyComponent::Undump(const YAML::Node& p_node, Archive& p_archive, uint32_t p_version) {
    auto id = p_node["parent_id"].as<uint32_t>();
    m_parentId = ecs::Entity{ id };
    return true;
}

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

bool TransformComponent::Dump(YAML::Emitter& p_emitter, Archive& p_archive, uint32_t p_version) const {
    p_emitter << YAML::Key << "flags" << YAML::Value << m_flags;
    p_emitter << YAML::Key << "translation" << YAML::Value << DumpVector3f(m_translation);
    p_emitter << YAML::Key << "rotation" << YAML::Value << DumpVector4f(m_rotation);
    p_emitter << YAML::Key << "scale" << YAML::Value << DumpVector3f(m_scale);
    return true;
}

bool TransformComponent::Undump(const YAML::Node& p_node, Archive& p_archive, uint32_t p_version) {
    CRASH_NOW();
    SetDirty();
    return true;
}

bool AnimationComponent::Dump(YAML::Emitter& p_emitter, Archive& p_archive, uint32_t p_version) const {
    // p_node["channels"] = channels;
    CRASH_NOW();
    return true;
}

bool AnimationComponent::Undump(const YAML::Node& p_node, Archive& p_archive, uint32_t p_version) {
    flags = p_node["flags"].as<uint32_t>();
    start = p_node["start"].as<float>();
    end = p_node["end"].as<float>();
    timer = p_node["timer"].as<float>();
    amount = p_node["amount"].as<float>();
    speed = p_node["speed"].as<float>();
    // channels = p_node["channels"].as<std::vector<float>>();
    CRASH_NOW();
    return true;
}

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

bool ArmatureComponent::Dump(YAML::Emitter& p_emitter, Archive& p_archive, uint32_t p_version) const {
    return true;
}

bool ArmatureComponent::Undump(const YAML::Node& p_node, Archive& p_archive, uint32_t p_version) {
    return true;
}

void ObjectComponent::Serialize(Archive& p_archive, uint32_t) {
    if (p_archive.IsWriteMode()) {
        p_archive << flags;
        p_archive << meshId;
    } else {
        p_archive >> flags;
        p_archive >> meshId;
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

bool PerspectiveCameraComponent::Dump(YAML::Emitter& p_emitter, Archive& p_archive, uint32_t p_version) const {

    return true;
}

bool PerspectiveCameraComponent::Undump(const YAML::Node& p_node, Archive& p_archive, uint32_t p_version) {
    SetDirty();
    return true;
}

void LuaScriptComponent::Serialize(Archive& p_archive, uint32_t p_version) {
    if (p_archive.IsWriteMode()) {
        p_archive << m_path;
    } else {
        std::string path;
        p_archive >> path;
        SetScript(path);
    }
}

void NativeScriptComponent::Serialize(Archive& p_archive, uint32_t p_version) {
    CRASH_NOW();
}

void CollisionObjectBase::Serialize(Archive& p_archive, uint32_t p_version) {
    if (p_archive.IsWriteMode()) {
        p_archive << collisionType;
        p_archive << collisionMask;
    } else {
        p_archive >> collisionType;
        p_archive >> collisionMask;
    }
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

void ClothComponent::Serialize(Archive& p_archive, uint32_t p_version) {
    CollisionObjectBase::Serialize(p_archive, p_version);

    CRASH_NOW();
    if (p_archive.IsWriteMode()) {
    } else {
    }
}

void EnvironmentComponent::Serialize(Archive& p_archive, uint32_t p_version) {
    CRASH_NOW();
}

WARNING_POP()

}  // namespace my
