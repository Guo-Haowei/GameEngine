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
#define REGISTER_COMPONENT(T, VER)                                                              \
public:                                                                                         \
    ecs::ComponentManager<T>& m_##T##s = m_component_lib.registerManager<T>("World::" #T, VER); \
                                                                                                \
public:                                                                                         \
    template<>                                                                                  \
    inline const T* getComponent<T>(const ecs::Entity& p_entity) const {                        \
        return m_##T##s.getComponent(p_entity);                                                 \
    }                                                                                           \
    template<>                                                                                  \
    inline T* getComponent<T>(const ecs::Entity& p_entity) {                                    \
        return m_##T##s.getComponent(p_entity);                                                 \
    }                                                                                           \
    template<>                                                                                  \
    inline bool contains<T>(const ecs::Entity& p_entity) const {                                \
        return m_##T##s.contains(p_entity);                                                     \
    }                                                                                           \
    template<>                                                                                  \
    inline size_t getIndex<T>(const ecs::Entity& p_entity) const {                              \
        return m_##T##s.getIndex(p_entity);                                                     \
    }                                                                                           \
    template<>                                                                                  \
    inline size_t getCount<T>() const {                                                         \
        return m_##T##s.getCount();                                                             \
    }                                                                                           \
    template<>                                                                                  \
    inline ecs::Entity getEntity<T>(size_t p_index) const {                                     \
        return m_##T##s.getEntity(p_index);                                                     \
    }                                                                                           \
    template<>                                                                                  \
    T& create<T>(const ecs::Entity& p_entity) {                                                 \
        return m_##T##s.create(p_entity);                                                       \
    }                                                                                           \
    enum { __DUMMY_ENUM_TO_FORCE_SEMI_COLON_##T }

#pragma endregion WORLD_COMPONENTS_REGISTERY
    template<typename T>
    const T* getComponent(const ecs::Entity&) const {
        return nullptr;
    }
    template<typename T>
    T* getComponent(const ecs::Entity&) {
        return nullptr;
    }
    template<typename T>
    bool contains(const ecs::Entity&) const {
        return false;
    }
    template<typename T>
    size_t getIndex(const ecs::Entity&) const {
        return ecs::Entity::INVALID_INDEX;
    }
    template<typename T>
    size_t getCount() const {
        return 0;
    }
    template<typename T>
    ecs::Entity getEntity(size_t) const {
        return ecs::Entity::INVALID;
    }
    template<typename T>
    T& create(const ecs::Entity&) {
        return *(T*)(nullptr);
    }

private:
    ecs::ComponentLibrary m_component_lib;

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

    bool serialize(Archive& p_archive);

    void update(float p_delta_time);

    void merge(Scene& p_other);

    void createCamera(int p_width,
                      int p_height,
                      float p_near_plane = Camera::DEFAULT_NEAR,
                      float p_far_plane = Camera::DEFAULT_FAR,
                      Degree p_fovy = Camera::DEFAULT_FOVY);

    ecs::Entity createNameEntity(const std::string& p_name);
    ecs::Entity createTransformEntity(const std::string& p_name);
    ecs::Entity createObjectEntity(const std::string& p_name);
    ecs::Entity createMeshEntity(const std::string& p_name);
    ecs::Entity createMaterialEntity(const std::string& p_name);

    ecs::Entity createPointLightEntity(const std::string& p_name,
                                       const vec3& p_position,
                                       const vec3& p_color = vec3(1),
                                       const float p_emissive = 5.0f);

    ecs::Entity createAreaLightEntity(const std::string& p_name,
                                      const vec3& p_color = vec3(1),
                                      const float p_emissive = 5.0f);

    ecs::Entity createInfiniteLightEntity(const std::string& p_name,
                                          const vec3& p_color = vec3(1),
                                          const float p_emissive = 5.0f);

    ecs::Entity createPlaneEntity(const std::string& p_name,
                                  const vec3& p_scale = vec3(0.5f),
                                  const mat4& p_transform = mat4(1.0f));

    ecs::Entity createPlaneEntity(const std::string& p_name,
                                  ecs::Entity p_material_id,
                                  const vec3& p_scale = vec3(0.5f),
                                  const mat4& p_transform = mat4(1.0f));

    ecs::Entity createCubeEntity(const std::string& p_name,
                                 const vec3& p_scale = vec3(0.5f),
                                 const mat4& p_transform = mat4(1.0f));

    ecs::Entity createCubeEntity(const std::string& p_name,
                                 ecs::Entity p_material_id,
                                 const vec3& p_scale = vec3(0.5f),
                                 const mat4& p_transform = mat4(1.0f));

    ecs::Entity createSphereEntity(const std::string& p_name,
                                   float p_radius = 0.5f,
                                   const mat4& p_transform = mat4(1.0f));

    ecs::Entity createSphereEntity(const std::string& p_name,
                                   ecs::Entity p_material_id,
                                   float p_radius = 0.5f,
                                   const mat4& p_transform = mat4(1.0f));

    void attachComponent(ecs::Entity p_entity, ecs::Entity p_parent);

    void attachComponent(ecs::Entity p_entity) { attachComponent(p_entity, m_root); }

    void removeEntity(ecs::Entity p_entity);

    struct RayIntersectionResult {
        ecs::Entity entity;
    };

    RayIntersectionResult intersects(Ray& p_ray);
    bool rayObjectIntersect(ecs::Entity p_object_id, Ray& p_ray);

    const AABB& getBound() const { return m_bound; }
    // @TODO: refactor
    ecs::Entity m_root;
    ecs::Entity m_selected;
    float m_delta_time = 0.0f;
    std::shared_ptr<Camera> m_camera;
    bool m_replace = false;

private:
    void updateHierarchy(uint32_t p_index);
    void updateAnimation(uint32_t p_index);
    void updateArmature(uint32_t p_index);
    void updateLight(uint32_t p_index);

    void runLightUpdateSystem(jobsystem::Context& p_ctx);
    void runTransformationUpdateSystem(jobsystem::Context& p_ctx);
    void runHierarchyUpdateSystem(jobsystem::Context& p_ctx);
    void runAnimationUpdateSystem(jobsystem::Context& p_ctx);
    void runArmatureUpdateSystem(jobsystem::Context& p_ctx);
    void runObjectUpdateSystem(jobsystem::Context& p_ctx);

    // @TODO: refactor
    AABB m_bound;
};

}  // namespace my
