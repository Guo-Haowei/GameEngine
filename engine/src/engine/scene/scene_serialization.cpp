#include "scene_serialization.h"

#include <yaml-cpp/yaml.h>

#include <fstream>

#include "engine/scene/scene.h"

namespace my {

// SCENE_VERSION history
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
inline constexpr uint32_t SCENE_VERSION = 15;
inline constexpr uint32_t SCENE_MAGIC = 'xScn';

// @TODO: refactor
static constexpr uint64_t has_next_flag = 6368519827137030510;

Result<void> SaveSceneText(const std::string& p_path, const Scene& p_scene) {
    unused(p_scene);

    YAML::Node node;
    node["name"] = "John Doe";
    node["age"] = 30;
    node["is_student"] = false;

    // Add a list (sequence) to the node
    YAML::Node hobbies = YAML::Node(YAML::NodeType::Sequence);
    hobbies.push_back("Reading");
    hobbies.push_back("Gaming");
    hobbies.push_back("Traveling");
    node["hobbies"] = hobbies;

    // Open a file to save the YAML data
    std::ofstream fout(p_path);

    // Check if the file is open
    if (fout.is_open()) {
        fout << node;
        fout.close();
        return Result<void>();
    }

    return HBN_ERROR(ErrorCode::ERR_FILE_CANT_WRITE);
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
    uint32_t magic = 0;
    uint32_t seed = ecs::Entity::MAX_ID;

    archive >> magic;
    if (magic != SCENE_MAGIC) {
        return HBN_ERROR(ErrorCode::ERR_FILE_CORRUPT, "magic is not 'xScn'");
    }

    archive >> version;
    if (version > SCENE_VERSION) {
        return HBN_ERROR(ErrorCode::ERR_FILE_CORRUPT, "file version {} is greater than max version {}", version, SCENE_VERSION);
    }

    archive >> seed;
    ecs::Entity::SetSeed(seed);

    archive >> p_scene.m_root;

    // dummy camera
    if (version < 12) {
        CRASH_NOW();
        PerspectiveCameraComponent old_camera;
        old_camera.Serialize(archive, version);

        auto camera_id = p_scene.CreatePerspectiveCameraEntity("editor_camera", old_camera.GetWidth(), old_camera.GetHeight());
        PerspectiveCameraComponent* new_camera = p_scene.GetComponent<PerspectiveCameraComponent>(camera_id);
        *new_camera = old_camera;
        new_camera->SetEditorCamera();
        p_scene.AttachChild(camera_id, p_scene.m_root);
    }

    for (;;) {
        uint64_t has_next = 0;
        archive >> has_next;
        if (has_next != has_next_flag) {
            return Result<void>();
        }

        std::string key;
        archive >> key;

        LOG_VERBOSE("Loading {}", key);

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
