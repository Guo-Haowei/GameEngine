#include "render_data.h"

#include "core/base/rid_owner.h"
#include "core/math/frustum.h"
#include "gl_utils.h"
#include "r_cbuffers.h"
#include "scene/scene.h"

extern my::RIDAllocator<MeshData> g_meshes;

namespace my {

void RenderData::clear() {
    scene = nullptr;
    for (int i = 0; i < SC_NUM_CASCADES; ++i) {
        shadow_passes[i].clear();
    }
    main_pass.clear();
    voxel_pass.clear();
}

void RenderData::update(const Scene* p_scene) {
    clear();
    scene = p_scene;

    // cascaded shadow map
    for (int i = 0; i < SC_NUM_CASCADES; ++i) {
        mat4 light_matrix = g_perFrameCache.cache.c_main_light_matrices[i];
        Frustum light_frustum(light_matrix);
        fill(
            p_scene,
            light_matrix,
            shadow_passes[i],
            [](const ObjectComponent& object) {
                return !(object.flags & ObjectComponent::CAST_SHADOW) || !(object.flags & ObjectComponent::RENDERABLE);
            },
            [&](const AABB& aabb) {
                return light_frustum.intersects(aabb);
            });
    }

    // voxel pass
    fill(
        p_scene,
        g_perFrameCache.cache.c_projection_view_matrix,
        voxel_pass,
        [](const ObjectComponent& object) {
            return !(object.flags & ObjectComponent::RENDERABLE);
        },
        [&](const AABB& aabb) {
            unused(aabb);
            // return scene->get_bound().intersects(aabb);
            return true;
        });

    Frustum camera_frustum(g_perFrameCache.cache.c_projection_view_matrix);
    // main pass
    fill(
        p_scene,
        g_perFrameCache.cache.c_projection_view_matrix,
        main_pass,
        [](const ObjectComponent& object) {
            return !(object.flags & ObjectComponent::RENDERABLE);
        },
        [&](const AABB& aabb) {
            return camera_frustum.intersects(aabb);
        });
}

void RenderData::fill(const Scene* p_scene, const mat4& projection_view_matrix, Pass& pass, FilterObjectFunc1 func1, FilterObjectFunc2 func2) {
    scene = p_scene;
    pass.projection_view_matrix = projection_view_matrix;
    Frustum frustum{ projection_view_matrix };
    uint32_t num_objects = (uint32_t)scene->get_count<ObjectComponent>();
    for (uint32_t i = 0; i < num_objects; ++i) {
        ecs::Entity entity = scene->get_entity<ObjectComponent>(i);
        if (!scene->contains<TransformComponent>(entity)) {
            continue;
        }

        const ObjectComponent& obj = scene->get_component_array<ObjectComponent>()[i];
        if (func1(obj)) {
            continue;
        }

        const TransformComponent& transform = *scene->get_component<TransformComponent>(entity);
        DEV_ASSERT(scene->contains<MeshComponent>(obj.mesh_id));
        const MeshComponent& mesh = *scene->get_component<MeshComponent>(obj.mesh_id);

        const mat4& world_matrix = transform.get_world_matrix();
        AABB aabb = mesh.local_bound;
        aabb.apply_matrix(world_matrix);
        if (!func2(aabb)) {
            continue;
        }

        Mesh draw;
        draw.armature_id = mesh.armature_id;
        draw.world_matrix = world_matrix;
        const MeshData* mesh_data = g_meshes.get_or_null(mesh.gpu_resource);
        if (!mesh_data) {
            continue;
        }
        draw.mesh_data = mesh_data;

        for (const auto& subset : mesh.subsets) {
            aabb = subset.local_bound;
            aabb.apply_matrix(world_matrix);
            if (!frustum.intersects(aabb)) {
                continue;
            }

            SubMesh sub_mesh;
            sub_mesh.index_count = subset.index_count;
            sub_mesh.index_offset = subset.index_offset;
            sub_mesh.material = scene->get_component<MaterialComponent>(subset.material_id);
            draw.subsets.emplace_back(std::move(sub_mesh));
        }

        pass.draws.emplace_back(std::move(draw));
    }
}

}  // namespace my
