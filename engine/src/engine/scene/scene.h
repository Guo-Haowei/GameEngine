#pragma once
#include "engine/assets/asset.h"
#include "engine/core/base/noncopyable.h"
#include "engine/math/ray.h"
#include "engine/scene/scene_component.h"
#include "engine/systems/ecs/component_manager.h"
#include "engine/systems/ecs/view.h"

struct lua_State;

namespace my::jobsystem {
class Context;
}

namespace my {

#define REGISTER_COMPONENT_LIST                                                            \
    REGISTER_COMPONENT(NameComponent, "World::NameComponent", 0)                           \
    REGISTER_COMPONENT(TransformComponent, "World::TransformComponent", 0)                 \
    REGISTER_COMPONENT(HierarchyComponent, "World::HierarchyComponent", 0)                 \
    REGISTER_COMPONENT(MaterialComponent, "World::MaterialComponent", 0)                   \
    REGISTER_COMPONENT(MeshComponent, "World::MeshComponent", 0)                           \
    REGISTER_COMPONENT(ObjectComponent, "World::ObjectComponent", 0)                       \
    REGISTER_COMPONENT(LightComponent, "World::LightComponent", 0)                         \
    REGISTER_COMPONENT(ArmatureComponent, "World::ArmatureComponent", 0)                   \
    REGISTER_COMPONENT(AnimationComponent, "World::AnimationComponent", 0)                 \
    REGISTER_COMPONENT(ParticleEmitterComponent, "World::ParticleEmitterComponent", 0)     \
    REGISTER_COMPONENT(MeshEmitterComponent, "World::MeshEmitterComponent", 0)             \
    REGISTER_COMPONENT(ForceFieldComponent, "World::ForceFieldComponent", 0)               \
    REGISTER_COMPONENT(LuaScriptComponent, "World::LuaScriptComponent", 0)                 \
    REGISTER_COMPONENT(NativeScriptComponent, "World::NativeScriptComponent", 0)           \
    REGISTER_COMPONENT(PerspectiveCameraComponent, "World::PerspectiveCameraComponent", 0) \
    REGISTER_COMPONENT(RigidBodyComponent, "World::RigidBodyComponent", 0)                 \
    REGISTER_COMPONENT(ClothComponent, "World::ClothComponent", 0)                         \
    REGISTER_COMPONENT(VoxelGiComponent, "World::VoxelGiComponent", 0)                     \
    REGISTER_COMPONENT(EnvironmentComponent, "World::EnvironmentComponent", 0)

// @TODO: refactor
struct PhysicsWorldContext;

enum class PhysicsMode : uint8_t {
    NONE = 0,
    COLLISION_DETECTION,
    SIMULATION,
    COUNT,
};

enum SceneDirtyFlags : uint32_t {
    SCENE_DIRTY_NONE = BIT(0),
    SCENE_DIRTY_WORLD = BIT(1),
    SCENE_DIRTY_CAMERA = BIT(2),
    SCENE_DIRTY_LIGHT = BIT(3),
};
DEFINE_ENUM_BITWISE_OPERATIONS(SceneDirtyFlags);

class Scene : public NonCopyable, public IAsset {
    ecs::ComponentLibrary m_componentLib;

public:
    static constexpr const char* EXTENSION = ".scene";

    Scene() : IAsset(AssetType::SCENE) {}

public:
    template<Serializable T>
    const T* GetComponent(const ecs::Entity&) const { return nullptr; }
    template<Serializable T>
    T* GetComponent(const ecs::Entity&) { return nullptr; }
    template<Serializable T>
    bool Contains(const ecs::Entity&) const { return false; }
    template<Serializable T>
    size_t GetCount() const { return 0; }
    template<Serializable T>
    ecs::Entity GetEntity(size_t) const { return ecs::Entity::INVALID; }
    template<Serializable T>
    T& Create(const ecs::Entity&) { return *(T*)(nullptr); }

    template<Serializable T>
    inline T& GetComponentByIndex(size_t) { return *(T*)0; }
    template<Serializable T>
    inline ecs::Entity GetEntityByIndex(size_t) { return ecs::Entity::INVALID; }

    template<typename T>
    inline ecs::View<T> View() {
        static_assert(0, "this code should never instantiate");
        struct Dummy {};
        ecs::ComponentManager<Dummy> dummyManager;
        return ecs::View(dummyManager);
    }

