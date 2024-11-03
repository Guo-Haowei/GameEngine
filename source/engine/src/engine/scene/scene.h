#pragma once
#include "core/base/noncopyable.h"
#include "core/math/ray.h"
#include "core/systems/component_manager.h"
#include "scene/animation_component.h"
#include "scene/armature_component.h"
#include "scene/camera.h"
#include "scene/collider_component.h"
#include "scene/hierarchy_component.h"
#include "scene/light_component.h"
#include "scene/material_component.h"
#include "scene/mesh_component.h"
#include "scene/name_component.h"
#include "scene/object_component.h"
#include "scene/particle_emitter_component.h"
#include "scene/rigid_body_component.h"
#include "scene/transform_component.h"

namespace my {

namespace jobsystem {
class Context;
}

class Scene : public NonCopyable {
public:
    static constexpr const char* EXTENSION = ".scene";

    Scene() = default;

#pragma region WORLD_COMPONENTS_REGISTERY
#define REGISTER_COMPONENT(T, VER)                                                             \
public:                                                                                        \
    ecs::ComponentManager<T>& m_##T##s = m_componentLib.RegisterManager<T>("World::" #T, VER); \
                                                                                               \
public:                                                                                        \
    template<>                                                                                 \
    inline const T* GetComponent<T>(const ecs::Entity& p_entity) const {                       \
        return m_##T##s.GetComponent(p_entity);                                                \
    }                                                                                          \
    template<>                                                                                 \
    inline T* GetComponent<T>(const ecs::Entity& p_entity) {                                   \
        return m_##T##s.GetComponent(p_entity);                                                \
    }                                                                                          \
    template<>                                                                                 \
    inline bool Contains<T>(const ecs::Entity& p_entity) const {                               \
        return m_##T##s.Contains(p_entity);                                                    \
    }                                                                                          \
    template<>                                                                                 \
    inline size_t GetIndex<T>(const ecs::Entity& p_entity) const {                             \
        return m_##T##s.GetIndex(p_entity);                                                    \
    }                                                                                          \
    template<>                                                                                 \
    inline size_t GetCount<T>() const {                                                        \
        return m_##T##s.GetCount();                                                            \
    }                                                                                          \
    template<>                                                                                 \
    inline ecs::Entity GetEntity<T>(size_t p_index) const {                                    \
        return m_##T##s.GetEntity(p_index);                                                    \
    }                                                                                          \
    template<>                                                                                 \
    T& Create<T>(const ecs::Entity& p_entity) {                                                \
        return m_##T##s.Create(p_entity);                                                      \
    }                                                                                          \
    enum { __DUMMY_ENUM_TO_FORCE_SEMI_COLON_##T }

#pragma endregion WORLD_COMPONENTS_REGISTERY
    template<typename T>
    const T* GetComponent(const ecs::Entity&) const {
        return nullptr;
    }
    template<typename T>
    T* GetComponent(const ecs::Entity&) {
        return nullptr;
    }
    template<typename T>
    bool Contains(const ecs::Entity&) const {
        return false;
    }
    template<typename T>
    size_t GetIndex(const ecs::Entity&) const {
        return ecs::Entity::INVALID_INDEX;
    }
    template<typename T>
    size_t GetCount() const {
        return 0;
    }
    template<typename T>
    ecs::Entity GetEntity(size_t) const {
        return ecs::Entity::INVALID;
    }
    template<typename T>
    T& Create(const ecs::Entity&) {
        return *(T*)(nullptr);
    }

private:
    ecs::ComponentLibrary m_componentLib;

    REGISTER_COMPONENT(NameComponent, 0);
    REGISTER_COMPONENT(TransformComponent, 0);
    REGISTER_COMPONENT(HierarchyComponent, 0);
    REGISTER_COMPONENT(MaterialComponent, 0);
    REGISTER_COMPONENT(MeshComponent, 0);
    REGISTER_COMPONENT(ObjectComponent, 0);
    REGISTER_COMPONENT(LightComponent, 0);
    REGISTER_COMPONENT(ArmatureComponent, 0);
    REGISTER_COMPONENT(AnimationComponent, 0);
    REGISTER_COMPONENT(RigidBodyComponent, 0);
    REGISTER_COMPONENT(BoxColliderComponent, 0);
    REGISTER_COMPONENT(MeshColliderComponent, 0);
    REGISTER_COMPONENT(ParticleEmitterComponent, 0);

public:
    bool Serialize(Archive& p_archive);

