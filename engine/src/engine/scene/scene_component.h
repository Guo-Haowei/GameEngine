#pragma once
#include "engine/core/math/angle.h"
#include "engine/core/math/geomath.h"
#include "engine/core/systems/entity.h"

namespace my {

struct ImageAsset;
struct TextAsset;
class Archive;
class ScriptableEntity;

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

private:
    std::string m_name;
};
#pragma endregion NAME_COMPONENT

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
    };
    struct Sampler {
        std::vector<float> keyframeTmes;
        std::vector<float> keyframeData;
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
};
#pragma endregion ANIMATION_COMPONENT

#pragma region ARMATURE_COMPONENT
struct ArmatureComponent {
    enum FLAGS {
        NONE = 0,
    };
    uint32_t flags = NONE;

    std::vector<ecs::Entity> boneCollection;
    std::vector<Matrix4x4f> inverseBindMatrices;

    // Non-Serialized
    std::vector<Matrix4x4f> boneTransforms;

    void Serialize(Archive& p_archive, uint32_t p_version);
};
#pragma endregion ARMATURE_COMPONENT

#pragma region OBJECT_COMPONENT
struct ObjectComponent {
    enum Flags : uint32_t {
        NONE = BIT(0),
        RENDERABLE = BIT(1),
        CAST_SHADOW = BIT(2),
        IS_TRANSPARENT = BIT(3),
    };

    Flags flags = static_cast<Flags>(RENDERABLE | CAST_SHADOW);
    ecs::Entity meshId;

    void Serialize(Archive& p_archive, uint32_t p_version);
};
DEFINE_ENUM_BITWISE_OPERATIONS(ObjectComponent::Flags);
#pragma endregion OBJECT_COMPONENT

#pragma region CAMERA_COMPONENT
class PerspectiveCameraComponent {
public:
    enum : uint32_t {
        NONE = BIT(0),
        DIRTY = BIT(1),
        EDITOR = BIT(2),
        PRIMARY = BIT(3),
    };

    static constexpr float DEFAULT_NEAR = 0.1f;
    static constexpr float DEFAULT_FAR = 1000.0f;
    static constexpr Degree DEFAULT_FOVY{ 50.0f };

    void Update();

    void SetDimension(int p_width, int p_height);

    bool IsDirty() const { return m_flags & DIRTY; }
    void SetDirty(bool p_dirty = true) { p_dirty ? m_flags |= DIRTY : m_flags &= ~DIRTY; }
    bool IsEditorCamera() const { return m_flags & EDITOR; }
    void SetEditorCamera(bool p_flag = true) { p_flag ? m_flags |= EDITOR : m_flags &= ~EDITOR; }
    bool IsPrimary() const { return m_flags & PRIMARY; }
    void SetPrimary(bool p_flag = true) { p_flag ? m_flags |= PRIMARY : m_flags &= ~PRIMARY; }

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

private:
    uint32_t m_flags = DIRTY;

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
    void SetScript(const std::string& p_path);

    std::string& GetScriptRef();

    const char* GetSource() const;

    void SetAsset(const TextAsset* p_asset) {
        m_asset = p_asset;
    }

    void Serialize(Archive& p_archive, uint32_t p_version);

private:
    std::string m_path;

    // Non-Serialized
    const TextAsset* m_asset{ nullptr };
};
#pragma endregion LUA_SCRIPT_COMPONENT

#pragma region NATIVE_SCRIPT_COMPONENT
struct NativeScriptComponent {
    using InstantiateFunc = ScriptableEntity* (*)(void);
    using DestroyFunc = void (*)(NativeScriptComponent*);

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
};
#pragma endregion COLLISION_OBJECT_COMPONENT

#pragma region ENVIRONMENT_COMPONENT
struct EnvironmentComponent {
    enum Type {
        HDR_TEXTURE,
        PROCEDURE,
    };

    struct Sky {
        Type type;
        std::string texturePath;
        // Non-Serialized
        mutable const ImageAsset* textureAsset;
    } sky;

    struct Ambient {
        Vector4f color;
    } ambient;

    void Serialize(Archive& p_archive, uint32_t p_version);
};
#pragma endregion ENVIRONMENT_COMPONENT

// #pragma region _COMPONENT
// #pragma endregion _COMPONENT

}  // namespace my
