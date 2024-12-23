#pragma once
#include "engine/math/aabb.h"
#include "engine/math/angle.h"
#include "engine/math/geomath.h"
#include "engine/systems/ecs/entity.h"

namespace YAML {
class Node;
class Emitter;
}  // namespace YAML

namespace my {

struct GpuMesh;
struct GpuStructuredBuffer;
struct ImageAsset;
struct TextAsset;
class Archive;
class FileAccess;
class Scene;
class ScriptableEntity;

struct ComponentFlagBase {
    enum : uint32_t {
        FLAG_NONE = BIT(0),
        FLAG_DIRTY = BIT(31),
    };

    uint32_t flags = FLAG_DIRTY;

    bool IsDirty() const { return flags & FLAG_DIRTY; }
    void SetDirty(bool p_dirty = true) { p_dirty ? flags |= FLAG_DIRTY : flags &= ~FLAG_DIRTY; }
    void OnDeserialized() { flags |= FLAG_DIRTY; }
};

#pragma region NAME_COMPONENT
class NameComponent {
public:
    NameComponent() = default;

    NameComponent(const char* p_name) { m_name = p_name; }

    void SetName(const char* p_name) { m_name = p_name; }
    void SetName(const std::string& p_name) { m_name = p_name; }

    const std::string& GetName() const { return m_name; }
    std::string& GetNameRef() { return m_name; }

    void Serialize(Archive& p_archive, uint32_t p_version);
    void OnDeserialized() {}

    static void RegisterClass();

private:
    std::string m_name;
};
#pragma endregion NAME_COMPONENT

#pragma region HIERARCHY_COMPONENT
class HierarchyComponent {
public:
    ecs::Entity GetParent() const { return m_parentId; }

    void Serialize(Archive& p_archive, uint32_t p_version);
    void OnDeserialized() {}

    static void RegisterClass();

private:
    ecs::Entity m_parentId;

    friend class Scene;
};
#pragma endregion HIERARCHY_COMPONENT

#pragma region TRANSFORM_COMPONENT
class TransformComponent : public ComponentFlagBase {
public:
    const Vector3f& GetTranslation() const { return m_translation; }
    void SetTranslation(const Vector3f& p_translation) { m_translation = p_translation; }
    void IncreaseTranslation(const Vector3f& p_delta) { m_translation += p_delta; }

    const Vector4f& GetRotation() const { return m_rotation; }
    void SetRotation(const Vector4f& p_rotation) { m_rotation = p_rotation; }

    const Vector3f& GetScale() const { return m_scale; }
    void SetScale(const Vector3f& p_scale) { m_scale = p_scale; }

    const Matrix4x4f& GetWorldMatrix() const { return m_worldMatrix; }

    void SetWorldMatrix(const Matrix4x4f& p_matrix) { m_worldMatrix = p_matrix; }

    Matrix4x4f GetLocalMatrix() const;

    void UpdateTransform();
    void Scale(const Vector3f& p_scale);
    void Translate(const Vector3f& p_translation);
    void Rotate(const Vector3f& p_euler);
    void RotateX(const Degree& p_degree) { Rotate(Vector3f(p_degree.GetRadians(), 0.0f, 0.0f)); }
    void RotateY(const Degree& p_degree) { Rotate(Vector3f(0.0f, p_degree.GetRadians(), 0.0f)); }
    void RotateZ(const Degree& p_degree) { Rotate(Vector3f(0.0f, 0.0f, p_degree.GetRadians())); }

    void SetLocalTransform(const Matrix4x4f& p_matrix);
    void MatrixTransform(const Matrix4x4f& p_matrix);

    void UpdateTransformParented(const TransformComponent& p_parent);

    void Serialize(Archive& p_archive, uint32_t p_version);
    static void RegisterClass();

private:
    Vector3f m_scale{ 1 };              // local scale
    Vector3f m_translation{ 0 };        // local translation
    Vector4f m_rotation{ 0, 0, 0, 1 };  // local rotation

