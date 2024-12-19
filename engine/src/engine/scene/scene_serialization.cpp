#include "scene_serialization.h"

#include <fstream>

#include "engine/core/framework/asset_registry.h"
#include "engine/core/io/archive.h"
#include "engine/core/io/file_access.h"
#include "engine/core/string/string_utils.h"
#include "engine/scene/scene.h"
#include "engine/systems/serialization/serialization.h"

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
static constexpr char SCENE_MAGIC[] = "xBScene";
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

    char magic[sizeof(SCENE_MAGIC)]{ 0 };
    if (!archive.Read(magic) || !StringUtils::StringEqual(magic, SCENE_MAGIC)) {
        return HBN_ERROR(ErrorCode::ERR_FILE_CORRUPT, "file corrupted, magic is not '{}'", SCENE_MAGIC);
    }

    uint32_t version;
    if (!archive.Read(version) || version > SCENE_VERSION) {
        return HBN_ERROR(ErrorCode::ERR_FILE_CORRUPT, "incorrect scene version {}, current version is {}", version, SCENE_VERSION);
    }

    SCENE_DBG_LOG("loading scene '{}', version: {}", p_path, version);

    uint32_t seed = ecs::Entity::MAX_ID;
    if (!archive.Read(seed)) {
        return HBN_ERROR(ErrorCode::ERR_FILE_CORRUPT, "failed to read seed");
    }

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
[[nodiscard]] Result<void> SerializeComponent(YAML::Emitter& p_out,
                                              const char* p_name,
                                              ecs::Entity p_entity,
                                              const Scene& p_scene,
                                              FileAccess* p_binary) {

    const T* component = p_scene.GetComponent<T>(p_entity);
    if (component) {
        p_out << DUMP_KEY(p_name);
        serialize::SerializeYamlContext context;
        context.file = p_binary;
        if (auto res = serialize::SerializeYaml(p_out, *component, context); !res) {
            return HBN_ERROR(res.error());
        }
    }
    return Result<void>();
}

void RegisterClasses() {
    static bool s_initialized = false;
    if (DEV_VERIFY(!s_initialized)) {
#define REGISTER_COMPONENT(a, ...) a::RegisterClass();
        REGISTER_COMPONENT_LIST
#undef REGISTER_COMPONENT
        MeshComponent::MeshSubset::RegisterClass();
        LightComponent::Attenuation::RegisterClass();
        EnvironmentComponent::Ambient::RegisterClass();
        EnvironmentComponent::Sky::RegisterClass();
        s_initialized = true;
    }
}