    void Update(float p_delta_time);

    void Merge(Scene& p_other);

    void CreateCamera(int p_width,
                      int p_height,
                      float p_near_plane = Camera::DEFAULT_NEAR,
                      float p_far_plane = Camera::DEFAULT_FAR,
                      Degree p_fovy = Camera::DEFAULT_FOVY);

    ecs::Entity CreateNameEntity(const std::string& p_name);
    ecs::Entity CreateTransformEntity(const std::string& p_name);
    ecs::Entity CreateObjectEntity(const std::string& p_name);
    ecs::Entity CreateMeshEntity(const std::string& p_name);
    ecs::Entity CreateMaterialEntity(const std::string& p_name);

    ecs::Entity CreatePointLightEntity(const std::string& p_name,
                                       const vec3& p_position,
                                       const vec3& p_color = vec3(1),
                                       const float p_emissive = 5.0f);

    ecs::Entity CreateAreaLightEntity(const std::string& p_name,
                                      const vec3& p_color = vec3(1),
                                      const float p_emissive = 5.0f);

    ecs::Entity CreateInfiniteLightEntity(const std::string& p_name,
                                          const vec3& p_color = vec3(1),
                                          const float p_emissive = 5.0f);

    ecs::Entity CreatePlaneEntity(const std::string& p_name,
                                  const vec3& p_scale = vec3(0.5f),
                                  const mat4& p_transform = mat4(1.0f));

    ecs::Entity CreatePlaneEntity(const std::string& p_name,
                                  ecs::Entity p_material_id,
                                  const vec3& p_scale = vec3(0.5f),
                                  const mat4& p_transform = mat4(1.0f));

    ecs::Entity CreateCubeEntity(const std::string& p_name,
                                 const vec3& p_scale = vec3(0.5f),
                                 const mat4& p_transform = mat4(1.0f));

    ecs::Entity CreateCubeEntity(const std::string& p_name,
                                 ecs::Entity p_material_id,
                                 const vec3& p_scale = vec3(0.5f),
                                 const mat4& p_transform = mat4(1.0f));

    ecs::Entity CreateSphereEntity(const std::string& p_name,
                                   float p_radius = 0.5f,
                                   const mat4& p_transform = mat4(1.0f));

    ecs::Entity CreateSphereEntity(const std::string& p_name,
                                   ecs::Entity p_material_id,
                                   float p_radius = 0.5f,
                                   const mat4& p_transform = mat4(1.0f));

    ecs::Entity CreateCylinderEntity(const std::string& p_name,
                                     float p_radius = 0.5f,
                                     float p_height = 1.0f,
                                     const mat4& p_transform = mat4(1.0f));

    ecs::Entity CreateCylinderEntity(const std::string& p_name,
                                     ecs::Entity p_material_id,
                                     float p_radius = 0.5f,
                                     float p_height = 1.0f,
                                     const mat4& p_transform = mat4(1.0f));

    ecs::Entity CreateParticleEmitter(const std::string& p_name, const mat4& p_transform = mat4(1.0f));

    void AttachComponent(ecs::Entity p_entity, ecs::Entity p_parent);

    void AttachComponent(ecs::Entity p_entity) { AttachComponent(p_entity, m_root); }

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
    float m_elapsedTime = 0.0f;
    std::shared_ptr<Camera> m_camera;
    bool m_replace = false;

private:
    void UpdateHierarchy(uint32_t p_index);
    void UpdateAnimation(uint32_t p_index);
    void UpdateArmature(uint32_t p_index);
    void UpdateLight(uint32_t p_index);

    void RunLightUpdateSystem(jobsystem::Context& p_context);
    void RunTransformationUpdateSystem(jobsystem::Context& p_context);
    void RunHierarchyUpdateSystem(jobsystem::Context& p_context);
    void RunAnimationUpdateSystem(jobsystem::Context& p_context);
    void RunArmatureUpdateSystem(jobsystem::Context& p_context);
    void RunObjectUpdateSystem(jobsystem::Context& p_context);
    void RunParticleEmitterUpdateSystem(jobsystem::Context& p_context);

    // @TODO: refactor
    AABB m_bound;
};

}  // namespace my