    // Non-serialized attributes
    Matrix4x4f m_worldMatrix{ 1 };
};
#pragma endregion TRANSFORM_COMPONENT

#pragma region MESH_COMPONENT
enum class VertexAttributeName : uint8_t {
    POSITION = 0,
    NORMAL,
    TEXCOORD_0,
    TEXCOORD_1,
    TANGENT,
    JOINTS_0,
    WEIGHTS_0,
    COLOR_0,
    COUNT,
};

struct MeshComponent {
    enum : uint32_t {
        NONE = BIT(0),
        RENDERABLE = BIT(1),
        DOUBLE_SIDED = BIT(2),
        DYNAMIC = BIT(3),
    };

    struct VertexAttribute {
        VertexAttributeName attribName;
        uint32_t offsetInByte{ 0 };
        uint32_t strideInByte{ 0 };

        uint32_t elementCount{ 0 };
    };

    uint32_t flags = RENDERABLE;
    std::vector<uint32_t> indices;
    std::vector<Vector3f> positions;
    std::vector<Vector3f> normals;
    std::vector<Vector3f> tangents;
    std::vector<Vector2f> texcoords_0;
    std::vector<Vector2f> texcoords_1;
    std::vector<Vector4i> joints_0;
    std::vector<Vector4f> weights_0;
    std::vector<Vector4f> color_0;

    struct MeshSubset {
        ecs::Entity material_id;
        uint32_t index_offset = 0;
        uint32_t index_count = 0;
        math::AABB local_bound;

        static void RegisterClass();
    };
    std::vector<MeshSubset> subsets;

    ecs::Entity armatureId;

    // Non-serialized
    mutable std::shared_ptr<GpuMesh> gpuResource;
    math::AABB localBound;

    mutable std::vector<Vector3f> updatePositions;
    mutable std::vector<Vector3f> updateNormals;

    VertexAttribute attributes[std::to_underlying(VertexAttributeName::COUNT)];

    void CreateRenderData();

    void Serialize(Archive& p_archive, uint32_t p_version);
    void OnDeserialized();

    static void RegisterClass();
};
#pragma endregion MESH_COMPONENT

#pragma region MATERIAL_COMPONENT
struct MaterialComponent {
    enum {
        TEXTURE_BASE,
        TEXTURE_NORMAL,
        TEXTURE_METALLIC_ROUGHNESS,
        TEXTURE_MAX,
    };

    struct TextureMap {
        std::string path;
        bool enabled = true;

        static void RegisterClass();
    };

    float metallic = 0.0f;
    float roughness = 1.0f;
    float emissive = 0.0f;
    Vector4f baseColor = Vector4f(1);
    TextureMap textures[TEXTURE_MAX];

    void Serialize(Archive& p_archive, uint32_t p_version);
    void OnDeserialized();

    static void RegisterClass();
};
#pragma endregion MATERIAL_COMPONENT

#pragma region ANIMATION_COMPONENT
struct AnimationComponent {
    enum : uint32_t {
        NONE = 0,
        PLAYING = 1 << 0,
        LOOPED = 1 << 1,
    };

    struct Channel {
        enum Path {
            PATH_TRANSLATION,
            PATH_ROTATION,
            PATH_SCALE,

            PATH_UNKNOWN,
        };

        Path path = PATH_UNKNOWN;
        ecs::Entity targetId;
        int samplerIndex = -1;

        static void RegisterClass();
    };

    struct Sampler {
        std::vector<float> keyframeTimes;
        std::vector<float> keyframeData;

        static void RegisterClass();
    };

    bool IsPlaying() const { return flags & PLAYING; }
    bool IsLooped() const { return flags & LOOPED; }
    float GetLegnth() const { return end - start; }
    float IsEnd() const { return timer > end; }

    uint32_t flags = LOOPED;
    float start = 0;
    float end = 0;
    float timer = 0;
    float amount = 1;  // blend amount
    float speed = 1;

