#include "scene.h"

#include "engine/core/debugger/profiler.h"
#include "engine/core/io/archive.h"
#include "engine/ecs/component_manager.inl"
#include "engine/math/geometry.h"
#include "engine/runtime/asset_registry.h"
#include "engine/systems/ecs_systems.h"
#include "engine/systems/job_system/job_system.h"

// @TODO: refactor
#include "engine/renderer/graphics_dvars.h"
#include "engine/renderer/path_tracer/bvh_accel.h"

namespace my::ecs {

// instantiate ComponentManagers
#define REGISTER_COMPONENT(TYPE, ...) template class ComponentManager<::my::TYPE>;
REGISTER_COMPONENT_LIST
#undef REGISTER_COMPONENT

}  // namespace my::ecs

namespace my {

void Scene::Update(float p_timestep) {
    HBN_PROFILE_EVENT();

    m_dirtyFlags.store(0);

    jobsystem::Context ctx;
    // animation
    RunLightUpdateSystem(*this, ctx, p_timestep);
    RunAnimationUpdateSystem(*this, ctx, p_timestep);
    ctx.Wait();
    // transform, update local matrix from position, rotation and scale
    RunTransformationUpdateSystem(*this, ctx, p_timestep);
    ctx.Wait();
    // hierarchy, update world matrix based on hierarchy
    RunHierarchyUpdateSystem(*this, ctx, p_timestep);
    ctx.Wait();
    // mesh particles
    RunMeshEmitterUpdateSystem(*this, ctx, p_timestep);
    // particle
    RunParticleEmitterUpdateSystem(*this, ctx, p_timestep);
    // armature
    RunArmatureUpdateSystem(*this, ctx, p_timestep);
    ctx.Wait();

    // update bounding box
    RunObjectUpdateSystem(*this, ctx, p_timestep);

    // @TODO: refactor
    for (auto [entity, camera] : m_CameraComponents) {
        if (camera.Update()) {
            m_dirtyFlags.fetch_or(SCENE_DIRTY_CAMERA);
        }
    }

    for (auto [entity, voxel_gi] : m_VoxelGiComponents) {
        auto transform = GetComponent<TransformComponent>(entity);
        if (DEV_VERIFY(transform)) {
            const auto& matrix = transform->GetWorldMatrix();
            Vector3f center{ matrix[3].x, matrix[3].y, matrix[3].z };
            Vector3f scale = transform->GetScale();
            const float size = glm::max(scale.x, glm::max(scale.y, scale.z));
            voxel_gi.region = AABB::FromCenterSize(center, Vector3f(size));
        }
    }

    // @TODO: refactor
    if (DVAR_GET_BOOL(gfx_bvh_generate)) {
        for (auto [entity, mesh] : m_MeshComponents) {
            if (!mesh.bvh) {
                mesh.bvh = BvhAccel::Construct(mesh.indices, mesh.positions);
            }
        }
        DVAR_SET_BOOL(gfx_bvh_generate, false);
    }
}

void Scene::Copy(Scene& p_other) {
    for (auto& entry : m_componentLib.m_entries) {
        auto& manager = *p_other.m_componentLib.m_entries[entry.first].m_manager;
        entry.second.m_manager->Copy(manager);
    }

    m_root = p_other.m_root;
    m_bound = p_other.m_bound;
    m_physicsMode = p_other.m_physicsMode;
}

void Scene::Merge(Scene& p_other) {
    for (auto& entry : m_componentLib.m_entries) {
        auto& manager = *p_other.m_componentLib.m_entries[entry.first].m_manager;
        entry.second.m_manager->Merge(manager);
    }
    if (p_other.m_root.IsValid()) {
        AttachChild(p_other.m_root, m_root);
    }

    m_bound.UnionBox(p_other.m_bound);
}

ecs::Entity Scene::GetMainCamera() {
    for (auto [entity, camera] : m_CameraComponents) {
        if (camera.IsPrimary()) {
            return entity;
        }
    }

    return ecs::Entity::INVALID;
}

ecs::Entity Scene::GetEditorCamera() {
    for (auto [entity, camera] : m_CameraComponents) {
        if (camera.IsEditorCamera()) {
            return entity;
        }
    }

    return ecs::Entity::INVALID;
}

ecs::Entity Scene::CreatePerspectiveCameraEntity(const std::string& p_name,
                                                 int p_width,
                                                 int p_height,
                                                 float p_near_plane,
                                                 float p_far_plane,
                                                 Degree p_fovy) {
    auto entity = CreateNameEntity(p_name);
    CameraComponent& camera = Create<CameraComponent>(entity);

    camera.m_width = p_width;
    camera.m_height = p_height;
    camera.m_near = p_near_plane;
    camera.m_far = p_far_plane;
    camera.m_fovy = p_fovy;
    camera.m_pitch = Degree{ -10.0f };
    camera.m_yaw = Degree{ -90.0f };
    camera.SetDirty();
    return entity;
}

ecs::Entity Scene::CreateNameEntity(const std::string& p_name) {
    auto entity = ecs::Entity::Create();
    Create<NameComponent>(entity).SetName(p_name);
    return entity;
}

ecs::Entity Scene::CreateTransformEntity(const std::string& p_name) {
    auto entity = CreateNameEntity(p_name);
    Create<TransformComponent>(entity);
    return entity;
}

ecs::Entity Scene::CreateObjectEntity(const std::string& p_name) {
    auto entity = CreateNameEntity(p_name);
    Create<MeshRendererComponent>(entity);
    Create<TransformComponent>(entity);
    return entity;
}

ecs::Entity Scene::CreateMeshEntity(const std::string& p_name) {
    auto entity = CreateNameEntity(p_name);
    Create<MeshComponent>(entity);
    return entity;
}

ecs::Entity Scene::CreateMaterialEntity(const std::string& p_name) {
    auto entity = CreateNameEntity(p_name);
    Create<MaterialComponent>(entity);
    return entity;
}

ecs::Entity Scene::CreatePointLightEntity(const std::string& p_name,
                                          const Vector3f& p_position,
                                          const Vector3f& p_color,
                                          const float p_emissive) {
    auto entity = CreateObjectEntity(p_name);

    LightComponent& light = Create<LightComponent>(entity);
    light.SetType(LIGHT_TYPE_POINT);
    light.m_atten.constant = 1.0f;
    light.m_atten.linear = 0.2f;
    light.m_atten.quadratic = 0.05f;

    MaterialComponent& material = Create<MaterialComponent>(entity);
    material.baseColor = Vector4f(p_color, 1.0f);
    material.emissive = p_emissive;

    TransformComponent& transform = *GetComponent<TransformComponent>(entity);
    MeshRendererComponent& object = *GetComponent<MeshRendererComponent>(entity);
    transform.SetTranslation(p_position);
    transform.SetDirty();

    auto mesh_id = CreateMeshEntity(p_name + ":mesh");
    object.meshId = mesh_id;
    object.flags = MeshRendererComponent::FLAG_RENDERABLE;

    MeshComponent& mesh = *GetComponent<MeshComponent>(mesh_id);
    mesh = MakeSphereMesh(0.1f, 40, 40);
    mesh.subsets[0].material_id = entity;
    return entity;
}

ecs::Entity Scene::CreateAreaLightEntity(const std::string& p_name,
                                         const Vector3f& p_color,
                                         const float p_emissive) {
    auto entity = CreateObjectEntity(p_name);

    // light
    LightComponent& light = Create<LightComponent>(entity);
    light.SetType(LIGHT_TYPE_AREA);
    // light.m_atten.constant = 1.0f;
    // light.m_atten.linear = 0.09f;
    // light.m_atten.quadratic = 0.032f;

    // material
    MaterialComponent& material = Create<MaterialComponent>(entity);
    material.baseColor = Vector4f(p_color, 1.0f);
    material.emissive = p_emissive;

    MeshRendererComponent& object = *GetComponent<MeshRendererComponent>(entity);

    auto mesh_id = CreateMeshEntity(p_name + ":mesh");
    object.meshId = mesh_id;
    object.flags = MeshRendererComponent::FLAG_RENDERABLE;

    MeshComponent& mesh = *GetComponent<MeshComponent>(mesh_id);
    mesh = MakePlaneMesh();
    mesh.subsets[0].material_id = entity;
    return entity;
}

ecs::Entity Scene::CreateInfiniteLightEntity(const std::string& p_name,
                                             const Vector3f& p_color,
                                             const float p_emissive) {
    auto entity = CreateNameEntity(p_name);

    Create<TransformComponent>(entity);

    LightComponent& light = Create<LightComponent>(entity);
    light.SetType(LIGHT_TYPE_INFINITE);
    light.m_atten.constant = 1.0f;
    light.m_atten.linear = 0.0f;
    light.m_atten.quadratic = 0.0f;

    MaterialComponent& material = Create<MaterialComponent>(entity);
    material.baseColor = Vector4f(p_color, 1.0f);
    material.emissive = p_emissive;
    return entity;
}

ecs::Entity Scene::CreateEnvironmentEntity(const std::string& p_name) {
    auto entity = CreateNameEntity(p_name);
    Create<EnvironmentComponent>(entity);
    return entity;
}

ecs::Entity Scene::CreateVoxelGiEntity(const std::string& p_name) {
    auto entity = CreateNameEntity(p_name);
    Create<VoxelGiComponent>(entity);
    Create<TransformComponent>(entity);
    return entity;
}

ecs::Entity Scene::CreatePlaneEntity(const std::string& p_name,
                                     const Vector3f& p_scale,
                                     const Matrix4x4f& p_transform) {
    auto material_id = CreateMaterialEntity(p_name + ":mat");
    return CreatePlaneEntity(p_name, material_id, p_scale, p_transform);
}

ecs::Entity Scene::CreatePlaneEntity(const std::string& p_name,
                                     ecs::Entity p_material_id,
                                     const Vector3f& p_scale,
                                     const Matrix4x4f& p_transform) {
    auto entity = CreateObjectEntity(p_name);
    TransformComponent& trans = *GetComponent<TransformComponent>(entity);
    MeshRendererComponent& object = *GetComponent<MeshRendererComponent>(entity);
    trans.MatrixTransform(p_transform);

    auto mesh_id = CreateMeshEntity(p_name + ":mesh");
    object.meshId = mesh_id;

    MeshComponent& mesh = *GetComponent<MeshComponent>(mesh_id);
    mesh = MakePlaneMesh(p_scale);
    mesh.subsets[0].material_id = p_material_id;

    return entity;
}

ecs::Entity Scene::CreateCubeEntity(const std::string& p_name,
                                    const Vector3f& p_scale,
                                    const Matrix4x4f& p_transform) {
    auto material_id = CreateMaterialEntity(p_name + ":mat");
    return CreateCubeEntity(p_name, material_id, p_scale, p_transform);
}

ecs::Entity Scene::CreateCubeEntity(const std::string& p_name,
                                    ecs::Entity p_material_id,
                                    const Vector3f& p_scale,
                                    const Matrix4x4f& p_transform) {
    auto entity = CreateObjectEntity(p_name);
    TransformComponent& trans = *GetComponent<TransformComponent>(entity);
    MeshRendererComponent& object = *GetComponent<MeshRendererComponent>(entity);
    trans.MatrixTransform(p_transform);

    auto mesh_id = CreateMeshEntity(p_name + ":mesh");
    object.meshId = mesh_id;

    MeshComponent& mesh = *GetComponent<MeshComponent>(mesh_id);
    mesh = MakeCubeMesh(p_scale);
    mesh.subsets[0].material_id = p_material_id;

    return entity;
}

ecs::Entity Scene::CreateMeshEntity(const std::string& p_name,
                                    ecs::Entity p_material_id,
                                    MeshComponent&& p_mesh) {
    auto entity = CreateObjectEntity(p_name);
    MeshRendererComponent& object = *GetComponent<MeshRendererComponent>(entity);

    auto mesh_id = CreateMeshEntity(p_name + ":mesh");
    object.meshId = mesh_id;

    MeshComponent& mesh = *GetComponent<MeshComponent>(mesh_id);
    mesh = std::move(p_mesh);
    mesh.subsets[0].material_id = p_material_id;
    return entity;
}

ecs::Entity Scene::CreateSphereEntity(const std::string& p_name,
                                      float p_radius,
                                      const Matrix4x4f& p_transform) {
    auto material_id = CreateMaterialEntity(p_name + ":mat");
    return CreateSphereEntity(p_name, material_id, p_radius, p_transform);
}

ecs::Entity Scene::CreateSphereEntity(const std::string& p_name,
                                      ecs::Entity p_material_id,
                                      float p_radius,
                                      const Matrix4x4f& p_transform) {
    auto entity = CreateObjectEntity(p_name);
    TransformComponent& transform = *GetComponent<TransformComponent>(entity);
    transform.MatrixTransform(p_transform);

    MeshRendererComponent& object = *GetComponent<MeshRendererComponent>(entity);
    auto mesh_id = CreateMeshEntity(p_name + ":mesh");
    object.meshId = mesh_id;

    MeshComponent& mesh = *GetComponent<MeshComponent>(mesh_id);
    mesh = MakeSphereMesh(p_radius);
    mesh.subsets[0].material_id = p_material_id;

    return entity;
}

ecs::Entity Scene::CreateCylinderEntity(const std::string& p_name,
                                        float p_radius,
                                        float p_height,
                                        const Matrix4x4f& p_transform) {
    auto material_id = CreateMaterialEntity(p_name + ":mat");
    return CreateCylinderEntity(p_name, material_id, p_radius, p_height, p_transform);
}

ecs::Entity Scene::CreateCylinderEntity(const std::string& p_name,
                                        ecs::Entity p_material_id,
                                        float p_radius,
                                        float p_height,
                                        const Matrix4x4f& p_transform) {
    auto entity = CreateObjectEntity(p_name);
    TransformComponent& transform = *GetComponent<TransformComponent>(entity);
    transform.MatrixTransform(p_transform);

    MeshRendererComponent& object = *GetComponent<MeshRendererComponent>(entity);
    auto mesh_id = CreateMeshEntity(p_name + ":mesh");
    object.meshId = mesh_id;

    MeshComponent& mesh = *GetComponent<MeshComponent>(mesh_id);
    mesh = MakeCylinderMesh(p_radius, p_height);
    mesh.subsets[0].material_id = p_material_id;

    return entity;
}

ecs::Entity Scene::CreateTorusEntity(const std::string& p_name,
                                     float p_radius,
                                     float p_tube_radius,
                                     const Matrix4x4f& p_transform) {
    auto material_id = CreateMaterialEntity(p_name + ":mat");
    return CreateTorusEntity(p_name, material_id, p_radius, p_tube_radius, p_transform);
}

ecs::Entity Scene::CreateTorusEntity(const std::string& p_name,
                                     ecs::Entity p_material_id,
                                     float p_radius,
                                     float p_tube_radius,
                                     const Matrix4x4f& p_transform) {
    // @TODO: fix this
    p_radius = 0.4f;
    p_tube_radius = 0.1f;

    auto entity = CreateObjectEntity(p_name);
    TransformComponent& transform = *GetComponent<TransformComponent>(entity);
    transform.MatrixTransform(p_transform);

    MeshRendererComponent& object = *GetComponent<MeshRendererComponent>(entity);
    auto mesh_id = CreateMeshEntity(p_name + ":mesh");
    object.meshId = mesh_id;

    MeshComponent& mesh = *GetComponent<MeshComponent>(mesh_id);
    mesh = MakeTorusMesh(p_radius, p_tube_radius);
    mesh.subsets[0].material_id = p_material_id;

    return entity;
}

ecs::Entity Scene::CreateTileMapEntity(const std::string& p_name, const Matrix4x4f& p_transform) {
    auto entity = CreateNameEntity(p_name);

    TransformComponent& transform = Create<TransformComponent>(entity);
    transform.MatrixTransform(p_transform);

    Create<TileMapComponent>(entity);
    return entity;
}

ecs::Entity Scene::CreateClothEntity(const std::string& p_name,
                                     ecs::Entity p_material_id,
                                     const Vector3f& p_point_0,
                                     const Vector3f& p_point_1,
                                     const Vector3f& p_point_2,
                                     const Vector3f& p_point_3,
                                     const Vector2i& p_res,
                                     ClothFixFlag p_fixed_flags,
                                     const Matrix4x4f& p_transform) {
    // @TODO: fix
    unused(p_transform);

    auto entity = CreateObjectEntity(p_name);
    // TransformComponent& transform = *GetComponent<TransformComponent>(entity);
    // transform.MatrixTransform(p_transform);

    ClothComponent& cloth = Create<ClothComponent>(entity);
    cloth.point_0 = p_point_0;
    cloth.point_1 = p_point_1;
    cloth.point_2 = p_point_2;
    cloth.point_3 = p_point_3;
    cloth.res = p_res;
    cloth.fixedFlags = p_fixed_flags;

    MeshRendererComponent& object = *GetComponent<MeshRendererComponent>(entity);
    auto mesh_id = CreateMeshEntity(p_name + ":mesh");
    object.meshId = mesh_id;

    MeshComponent& mesh = *GetComponent<MeshComponent>(mesh_id);
    mesh = MakePlaneMesh(p_point_0,
                         p_point_1,
                         p_point_2,
                         p_point_3);
    mesh.subsets[0].material_id = p_material_id;

    return entity;
}

ecs::Entity Scene::CreateEmitterEntity(const std::string& p_name, const Matrix4x4f& p_transform) {
    auto entity = CreateTransformEntity(p_name);
    Create<ParticleEmitterComponent>(entity);

    TransformComponent& transform = *GetComponent<TransformComponent>(entity);
    transform.MatrixTransform(p_transform);
    return entity;
}

ecs::Entity Scene::CreateMeshEmitterEntity(const std::string& p_name, const Vector3f& p_translation) {
    auto entity = CreateNameEntity(p_name);
    Create<TransformComponent>(entity).SetTranslation(p_translation);
    Create<MeshEmitterComponent>(entity);
    return entity;
}

ecs::Entity Scene::CreateForceFieldEntity(const std::string& p_name, const Matrix4x4f& p_transform) {
    auto entity = CreateTransformEntity(p_name);
    Create<ForceFieldComponent>(entity);

    TransformComponent& transform = *GetComponent<TransformComponent>(entity);
    transform.MatrixTransform(p_transform);

    return entity;
}

ecs::Entity Scene::FindEntityByName(const char* p_name) {
    for (auto [entity, name] : m_NameComponents) {
        if (name.GetName() == p_name) {
            return entity;
        }
    }
    return ecs::Entity::INVALID;
}

void Scene::AttachChild(ecs::Entity p_child, ecs::Entity p_parent) {
    DEV_ASSERT(p_child != p_parent);
    DEV_ASSERT(p_parent.IsValid());

    // if child already has a parent, detach it
    if (Contains<HierarchyComponent>(p_child)) {
        CRASH_NOW_MSG("Unlikely to happen at this point");
    }

    HierarchyComponent& hier = Create<HierarchyComponent>(p_child);
    hier.m_parentId = p_parent;
}

void Scene::RemoveEntity(ecs::Entity p_entity) {
    std::vector<ecs::Entity> children;
    for (auto [child, hierarchy] : m_HierarchyComponents) {
        if (hierarchy.GetParent() == p_entity) {
            children.emplace_back(child);
        }
    }
    for (auto child : children) {
        RemoveEntity(child);
    }

    LightComponent* light = GetComponent<LightComponent>(p_entity);
    if (light) {
        // @TODO: shadow atlas
        m_LightComponents.Remove(p_entity);
    }
    m_HierarchyComponents.Remove(p_entity);
    m_TransformComponents.Remove(p_entity);
    m_MeshRendererComponents.Remove(p_entity);
    m_ParticleEmitterComponents.Remove(p_entity);
    m_ForceFieldComponents.Remove(p_entity);
    m_NameComponents.Remove(p_entity);
}

bool Scene::RayObjectIntersect(ecs::Entity p_object_id, Ray& p_ray) {
    MeshRendererComponent* object = GetComponent<MeshRendererComponent>(p_object_id);
    MeshComponent* mesh = GetComponent<MeshComponent>(object->meshId);
    TransformComponent* transform = GetComponent<TransformComponent>(p_object_id);
    DEV_ASSERT(mesh && transform);

    if (!transform || !mesh) {
        return false;
    }

    Matrix4x4f inversedModel = glm::inverse(transform->GetWorldMatrix());
    Ray inversedRay = p_ray.Inverse(inversedModel);
    Ray inversedRayAABB = inversedRay;  // make a copy, we don't want dist to be modified by AABB
    // Perform aabb test
    if (!inversedRayAABB.Intersects(mesh->localBound)) {
        return false;
    }

    // @TODO: test submesh intersection

    // Test every single triange
    for (size_t i = 0; i < mesh->indices.size(); i += 3) {
        const Vector3f& A = mesh->positions[mesh->indices[i]];
        const Vector3f& B = mesh->positions[mesh->indices[i + 1]];
        const Vector3f& C = mesh->positions[mesh->indices[i + 2]];
#define CC(a) Vector3f(a.x, a.y, a.z)
        if (inversedRay.Intersects(CC(A), CC(B), CC(C))) {
#undef CC
            p_ray.CopyDist(inversedRay);
            return true;
        }
    }
    return false;
}

Scene::RayIntersectionResult Scene::Intersects(Ray& p_ray) {
    RayIntersectionResult result;

    // @TODO: box collider
    for (size_t object_idx = 0; object_idx < GetCount<MeshRendererComponent>(); ++object_idx) {
        ecs::Entity entity = GetEntity<MeshRendererComponent>(object_idx);
        if (RayObjectIntersect(entity, p_ray)) {
            result.entity = entity;
        }
    }

    return result;
}

}  // namespace my
