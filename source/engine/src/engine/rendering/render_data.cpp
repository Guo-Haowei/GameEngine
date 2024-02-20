#include "render_data.h"

#include "core/base/rid_owner.h"
#include "core/framework/asset_manager.h"
#include "core/framework/common_dvars.h"
#include "core/math/frustum.h"
#include "gl_utils.h"
#include "r_cbuffers.h"
#include "scene/scene.h"

extern my::RIDAllocator<MeshData> g_meshes;

namespace my {

void RenderData::clear() {
    scene = nullptr;

    for (auto& pass : shadow_passes) {
        pass.clear();
    }

    point_shadow_passes.clear();
    main_pass.clear();
    voxel_pass.clear();
}

void RenderData::point_light_draw_data() {
    // point shadow map
    for (int light_idx = 0; light_idx < (int)scene->get_count<LightComponent>(); ++light_idx) {
        auto light_id = scene->get_entity<LightComponent>(light_idx);
        const LightComponent& light = scene->get_component_array<LightComponent>()[light_idx];
        if (light.type == LIGHT_TYPE_POINT && light.cast_shadow()) {
            const TransformComponent* transform = scene->get_component<TransformComponent>(light_id);
            DEV_ASSERT(transform);
            vec3 position = transform->get_translation();

            const mat4* light_space_matrices = g_perFrameCache.cache.c_lights[light_idx].matrices;
            std::array<Frustum, 6> frustums = {
                Frustum{ light_space_matrices[0] },
                Frustum{ light_space_matrices[1] },
                Frustum{ light_space_matrices[2] },
                Frustum{ light_space_matrices[3] },
                Frustum{ light_space_matrices[4] },
                Frustum{ light_space_matrices[5] },
            };

            Pass pass;
            fill(
                scene,
                mat4(1),
                pass,
                [](const ObjectComponent& object) {
                    return !(object.flags & ObjectComponent::CAST_SHADOW) || !(object.flags & ObjectComponent::RENDERABLE);
                },
                [&](const AABB& aabb) {
                    for (const auto& frustum : frustums) {
                        if (frustum.intersects(aabb)) {
                            return true;
                        }
                    }
                    return false;
                });
            pass.light_index = light_idx;
            point_shadow_passes.push_back(pass);
        }
    }
}

void RenderData::update(const Scene* p_scene) {
    clear();
    scene = p_scene;

    point_light_draw_data();

    // cascaded shadow map
    for (int i = 0; i < MAX_CASCADE_COUNT; ++i) {
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

    // @TODO: cache pointers
    ImageHandle* point_light = AssetManager::singleton().find_image("@res://images/pointlight.png");
    DEV_ASSERT(point_light);
    Image* point_light_image = point_light->get();

    ImageHandle* omni_light = AssetManager::singleton().find_image("@res://images/omnilight.png");
    DEV_ASSERT(omni_light);
    Image* omni_light_image = omni_light->get();

    // lights
    light_billboards.clear();
    if (DVAR_GET_BOOL(show_editor)) {
        for (uint32_t idx = 0; idx < (uint32_t)scene->get_count<LightComponent>(); ++idx) {
            auto light_id = scene->get_entity<LightComponent>(idx);
            const LightComponent& light = scene->get_component_array<LightComponent>()[idx];
            const TransformComponent* transform = scene->get_component<TransformComponent>(light_id);
            DEV_ASSERT(transform);
            LightBillboard billboard;
            vec3 translation = transform->get_translation();
            switch (light.type) {
                case LIGHT_TYPE_OMNI:
                    billboard.image = omni_light_image;
                    break;
                case LIGHT_TYPE_POINT:
                    billboard.image = point_light_image;
                    break;
                default:
                    CRASH_NOW();
                    break;
            }
            billboard.transform = glm::translate(translation);
            billboard.type = light.type;

            light_billboards.emplace_back(billboard);
        }
    }
}

void RenderData::fill(const Scene* p_scene, const mat4& projection_view_matrix, Pass& pass, FilterObjectFunc1 func1, FilterObjectFunc2 func2) {
    scene = p_scene;
    pass.projection_view_matrix = projection_view_matrix;
    uint32_t num_objects = (uint32_t)scene->get_count<ObjectComponent>();
    for (uint32_t i = 0; i < num_objects; ++i) {
        ecs::Entity entity = scene->get_entity<ObjectComponent>(i);
        if (!scene->contains<TransformComponent>(entity)) {
            continue;
        }

        const ObjectComponent& obj = scene->get_component_array<ObjectComponent>()[i];
        // ????
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
            if (!func2(aabb)) {
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
