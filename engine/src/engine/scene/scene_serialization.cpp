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

// @TODO: refactor
static constexpr uint64_t has_next_flag = 6368519827137030510;

Result<void> SaveSceneText(const std::string& p_path, const Scene& p_scene) {
    unused(p_scene);

    YAML::Node scene;
    scene["version"] = SCENE_VERSION;
    scene["seed"] = ecs::Entity::GetSeed();
    scene["root"] = p_scene.m_root.GetId();

    Archive archive;
    for (const auto& it : p_scene.GetLibraryEntries()) {
        YAML::Node component = YAML::Node(YAML::NodeType::Sequence);
        scene[it.first] = component;
        it.second.m_manager->Dump(component, archive, SCENE_VERSION);
    }

    std::ofstream fout(p_path);
    if (!fout.is_open()) {
        return HBN_ERROR(ErrorCode::ERR_FILE_CANT_WRITE);
    }

    fout << scene;
    fout.close();
    return Result<void>();
}

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
        archive << has_next_flag;
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
        if (has_next != has_next_flag) {
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

}  // namespace my
