#include "scene_serialization.h"

#include <yaml-cpp/yaml.h>

#include <fstream>

#include "engine/core/io/archive.h"
#include "engine/core/string/string_utils.h"
#include "engine/scene/scene.h"

namespace my {

#define SCENE_DBG_LOG(...) LOG_VERBOSE(__VA_ARGS__)

#pragma region VERSION_HISTORY
// SCENE_VERSION history
// version 1: initial version
// version 2: don't serialize scene.m_bound
// version 3: light component atten
// version 4: light component flags
// version 5: add validation
// version 6: add collider component
// version 7: add enabled to material
// version 8: add particle emitter
// version 9: add ParticleEmitterComponent.gravity
// version 10: add ForceFieldComponent
// version 11: add ScriptFieldComponent
// version 12: add CameraComponent
// version 13: add SoftBodyComponent
// version 14: modify RigidBodyComponent
// version 15: add predefined shadow region to lights
// version 16: change scene binary representation
#pragma endregion VERSION_HISTORY
static constexpr uint32_t SCENE_VERSION = 16;
static constexpr uint32_t SCENE_MAGIC_SIZE = 8;
static constexpr char SCENE_MAGIC[SCENE_MAGIC_SIZE] = "xScene";
static constexpr char SCENE_GUARD_MESSAGE[] = "Should see this message";
static constexpr uint64_t HAS_NEXT_FLAG = 6368519827137030510;

Result<void> SaveSceneBinary(const std::string& p_path, Scene& p_scene) {
    Archive archive;
    if (auto res = archive.OpenWrite(p_path); !res) {
        return HBN_ERROR(res.error());
    }

    archive << SCENE_MAGIC;
    archive << SCENE_VERSION;
    archive << ecs::Entity::GetSeed();
    archive << p_scene.m_root;

    archive << SCENE_GUARD_MESSAGE;

    for (const auto& it : p_scene.GetLibraryEntries()) {
        archive << HAS_NEXT_FLAG;
        archive << it.first;  // write name
        it.second.m_manager->Serialize(archive, SCENE_VERSION);
    }
    archive << uint64_t(0);
    return Result<void>();
}

Result<void> LoadSceneBinary(const std::string& p_path, Scene& p_scene) {
    Archive archive;
    if (auto res = archive.OpenRead(p_path); !res) {
        return HBN_ERROR(res.error());
    }

    uint32_t version = UINT_MAX;

    uint32_t seed = ecs::Entity::MAX_ID;

    char magic[SCENE_MAGIC_SIZE]{ 0 };
    archive >> magic;
    if (!StringUtils::StringEqual(magic, SCENE_MAGIC)) {
        return HBN_ERROR(ErrorCode::ERR_FILE_CORRUPT, "magic is not 'xScn'");
    }

    archive >> version;
    if (version > SCENE_VERSION) {
        return HBN_ERROR(ErrorCode::ERR_FILE_CORRUPT, "file version {} is greater than max version {}", version, SCENE_VERSION);
    }

    LOG_OK("loading scene '{}', version: {}", p_path, version);

    archive >> seed;
    ecs::Entity::SetSeed(seed);

    archive >> p_scene.m_root;

    char guard_message[sizeof(SCENE_GUARD_MESSAGE)]{ 0 };
    archive >> guard_message;
    if (!StringUtils::StringEqual(guard_message, SCENE_GUARD_MESSAGE)) {
        return HBN_ERROR(ErrorCode::ERR_FILE_CORRUPT);
    }

    for (;;) {
        uint64_t has_next = 0;
        archive >> has_next;
        if (has_next != HAS_NEXT_FLAG) {
            return Result<void>();
        }

        std::string key;
        archive >> key;

        SCENE_DBG_LOG("Loading Component {}", key);

        auto it = p_scene.GetLibraryEntries().find(key);
        if (it == p_scene.GetLibraryEntries().end()) {
            return HBN_ERROR(ErrorCode::ERR_FILE_CORRUPT, "entry '{}' not found", key);
        }
        if (!it->second.m_manager->Serialize(archive, version)) {
            return HBN_ERROR(ErrorCode::ERR_FILE_CORRUPT, "failed to serialize '{}'", key);
        }
    }
}

template<Serializable T>
static void EmitComponent(YAML::Emitter& p_out,
                          const char* p_name,
                          ecs::Entity p_entity,
                          const Scene& p_scene,
                          Archive& p_archive) {
    const T* component = p_scene.GetComponent<T>(p_entity);
    if (component) {
        p_out << YAML::Key << p_name << YAML::Value;
        p_out << YAML::BeginMap;
        component->Dump(p_out, p_archive, SCENE_VERSION);
        p_out << YAML::EndMap;
    }
}

Result<void> SaveSceneText(const std::string& p_path, const Scene& p_scene) {
    YAML::Emitter out;
    out << YAML::BeginMap;
    out << YAML::Key << "version" << YAML::Value << SCENE_VERSION;
    out << YAML::Key << "seed" << YAML::Value << ecs::Entity::GetSeed();
    out << YAML::Key << "root" << YAML::Value << p_scene.m_root.GetId();

    std::unordered_set<uint32_t> entity_set;

    Archive archive;
    for (const auto& it : p_scene.GetLibraryEntries()) {
        auto& manager = it.second.m_manager;
        for (auto entity : manager->GetEntityArray()) {
            entity_set.insert(entity.GetId());
        }
    }

    std::vector<uint32_t> entity_array(entity_set.begin(), entity_set.end());
    std::sort(entity_array.begin(), entity_array.end());

    out << YAML::Key << "entities" << YAML::Value;
    out << YAML::BeginSeq;

    for (auto id : entity_array) {
        ecs::Entity entity{ id };

        out << YAML::BeginMap;
        out << YAML::Key << "id" << YAML::Value << id;

        out.SetSeqFormat(YAML::Flow);
        EmitComponent<NameComponent>(out, "name_component", entity, p_scene, archive);
        EmitComponent<HierarchyComponent>(out, "hierarchy_component", entity, p_scene, archive);
        EmitComponent<TransformComponent>(out, "transform_component", entity, p_scene, archive);
        out.SetSeqFormat(YAML::Block);

        out << YAML::EndMap;
    }

    out << YAML::EndSeq;
    out << YAML::EndMap;

    auto res = FileAccess::Open(p_path, FileAccess::WRITE);
    if (!res) {
        return HBN_ERROR(res.error());
    }

    auto file_access = *res;
    const char* result = out.c_str();
    file_access->WriteBuffer(result, strlen(result));
    return Result<void>();
}

WARNING_PUSH()
WARNING_DISABLE(4100, "-Wunused-parameter")
#pragma region SCENE_COMPONENT_SERIALIZATION
static void EmitVector3f(YAML::Emitter& p_out, const Vector3f& p_vec) {
    p_out << YAML::BeginSeq;
    p_out << p_vec.x;
    p_out << p_vec.y;
    p_out << p_vec.z;
    p_out << YAML::EndSeq;
}

static void EmitVector4f(YAML::Emitter& p_out, const Vector4f& p_vec) {
    p_out << YAML::BeginSeq;
    p_out << p_vec.x;
    p_out << p_vec.y;
    p_out << p_vec.z;
    p_out << YAML::EndSeq;
}

void NameComponent::Serialize(Archive& p_archive, uint32_t) {
    if (p_archive.IsWriteMode()) {
        p_archive << m_name;
    } else {
        p_archive >> m_name;
    }
}

bool NameComponent::Dump(YAML::Emitter& p_out, Archive& p_archive, uint32_t p_version) const {
    p_out << YAML::Key << "name" << YAML::Value << m_name;
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

bool HierarchyComponent::Dump(YAML::Emitter& p_out, Archive& p_archive, uint32_t p_version) const {
    p_out << YAML::Key << "parent_id" << YAML::Value << m_parentId.GetId();
    return true;
}

bool HierarchyComponent::Undump(const YAML::Node& p_node, Archive& p_archive, uint32_t p_version) {
    auto id = p_node["parent_id"].as<uint32_t>();
    m_parentId = ecs::Entity{ id };
    return true;
}

bool AnimationComponent::Dump(YAML::Emitter& p_out, Archive& p_archive, uint32_t p_version) const {
    // p_node["channels"] = channels;
    CRASH_NOW();
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

bool TransformComponent::Dump(YAML::Emitter& p_out, Archive& p_archive, uint32_t p_version) const {
    p_out << YAML::Key << "flags" << YAML::Value << m_flags;
    p_out << YAML::Key << "translation" << YAML::Value;
    EmitVector3f(p_out, m_translation);
    p_out << YAML::Key << "rotation" << YAML::Value;
    EmitVector4f(p_out, m_rotation);
    p_out << YAML::Key << "scale" << YAML::Value;
    EmitVector3f(p_out, m_scale);
    return true;
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

bool TransformComponent::Undump(const YAML::Node& p_node, Archive& p_archive, uint32_t p_version) {
    CRASH_NOW();
    SetDirty();
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

bool ArmatureComponent::Dump(YAML::Emitter& p_out, Archive& p_archive, uint32_t p_version) const {
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

bool PerspectiveCameraComponent::Dump(YAML::Emitter& p_out, Archive& p_archive, uint32_t p_version) const {

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
#pragma endregion SCENE_COMPONENT_SERIALIZATION
WARNING_POP()

}  // namespace my