    std::vector<Channel> channels;
    std::vector<Sampler> samplers;

    void Serialize(Archive& p_archive, uint32_t p_version);
    void OnDeserialized() {}

    static void RegisterClass();
};
#pragma endregion ANIMATION_COMPONENT

#pragma region ARMATURE_COMPONENT
struct ArmatureComponent {
    std::vector<ecs::Entity> boneCollection;
    std::vector<Matrix4x4f> inverseBindMatrices;

    // Non-Serialized
    std::vector<Matrix4x4f> boneTransforms;

    void Serialize(Archive& p_archive, uint32_t p_version);
    void OnDeserialized() {}

    static void RegisterClass();
};
#pragma endregion ARMATURE_COMPONENT

#pragma region OBJECT_COMPONENT
struct ObjectComponent : public ComponentFlagBase {
    enum : uint32_t {
        FLAG_RENDERABLE = BIT(1),
        FLAG_CAST_SHADOW = BIT(2),
        FLAG_TRANSPARENT = BIT(3),
    };

    ecs::Entity meshId;

    ObjectComponent() {
        flags |= FLAG_RENDERABLE | FLAG_CAST_SHADOW;
    }

    void Serialize(Archive& p_archive, uint32_t p_version);
    void OnDeserialized() {}

    static void RegisterClass();
};
#pragma endregion OBJECT_COMPONENT

#pragma region CAMERA_COMPONENT
class PerspectiveCameraComponent : public ComponentFlagBase {
public:
    enum : uint32_t {
        EDITOR = BIT(2),
        PRIMARY = BIT(3),
    };

    static constexpr float DEFAULT_NEAR = 0.1f;
    static constexpr float DEFAULT_FAR = 1000.0f;
    static constexpr Degree DEFAULT_FOVY{ 50.0f };

    void Update();

    void SetDimension(int p_width, int p_height);

    bool IsEditorCamera() const { return flags & EDITOR; }
    void SetEditorCamera(bool p_flag = true) { p_flag ? flags |= EDITOR : flags &= ~EDITOR; }
    bool IsPrimary() const { return flags & PRIMARY; }
    void SetPrimary(bool p_flag = true) { p_flag ? flags |= PRIMARY : flags &= ~PRIMARY; }

    Degree GetFovy() const { return m_fovy; }
    void SetFovy(Degree p_degree) {
        m_fovy = p_degree;
        SetDirty();
    }

    float GetNear() const { return m_near; }
    void SetNear(float p_near) {
        m_near = p_near;
        SetDirty();
    }

    float GetFar() const { return m_far; }
    void SetFar(float p_far) {
        m_far = p_far;
        SetDirty();
    }

    const Vector3f& GetPosition() const { return m_position; }
    void SetPosition(const Vector3f& p_position) {
        m_position = p_position;
        SetDirty();
    }

    int GetWidth() const { return m_width; }
    int GetHeight() const { return m_height; }
    float GetAspect() const { return (float)m_width / m_height; }
    const Matrix4x4f& GetViewMatrix() const { return m_viewMatrix; }
    const Matrix4x4f& GetProjectionMatrix() const { return m_projectionMatrix; }
    const Matrix4x4f& GetProjectionViewMatrix() const { return m_projectionViewMatrix; }
    const Vector3f& GetRight() const { return m_right; }
    const Vector3f GetFront() const { return m_front; }

    void Serialize(Archive& p_archive, uint32_t p_version);

    static void RegisterClass();

private:
    Degree m_fovy{ DEFAULT_FOVY };
    float m_near{ DEFAULT_NEAR };
    float m_far{ DEFAULT_FAR };
    int m_width{ 0 };
    int m_height{ 0 };
    Degree m_pitch;  // x-axis
    Degree m_yaw;    // y-axis
    Vector3f m_position{ 0 };

    // Non-serlialized
    Vector3f m_front;
    Vector3f m_right;