Result<void> SaveSceneText(const std::string& p_path, const Scene& p_scene) {
    std::unordered_set<uint32_t> entity_set;

    for (const auto& it : p_scene.GetLibraryEntries()) {
        auto& manager = it.second.m_manager;
        for (auto entity : manager->GetEntityArray()) {
            entity_set.insert(entity.GetId());
        }
    }

    std::vector<uint32_t> entity_array(entity_set.begin(), entity_set.end());
    std::sort(entity_array.begin(), entity_array.end());

    auto binary_path = std::format("{}{}", p_path, ".bin");
    Archive archive;
    if (auto res = archive.OpenWrite(binary_path); !res) {
        return HBN_ERROR(res.error());
    }

    YAML::Emitter out;
    out << YAML::BeginMap;
    out << DUMP_KEY("version") << SCENE_VERSION;
    out << DUMP_KEY("seed") << ecs::Entity::GetSeed();
    out << DUMP_KEY("root") << p_scene.m_root.GetId();
    out << DUMP_KEY("binary") << binary_path;

    out << DUMP_KEY("entities");
    out << YAML::BeginSeq;

    bool ok = true;
    ok = ok && archive.Write(SCENE_MAGIC);
    ok = ok && archive.Write(SCENE_VERSION);
    ok = ok && archive.Write(SCENE_GUARD_MESSAGE);
    if (!ok) {
        return HBN_ERROR(ErrorCode::ERR_FILE_CANT_WRITE, "failed to save file '{}'", p_path);
    }

    for (auto id : entity_array) {
        ecs::Entity entity{ id };

        out << YAML::BeginMap;
        out << DUMP_KEY("id") << id;

        out.SetSeqFormat(YAML::Flow);

#define REGISTER_COMPONENT(a, ...)                                                                         \
    if (auto res = SerializeComponent<a>(out, #a, entity, p_scene, archive.GetFileAccess().get()); !res) { \
        return HBN_ERROR(res.error());                                                                     \
    }
        REGISTER_COMPONENT_LIST
#undef REGISTER_COMPONENT

        out.SetSeqFormat(YAML::Block);

        out << YAML::EndMap;
    }

    out << YAML::EndSeq;
    out << YAML::EndMap;

    auto res = FileAccess::Open(p_path, FileAccess::WRITE);
    if (!res) {
        return HBN_ERROR(res.error());
    }

    auto yaml_file = *res;
    const char* result = out.c_str();
    yaml_file->WriteBuffer(result, strlen(result));
    return Result<void>();
}

template<Serializable T>
static Result<void> DeserializeComponent(const YAML::Node& p_node,
                                         const char* p_key,
                                         ecs::Entity p_id,
                                         uint32_t p_version,
                                         Scene& p_scene,
                                         FileAccess* p_binary) {
    const auto& node = p_node[p_key];
    if (!node.IsDefined()) {
        return Result<void>();
    }

    if (!node.IsMap()) {
        return HBN_ERROR(ErrorCode::ERR_PARSE_ERROR, "entity {} has invalid '{}'", p_id.GetId(), p_key);
    }

    // @TODO: reserve component manager
    T& component = p_scene.Create<T>(p_id);
    serialize::SerializeYamlContext context;
    context.file = p_binary;
    context.version = p_version;
    if (auto res = serialize::DeserializeYaml(node, component, context); !res) {
        return HBN_ERROR(res.error());
    }

    component.OnDeserialized();
    return Result<void>();
}

Result<void> LoadSceneText(const std::string& p_path, Scene& p_scene) {
    unused(p_scene);

    auto res = FileAccess::Open(p_path, FileAccess::READ);
    if (!res) {
        return HBN_ERROR(res.error());
    }

    auto file = *res;
    const size_t size = file->GetLength();

    std::string buffer;
    buffer.resize(size);
    if (auto read = file->ReadBuffer(buffer.data(), size); read != size) {
        return HBN_ERROR(ErrorCode::ERR_FILE_CANT_READ, "failed to read {} bytes from '{}'", size, p_path);
    }

    file->Close();
    const auto node = YAML::Load(buffer);

    YAML::Emitter out;
    auto version = node["version"].as<uint32_t>();
    if (version > SCENE_VERSION) {
        return HBN_ERROR(ErrorCode::ERR_FILE_CORRUPT, "incorrect version {}", version);
    }
    auto seed = node["seed"].as<uint32_t>();
    ecs::Entity::SetSeed(seed);

    ecs::Entity root(node["root"].as<uint32_t>());
    p_scene.m_root = root;

    auto binary_file = node["binary"].as<std::string>();
    res = FileAccess::Open(binary_file, FileAccess::READ);
    if (!res) {
        return HBN_ERROR(res.error());
    }

    file = *res;
    const auto& entities = node["entities"];
    if (!entities.IsSequence()) {
        return HBN_ERROR(ErrorCode::ERR_FILE_CORRUPT, "invalid format");
    }

    for (const auto& entity : entities) {
        if (!entity.IsMap()) {
            return HBN_ERROR(ErrorCode::ERR_FILE_CORRUPT, "invalid format");
        }

        ecs::Entity id(entity["id"].as<uint32_t>());

#define REGISTER_COMPONENT(a, ...)                                                         \
    do {                                                                                   \
        auto res2 = DeserializeComponent<a>(entity, #a, id, version, p_scene, file.get()); \
        if (!res2) { return HBN_ERROR(res2.error()); }                                     \
    } while (0);
        REGISTER_COMPONENT_LIST
#undef REGISTER_COMPONENT
    }

    return Result<void>();
}

#pragma region SCENE_COMPONENT_SERIALIZATION
using serialize::FieldFlag;

void NameComponent::Serialize(Archive& p_archive, uint32_t p_version) {
    unused(p_version);

    p_archive.ArchiveValue(m_name);
}

void NameComponent::RegisterClass() {
    REGISTER_FIELD(NameComponent, "name", m_name, FieldFlag::NONE);
}

void HierarchyComponent::Serialize(Archive& p_archive, uint32_t) {
    p_archive.ArchiveValue(m_parentId);
}

void HierarchyComponent::RegisterClass() {
    REGISTER_FIELD(HierarchyComponent, "parent_id", m_parentId, FieldFlag::NONE);
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

void AnimationComponent::RegisterClass() {
}

void TransformComponent::Serialize(Archive& p_archive, uint32_t) {
    p_archive.ArchiveValue(flags);
    p_archive.ArchiveValue(m_scale);
    p_archive.ArchiveValue(m_translation);
    p_archive.ArchiveValue(m_rotation);
}

void TransformComponent::RegisterClass() {
    REGISTER_FIELD(TransformComponent, "flags", flags, FieldFlag::NONE);
    REGISTER_FIELD(TransformComponent, "translation", m_translation, FieldFlag::NONE);
    REGISTER_FIELD(TransformComponent, "rotation", m_rotation, FieldFlag::NONE);
    REGISTER_FIELD(TransformComponent, "scale", m_scale, FieldFlag::NONE);
}

void MeshComponent::Serialize(Archive& p_archive, uint32_t) {
    p_archive.ArchiveValue(flags);
    p_archive.ArchiveValue(indices);
    p_archive.ArchiveValue(positions);
    p_archive.ArchiveValue(normals);
    p_archive.ArchiveValue(tangents);
    p_archive.ArchiveValue(texcoords_0);
    p_archive.ArchiveValue(texcoords_1);
    p_archive.ArchiveValue(joints_0);
    p_archive.ArchiveValue(weights_0);
    p_archive.ArchiveValue(color_0);
    p_archive.ArchiveValue(subsets);
    p_archive.ArchiveValue(armatureId);
}

void MeshComponent::OnDeserialized() {
    CreateRenderData();
}

void MeshComponent::MeshSubset::RegisterClass() {
    using serialize::FieldFlag;

    REGISTER_FIELD_2(MeshComponent::MeshSubset, material_id, FieldFlag::NONE);
    REGISTER_FIELD_2(MeshComponent::MeshSubset, index_count, FieldFlag::NONE);
    REGISTER_FIELD_2(MeshComponent::MeshSubset, index_offset, FieldFlag::NONE);
    REGISTER_FIELD_2(MeshComponent::MeshSubset, local_bound, FieldFlag::NONE);
}

void MeshComponent::RegisterClass() {
    using serialize::FieldFlag;

    REGISTER_FIELD_2(MeshComponent, flags, FieldFlag::NONE);
    REGISTER_FIELD_2(MeshComponent, subsets, FieldFlag::NONE);
    REGISTER_FIELD(MeshComponent, "armature_id", armatureId, FieldFlag::NONE);

    REGISTER_FIELD_2(MeshComponent, indices, FieldFlag::BINARY);
    REGISTER_FIELD_2(MeshComponent, positions, FieldFlag::BINARY);
    REGISTER_FIELD_2(MeshComponent, normals, FieldFlag::BINARY);
    REGISTER_FIELD_2(MeshComponent, tangents, FieldFlag::BINARY);
    REGISTER_FIELD_2(MeshComponent, texcoords_0, FieldFlag::BINARY);
    REGISTER_FIELD_2(MeshComponent, texcoords_1, FieldFlag::BINARY);
    REGISTER_FIELD_2(MeshComponent, joints_0, FieldFlag::BINARY);
    REGISTER_FIELD_2(MeshComponent, weights_0, FieldFlag::BINARY);
    REGISTER_FIELD_2(MeshComponent, color_0, FieldFlag::BINARY);
}

void MaterialComponent::Serialize(Archive& p_archive, uint32_t p_version) {
    unused(p_version);

    p_archive.ArchiveValue(metallic);
    p_archive.ArchiveValue(roughness);
    p_archive.ArchiveValue(emissive);
    p_archive.ArchiveValue(baseColor);

    // @TODO: refactor this
    if (p_archive.IsWriteMode()) {
        for (int i = 0; i < TEXTURE_MAX; ++i) {
            p_archive << textures[i].enabled;
            p_archive << textures[i].path;
        }
    } else {
        for (int i = 0; i < TEXTURE_MAX; ++i) {
            p_archive >> textures[i].enabled;
            std::string& path = textures[i].path;
            p_archive >> path;
        }
    }
}

void MaterialComponent::OnDeserialized() {
    for (int i = 0; i < TEXTURE_MAX; ++i) {
        const auto& path = textures[i].path;
        if (!path.empty()) {
            AssetRegistry::GetSingleton().RequestAssetAsync(path);
        }
    }
}

void MaterialComponent::RegisterClass() {
    using serialize::FieldFlag;
    REGISTER_FIELD(MaterialComponent, "base_color", baseColor, FieldFlag::NONE);
    REGISTER_FIELD_2(MaterialComponent, roughness, FieldFlag::NONE);
    REGISTER_FIELD_2(MaterialComponent, metallic, FieldFlag::NONE);
    REGISTER_FIELD_2(MaterialComponent, emissive, FieldFlag::NONE);
}

void LightComponent::Serialize(Archive& p_archive, uint32_t p_version) {
    DEV_ASSERT(p_version > 14);

    p_archive.ArchiveValue(m_flags);
    p_archive.ArchiveValue(m_type);
    p_archive.ArchiveValue(m_atten);
    p_archive.ArchiveValue(m_shadowRegion);
}

void LightComponent::OnDeserialized() {
    // @TODO: use common base
    m_flags |= DIRTY;
}

void LightComponent::Attenuation::RegisterClass() {
    using serialize::FieldFlag;
    REGISTER_FIELD(LightComponent::Attenuation, "constant", constant, FieldFlag::NONE);
    REGISTER_FIELD(LightComponent::Attenuation, "linear", linear, FieldFlag::NONE);
    REGISTER_FIELD(LightComponent::Attenuation, "quadratic", quadratic, FieldFlag::NONE);
}

void LightComponent::RegisterClass() {
    using serialize::FieldFlag;
    REGISTER_FIELD(LightComponent, "flags", m_flags, FieldFlag::NONE);
    REGISTER_FIELD(LightComponent, "type", m_type, FieldFlag::NONE);
    REGISTER_FIELD(LightComponent, "shadow_region", m_shadowRegion, FieldFlag::NUALLABLE);
    REGISTER_FIELD(LightComponent, "attenuation", m_atten, FieldFlag::NUALLABLE);
}

void ArmatureComponent::Serialize(Archive& p_archive, uint32_t) {
    p_archive.ArchiveValue(flags);
    p_archive.ArchiveValue(boneCollection);
    p_archive.ArchiveValue(inverseBindMatrices);
}

void ArmatureComponent::RegisterClass() {
    using serialize::FieldFlag;
}

void ObjectComponent::Serialize(Archive& p_archive, uint32_t) {
    p_archive.ArchiveValue(flags);
    p_archive.ArchiveValue(meshId);
}

void ObjectComponent::RegisterClass() {
    using serialize::FieldFlag;

    REGISTER_FIELD(ObjectComponent, "flags", flags, FieldFlag::NONE);
    REGISTER_FIELD(ObjectComponent, "mesh_id", meshId, FieldFlag::NONE);
}

void PerspectiveCameraComponent::Serialize(Archive& p_archive, uint32_t) {
    p_archive.ArchiveValue(flags);
    p_archive.ArchiveValue(m_near);
    p_archive.ArchiveValue(m_far);
    p_archive.ArchiveValue(m_fovy);
    p_archive.ArchiveValue(m_width);
    p_archive.ArchiveValue(m_height);
    p_archive.ArchiveValue(m_pitch);
    p_archive.ArchiveValue(m_yaw);
    p_archive.ArchiveValue(m_position);
}

void PerspectiveCameraComponent::RegisterClass() {
    using serialize::FieldFlag;

    REGISTER_FIELD(PerspectiveCameraComponent, "flags", flags, FieldFlag::NONE);
    REGISTER_FIELD(PerspectiveCameraComponent, "fovy", m_fovy, FieldFlag::NONE);
    REGISTER_FIELD(PerspectiveCameraComponent, "near", m_near, FieldFlag::NONE);
    REGISTER_FIELD(PerspectiveCameraComponent, "far", m_far, FieldFlag::NONE);
    REGISTER_FIELD(PerspectiveCameraComponent, "width", m_width, FieldFlag::NONE);
    REGISTER_FIELD(PerspectiveCameraComponent, "height", m_height, FieldFlag::NONE);
    REGISTER_FIELD(PerspectiveCameraComponent, "pitch", m_pitch, FieldFlag::NONE);
    REGISTER_FIELD(PerspectiveCameraComponent, "yaw", m_yaw, FieldFlag::NONE);
    REGISTER_FIELD(PerspectiveCameraComponent, "position", m_position, FieldFlag::NONE);
}

void LuaScriptComponent::Serialize(Archive& p_archive, uint32_t) {
    p_archive.ArchiveValue(m_path);
}

void LuaScriptComponent::OnDeserialized() {
    SetScript(m_path);
}

void LuaScriptComponent::RegisterClass() {
    using serialize::FieldFlag;

    REGISTER_FIELD(LuaScriptComponent, "path", m_path, FieldFlag::NONE);
}

void NativeScriptComponent::Serialize(Archive& p_archive, uint32_t) {
    p_archive.ArchiveValue(scriptName);
}

void NativeScriptComponent::RegisterClass() {
    using serialize::FieldFlag;

    REGISTER_FIELD(NativeScriptComponent, "script_name", scriptName, FieldFlag::NONE);
}

void CollisionObjectBase::Serialize(Archive& p_archive, uint32_t) {
    p_archive.ArchiveValue(collisionType);
    p_archive.ArchiveValue(collisionMask);
}

void RigidBodyComponent::Serialize(Archive& p_archive, uint32_t p_version) {
    CollisionObjectBase::Serialize(p_archive, p_version);

    p_archive.ArchiveValue(shape);
    p_archive.ArchiveValue(objectType);
    p_archive.ArchiveValue(param);
    p_archive.ArchiveValue(mass);
}

void RigidBodyComponent::RegisterClass() {
    using serialize::FieldFlag;

    // @TODO: fix this part
    // REGISTER_FIELD(RigidBodyComponent, "shape", shape, FieldFlag::NONE);
    // REGISTER_FIELD(RigidBodyComponent, "type", objectType, FieldFlag::NONE);
    // REGISTER_FIELD(RigidBodyComponent, "param", param, FieldFlag::NONE);
    REGISTER_FIELD(RigidBodyComponent, "mass", mass, FieldFlag::NONE);
}

// @TODO: refactor these components
void ParticleEmitterComponent::RegisterClass() {
    using serialize::FieldFlag;

    REGISTER_FIELD(ParticleEmitterComponent, "scale", particleScale, FieldFlag::NONE);
    REGISTER_FIELD(ParticleEmitterComponent, "lifespan", particleLifeSpan, FieldFlag::NONE);
}

void ForceFieldComponent::Serialize(Archive& p_archive, uint32_t) {
    p_archive.ArchiveValue(strength);
    p_archive.ArchiveValue(radius);
}

void ForceFieldComponent::RegisterClass() {
    using serialize::FieldFlag;

    REGISTER_FIELD_2(ForceFieldComponent, strength, FieldFlag::NONE);
    REGISTER_FIELD_2(ForceFieldComponent, radius, FieldFlag::NONE);
}

void BoxColliderComponent::Serialize(Archive& p_archive, uint32_t) {
    p_archive.ArchiveValue(box);
}

void BoxColliderComponent::RegisterClass() {
    using serialize::FieldFlag;

    REGISTER_FIELD_2(BoxColliderComponent, box, FieldFlag::NONE);
}

void MeshColliderComponent::Serialize(Archive& p_archive, uint32_t) {
    p_archive.ArchiveValue(objectId);
}

void MeshColliderComponent::RegisterClass() {
    using serialize::FieldFlag;

    REGISTER_FIELD(MeshColliderComponent, "object_id", objectId, FieldFlag::NONE);
}

void ClothComponent::Serialize(Archive& p_archive, uint32_t p_version) {
    CollisionObjectBase::Serialize(p_archive, p_version);

    CRASH_NOW_MSG("@TODO: implement");
}

void ClothComponent::RegisterClass() {
    using serialize::FieldFlag;

    REGISTER_FIELD_2(ClothComponent, point_0, FieldFlag::NONE);
    REGISTER_FIELD_2(ClothComponent, point_1, FieldFlag::NONE);
    REGISTER_FIELD_2(ClothComponent, point_2, FieldFlag::NONE);
    REGISTER_FIELD_2(ClothComponent, point_3, FieldFlag::NONE);
}

void EnvironmentComponent::Serialize(Archive& p_archive, uint32_t) {
    unused(p_archive);
    CRASH_NOW();
}
void EnvironmentComponent::Sky::RegisterClass() {
    REGISTER_FIELD(EnvironmentComponent::Sky, "type", type, FieldFlag::NONE);
    REGISTER_FIELD(EnvironmentComponent::Sky, "texture", texturePath, FieldFlag::NONE);
}

void EnvironmentComponent::Ambient::RegisterClass() {
    REGISTER_FIELD_2(EnvironmentComponent::Ambient, color, FieldFlag::NONE);
}

void EnvironmentComponent::RegisterClass() {
    REGISTER_FIELD_2(EnvironmentComponent, sky, FieldFlag::NONE);
    REGISTER_FIELD_2(EnvironmentComponent, ambient, FieldFlag::NONE);
}
#pragma endregion SCENE_COMPONENT_SERIALIZATION

}  // namespace my
