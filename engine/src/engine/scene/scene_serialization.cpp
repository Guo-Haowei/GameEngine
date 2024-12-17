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
    unused(p_scene);

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

}  // namespace my