    Matrix4x4f m_viewMatrix;
    Matrix4x4f m_projectionMatrix;
    Matrix4x4f m_projectionViewMatrix;

    friend class Scene;
    friend class EditorCameraController;
};
#pragma endregion CAMERA_COMPONENT

#pragma region LUA_SCRIPT_COMPONENT
class LuaScriptComponent {
public:
    LuaScriptComponent& SetClassName(std::string_view p_class_name);
    LuaScriptComponent& SetPath(std::string_view p_path);

    const std::string& GetPath() const { return m_path; }
    const std::string& GetClassName() const { return m_className; }
    int GetInstance() const { return m_instance; }

    std::string& GetPathRef() { return m_path; }
    std::string& GetClassNameRef() { return m_className; }

    void Serialize(Archive& p_archive, uint32_t p_version);
    void OnDeserialized();

    static void RegisterClass();

private:
    std::string m_className;
    std::string m_path;

    // Non-Serialized
    int m_instance{ 0 };

    friend class LuaScriptManager;
};
#pragma endregion LUA_SCRIPT_COMPONENT

#pragma region NATIVE_SCRIPT_COMPONENT
struct NativeScriptComponent {
    using InstantiateFunc = ScriptableEntity* (*)(void);
    using DestroyFunc = void (*)(NativeScriptComponent*);

    std::string scriptName = typeid(this).name();
    ScriptableEntity* instance{ nullptr };
    InstantiateFunc instantiateFunc{ nullptr };
    DestroyFunc destroyFunc{ nullptr };

    NativeScriptComponent() = default;

    ~NativeScriptComponent();

    NativeScriptComponent(const NativeScriptComponent& p_rhs);

    NativeScriptComponent& operator=(const NativeScriptComponent& p_rhs);

    template<typename T>
    NativeScriptComponent& Bind() {
        instantiateFunc = []() -> ScriptableEntity* {
            return new T();
        };
        destroyFunc = [](NativeScriptComponent* p_script) {
            delete (T*)p_script->instance;
            p_script->instance = nullptr;
        };

        return *this;
    }

    void Serialize(Archive& p_archive, uint32_t p_version);
    void OnDeserialized() {}

    static void RegisterClass();
};
#pragma endregion NATIVE_SCRIPT_COMPONENT

#pragma region COLLISION_OBJECT_COMPONENT

/*
    enum Type : uint32_t {
        PLAYER = BIT(1),
        FRIENDLY = BIT(2),
        HOSTILE = BIT(4),
    };

    player.collisionType = PLAYER;
    player.collisionMask = HOSTILE;

    hostile.collisionType = HOSTILE;
    hostile.collisionMask = PLAYER | FRIENDLY;
*/
struct CollisionObjectBase {
    uint32_t collisionType = 0;
    uint32_t collisionMask = 0;

    // Non-Serialized
    void* physicsObject{ nullptr };

    void Serialize(Archive& p_archive, uint32_t p_version);
    void OnDeserialized() {}
};

struct RigidBodyComponent : CollisionObjectBase {
    enum CollisionShape : uint8_t {
        SHAPE_UNKNOWN,
        SHAPE_SPHERE,
        SHAPE_CUBE,
        SHAPE_MAX,
    };

    enum ObjectType : uint8_t {
        DYNAMIC,
        GHOST,
    };

    union Parameter {
        struct {
            Vector3f half_size;
        } box;
        struct {
            float radius;
        } sphere;
    };

    CollisionShape shape{ SHAPE_UNKNOWN };
    ObjectType objectType{ DYNAMIC };
    float mass{ 1.0f };
    Parameter param;

    RigidBodyComponent& InitCube(const Vector3f& p_half_size);

    RigidBodyComponent& InitSphere(float p_radius);

    RigidBodyComponent& InitGhost();

    void Serialize(Archive& p_archive, uint32_t p_version);
    void OnDeserialized() {}