    template<typename T>
    inline const ecs::View<T> View() const {
        static_assert(0, "this code should never instantiate");
        struct Dummy {};
        ecs::ComponentManager<Dummy> dummyManager;
        return ecs::View(dummyManager);
    }

#pragma region WORLD_COMPONENTS_REGISTRY
#define REGISTER_COMPONENT(T, NAME, VER)                                                                           \
    ecs::ComponentManager<T>& m_##T##s = m_componentLib.RegisterManager<T>(NAME, VER);                             \
    template<>                                                                                                     \
    inline T& GetComponentByIndex<T>(size_t p_index) { return m_##T##s.m_componentArray[p_index]; }                \
    template<>                                                                                                     \
    inline ecs::Entity GetEntityByIndex<T>(size_t p_index) { return m_##T##s.m_entityArray[p_index]; }             \
    template<>                                                                                                     \
    inline const T* GetComponent<T>(const ecs::Entity& p_entity) const { return m_##T##s.GetComponent(p_entity); } \
    template<>                                                                                                     \
    inline T* GetComponent<T>(const ecs::Entity& p_entity) { return m_##T##s.GetComponent(p_entity); }             \
    template<>                                                                                                     \
    inline bool Contains<T>(const ecs::Entity& p_entity) const { return m_##T##s.Contains(p_entity); }             \
    template<>                                                                                                     \
    inline size_t GetCount<T>() const { return m_##T##s.GetCount(); }                                              \
    template<>                                                                                                     \
    inline ecs::Entity GetEntity<T>(size_t p_index) const { return m_##T##s.GetEntity(p_index); }                  \
    template<>                                                                                                     \
    T& Create<T>(const ecs::Entity& p_entity) { return m_##T##s.Create(p_entity); }                                \
    template<>                                                                                                     \
    inline ecs::View<T> View() { return ecs::View<T>(m_##T##s); }                                                  \
    template<>                                                                                                     \
    inline const ecs::View<T> View() const { return ecs::View<T>(m_##T##s); }

#pragma endregion WORLD_COMPONENTS_REGISTRY

    REGISTER_COMPONENT_LIST
#undef REGISTER_COMPONENT

public:
    void Update(float p_delta_time);

    void Copy(Scene& p_other);

    void Merge(Scene& p_other);

    ecs::Entity GetMainCamera();

    ecs::Entity GetEditorCamera();

    ecs::Entity CreatePerspectiveCameraEntity(const std::string& p_name,
                                              int p_width,
                                              int p_height,
                                              float p_near_plane = PerspectiveCameraComponent::DEFAULT_NEAR,
                                              float p_far_plane = PerspectiveCameraComponent::DEFAULT_FAR,
                                              Degree p_fovy = PerspectiveCameraComponent::DEFAULT_FOVY);

    ecs::Entity CreateNameEntity(const std::string& p_name);
    ecs::Entity CreateTransformEntity(const std::string& p_name);
    ecs::Entity CreateObjectEntity(const std::string& p_name);
    ecs::Entity CreateMeshEntity(const std::string& p_name);
    ecs::Entity CreateMaterialEntity(const std::string& p_name);

    ecs::Entity CreatePointLightEntity(const std::string& p_name,
                                       const Vector3f& p_position = Vector3f(0.0f, 1.0f, 0.0f),
                                       const Vector3f& p_color = Vector3f(1.0f),
                                       const float p_emissive = 5.0f);

    ecs::Entity CreateAreaLightEntity(const std::string& p_name,
                                      const Vector3f& p_color = Vector3f(1),
                                      const float p_emissive = 5.0f);

    ecs::Entity CreateInfiniteLightEntity(const std::string& p_name,
                                          const Vector3f& p_color = Vector3f(1),
                                          const float p_emissive = 5.0f);

    ecs::Entity CreateEnvironmentEntity(const std::string& p_name);

    ecs::Entity CreateVoxelGiEntity(const std::string& p_name);

    ecs::Entity CreatePlaneEntity(const std::string& p_name,
                                  const Vector3f& p_scale = Vector3f(0.5f),
                                  const Matrix4x4f& p_transform = Matrix4x4f(1.0f));

    ecs::Entity CreatePlaneEntity(const std::string& p_name,
                                  ecs::Entity p_material_id,
                                  const Vector3f& p_scale = Vector3f(0.5f),
                                  const Matrix4x4f& p_transform = Matrix4x4f(1.0f));

    ecs::Entity CreateCubeEntity(const std::string& p_name,
                                 const Vector3f& p_scale = Vector3f(0.5f),
                                 const Matrix4x4f& p_transform = Matrix4x4f(1.0f));

    ecs::Entity CreateCubeEntity(const std::string& p_name,
                                 ecs::Entity p_material_id,
                                 const Vector3f& p_scale = Vector3f(0.5f),
                                 const Matrix4x4f& p_transform = Matrix4x4f(1.0f));

    ecs::Entity CreateMeshEntity(const std::string& p_name,
                                 ecs::Entity p_material_id,
                                 MeshComponent&& p_mesh);

    ecs::Entity CreateSphereEntity(const std::string& p_name,
                                   float p_radius = 0.5f,
                                   const Matrix4x4f& p_transform = Matrix4x4f(1.0f));

    ecs::Entity CreateSphereEntity(const std::string& p_name,
                                   ecs::Entity p_material_id,
                                   float p_radius = 0.5f,
                                   const Matrix4x4f& p_transform = Matrix4x4f(1.0f));

    ecs::Entity CreateCylinderEntity(const std::string& p_name,
                                     float p_radius = 0.5f,
                                     float p_height = 1.0f,
                                     const Matrix4x4f& p_transform = Matrix4x4f(1.0f));

    ecs::Entity CreateCylinderEntity(const std::string& p_name,
                                     ecs::Entity p_material_id,
                                     float p_radius = 0.5f,
                                     float p_height = 1.0f,
                                     const Matrix4x4f& p_transform = Matrix4x4f(1.0f));

    ecs::Entity CreateTorusEntity(const std::string& p_name,
                                  float p_radius = 0.5f,
                                  float p_tube_radius = 0.2f,
                                  const Matrix4x4f& p_transform = Matrix4x4f(1.0f));

    ecs::Entity CreateTorusEntity(const std::string& p_name,
                                  ecs::Entity p_material_id,
                                  float p_radius = 0.5f,
                                  float p_tube_radius = 0.2f,
                                  const Matrix4x4f& p_transform = Matrix4x4f(1.0f));

    ecs::Entity CreateClothEntity(const std::string& p_name,
                                  ecs::Entity p_material_id,
                                  const Vector3f& p_point_0,
                                  const Vector3f& p_point_1,
                                  const Vector3f& p_point_2,
                                  const Vector3f& p_point_3,
                                  const Vector2i& p_res,
                                  ClothFixFlag p_flags,
                                  const Matrix4x4f& p_transform = Matrix4x4f(1.0f));

    ecs::Entity CreateEmitterEntity(const std::string& p_name, const Matrix4x4f& p_transform = Matrix4x4f(1.0f));

    ecs::Entity CreateMeshEmitterEntity(const std::string& p_name, const Vector3f& p_translation = Vector3f::Zero);

    ecs::Entity CreateForceFieldEntity(const std::string& p_name, const Matrix4x4f& p_transform = Matrix4x4f(1.0f));

    ecs::Entity FindEntityByName(const char* p_name);

    void AttachChild(ecs::Entity p_entity, ecs::Entity p_parent);

    void AttachChild(ecs::Entity p_entity) { AttachChild(p_entity, m_root); }

    void RemoveEntity(ecs::Entity p_entity);

    struct RayIntersectionResult {
        ecs::Entity entity;
    };

    RayIntersectionResult Intersects(Ray& p_ray);
    bool RayObjectIntersect(ecs::Entity p_object_id, Ray& p_ray);

    const AABB& GetBound() const { return m_bound; }

    // @TODO: refactor
    ecs::Entity m_root;
    ecs::Entity m_selected;
    float m_timestep = 0.0f;
    bool m_replace = false;

    PhysicsMode m_physicsMode{ PhysicsMode::NONE };
    mutable PhysicsWorldContext* m_physicsWorld{ nullptr };
    mutable lua_State* L{ nullptr };

    const auto& GetLibraryEntries() const { return m_componentLib.m_entries; }
    SceneDirtyFlags GetDirtyFlags() const { return static_cast<SceneDirtyFlags>(m_dirtyFlags.load()); }

private:
    void UpdateHierarchy(size_t p_index);
    void UpdateAnimation(size_t p_index);
    void UpdateArmature(size_t p_index);

    void RunLightUpdateSystem(jobsystem::Context& p_context);
    void RunTransformationUpdateSystem(jobsystem::Context& p_context);
    void RunHierarchyUpdateSystem(jobsystem::Context& p_context);
    void RunAnimationUpdateSystem(jobsystem::Context& p_context);
    void RunArmatureUpdateSystem(jobsystem::Context& p_context);
    void RunObjectUpdateSystem(jobsystem::Context& p_context);
    void RunParticleEmitterUpdateSystem(jobsystem::Context& p_context);
    void RunMeshEmitterUpdateSystem(jobsystem::Context& p_context);

    // @TODO: refactor
    AABB m_bound;

    std::atomic<uint32_t> m_dirtyFlags{ SCENE_DIRTY_NONE };
};

}  // namespace my