    static void RegisterClass();
};

enum ClothFixFlag : uint32_t {
    CLOTH_FIX_0 = BIT(1),
    CLOTH_FIX_1 = BIT(2),
    CLOTH_FIX_2 = BIT(3),
    CLOTH_FIX_3 = BIT(4),

    CLOTH_FIX_ALL = CLOTH_FIX_0 | CLOTH_FIX_1 | CLOTH_FIX_2 | CLOTH_FIX_3,
};
DEFINE_ENUM_BITWISE_OPERATIONS(ClothFixFlag);

struct ClothComponent : CollisionObjectBase {
    Vector3f point_0;
    Vector3f point_1;
    Vector3f point_2;
    Vector3f point_3;
    Vector2i res;
    ClothFixFlag fixedFlags;

    // Non-Serialized
    void* physicsObject{ nullptr };

    void Serialize(Archive& p_archive, uint32_t p_version);
    void OnDeserialized() {}

    static void RegisterClass();
};
#pragma endregion COLLISION_OBJECT_COMPONENT

#pragma region ENVIRONMENT_COMPONENT
struct EnvironmentComponent {
    enum Type : uint32_t {
        HDR_TEXTURE,
        PROCEDURE,
    };

    struct Sky {
        Type type;
        std::string texturePath;
        // Non-Serialized
        mutable const ImageAsset* textureAsset;

        static void RegisterClass();
    } sky;

    struct Ambient {
        Vector4f color;
        static void RegisterClass();
    } ambient;

    void Serialize(Archive& p_archive, uint32_t p_version);
    void OnDeserialized() {}

    static void RegisterClass();
};

struct VoxelGiComponent {
    enum : uint32_t {
        ENABLED = BIT(1),
        SHOW_DEBUG_BOX = BIT(2),
    };

    uint32_t flags = 0;
    // Non-serialized
    math::AABB region;

    bool Enabled() const { return flags & ENABLED; }
    bool ShowDebugBox() const { return flags & SHOW_DEBUG_BOX; }

    void Serialize(Archive& p_archive, uint32_t p_version);
    void OnDeserialized() {}

    static void RegisterClass();
};
#pragma endregion ENVIRONMENT_COMPONENT

#pragma region PARTICLE_EMITTER_COMPONENT
struct ParticleEmitterComponent {
    bool gravity{ false };  // @TODO: force instead of gravity
    int maxParticleCount{ 1000 };
    int particlesPerFrame{ 10 };
    float particleScale{ 1.0f };
    float particleLifeSpan{ 3.0f };
    Vector3f startingVelocity{ 0.0f };
    Vector4f color{ Vector4f::One };
    std::string texture;

    // Non-Serialized
    std::shared_ptr<GpuStructuredBuffer> particleBuffer{ nullptr };
    std::shared_ptr<GpuStructuredBuffer> counterBuffer{ nullptr };
    std::shared_ptr<GpuStructuredBuffer> deadBuffer{ nullptr };
    std::shared_ptr<GpuStructuredBuffer> aliveBuffer[2]{ nullptr, nullptr };

    uint32_t aliveBufferIndex{ 0 };

    uint32_t GetPreIndex() const { return aliveBufferIndex; }
    uint32_t GetPostIndex() const { return 1 - aliveBufferIndex; }

    void Serialize(Archive& p_archive, uint32_t p_version);
    void OnDeserialized() {}
    static void RegisterClass();
};
#pragma endregion PARTICLE_EMITTER_COMPONENT

#pragma region FORCE_FIELD_COMPONENT
struct ForceFieldComponent {
    float strength{ 1.0f };
    float radius{ 0.01f };

    void Serialize(Archive& p_archive, uint32_t p_version);
    void OnDeserialized() {}

    static void RegisterClass();
};
#pragma endregion FORCE_FIELD_COMPONENT

// #pragma region _COMPONENT
// #pragma endregion _COMPONENT

}  // namespace my
