#include "render_graph_vxgi.h"

#include "core/base/rid_owner.h"
#include "core/math/frustum.h"
#include "rendering/rendering_dvars.h"

// @TODO: refactor
#include "core/framework/graphics_manager.h"
#include "core/framework/scene_manager.h"
#include "rendering/pipeline_state.h"
#include "rendering/r_cbuffers.h"
#include "rendering/render_data.h"

extern GpuTexture g_albedoVoxel;
extern GpuTexture g_normalVoxel;
extern MeshData g_box;
extern MeshData g_skybox;
extern MeshData g_billboard;

extern my::RIDAllocator<MeshData> g_meshes;

namespace my::rg {

// @TODO: refactor render passes to have multiple frame buffers
// const int shadow_res = DVAR_GET_INT(r_point_shadow_res);
void point_shadow_pass_func(int width, int height, int pass_id) {
    auto render_data = GraphicsManager::singleton().get_render_data();
    if (render_data->point_shadow_passes.size() <= pass_id) {
        return;
    }

    const Scene& scene = SceneManager::get_scene();
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_FRONT);
    glClear(GL_DEPTH_BUFFER_BIT);

    glViewport(0, 0, width, height);

    RenderData::Pass& pass = render_data->point_shadow_passes[pass_id];

    for (const auto& draw : pass.draws) {
        const bool has_bone = draw.armature_id.is_valid();

        if (has_bone) {
            auto& armature = *scene.get_component<ArmatureComponent>(draw.armature_id);
            DEV_ASSERT(armature.bone_transforms.size() <= MAX_BONE_COUNT);

            memcpy(g_boneCache.cache.c_bones, armature.bone_transforms.data(), sizeof(mat4) * armature.bone_transforms.size());
            g_boneCache.Update();
        }

        GraphicsManager::singleton().set_pipeline_state(has_bone ? PROGRAM_POINT_SHADOW_ANIMATED : PROGRAM_POINT_SHADOW_STATIC);

        g_perBatchCache.cache.c_projection_view_model_matrix = pass.projection_view_matrix * draw.world_matrix;
        g_perBatchCache.cache.c_model_matrix = draw.world_matrix;
        g_perBatchCache.cache.c_light_index = pass.light_index;
        g_perBatchCache.Update();

        glBindVertexArray(draw.mesh_data->vao);
        glDrawElements(GL_TRIANGLES, draw.mesh_data->count, GL_UNSIGNED_INT, 0);
    }
}

void shadow_pass_func(int width, int height) {
    // @TODO: for each light source, render shadow
    const Scene& scene = SceneManager::get_scene();

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_FRONT);
    glClear(GL_DEPTH_BUFFER_BIT);

    int actual_width = width / MAX_CASCADE_COUNT;
    auto render_data = GraphicsManager::singleton().get_render_data();

    for (int cascade_idx = 0; cascade_idx < MAX_CASCADE_COUNT; ++cascade_idx) {
        glViewport(cascade_idx * actual_width, 0, actual_width, height);

        RenderData::Pass& pass = render_data->shadow_passes[cascade_idx];

        for (const auto& draw : pass.draws) {
            const bool has_bone = draw.armature_id.is_valid();

            if (has_bone) {
                auto& armature = *scene.get_component<ArmatureComponent>(draw.armature_id);
                DEV_ASSERT(armature.bone_transforms.size() <= MAX_BONE_COUNT);

                memcpy(g_boneCache.cache.c_bones, armature.bone_transforms.data(), sizeof(mat4) * armature.bone_transforms.size());
                g_boneCache.Update();
            }

            GraphicsManager::singleton().set_pipeline_state(has_bone ? PROGRAM_DPETH_ANIMATED : PROGRAM_DPETH_STATIC);

            g_perBatchCache.cache.c_projection_view_model_matrix = pass.projection_view_matrix * draw.world_matrix;
            g_perBatchCache.cache.c_model_matrix = draw.world_matrix;
            g_perBatchCache.Update();

            glBindVertexArray(draw.mesh_data->vao);
            glDrawElements(GL_TRIANGLES, draw.mesh_data->count, GL_UNSIGNED_INT, 0);
        }

        glCullFace(GL_BACK);
        glUseProgram(0);
    }
}

void voxelization_pass_func(int width, int height) {
    unused(width);
    unused(height);
    if (!DVAR_GET_BOOL(r_enable_vxgi)) {
        return;
    }

    g_albedoVoxel.clear();
    g_normalVoxel.clear();

    const int voxel_size = DVAR_GET_INT(r_voxel_size);

    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    glViewport(0, 0, voxel_size, voxel_size);

    // @TODO: fix
    constexpr int IMAGE_VOXEL_ALBEDO_SLOT = 0;
    constexpr int IMAGE_VOXEL_NORMAL_SLOT = 1;
    g_albedoVoxel.bindImageTexture(IMAGE_VOXEL_ALBEDO_SLOT);
    g_normalVoxel.bindImageTexture(IMAGE_VOXEL_NORMAL_SLOT);

    auto render_data = GraphicsManager::singleton().get_render_data();
    RenderData::Pass& pass = render_data->voxel_pass;

    for (const auto& draw : pass.draws) {
        const bool has_bone = draw.armature_id.is_valid();

        if (has_bone) {
            auto& armature = *render_data->scene->get_component<ArmatureComponent>(draw.armature_id);
            DEV_ASSERT(armature.bone_transforms.size() <= MAX_BONE_COUNT);

            memcpy(g_boneCache.cache.c_bones, armature.bone_transforms.data(), sizeof(mat4) * armature.bone_transforms.size());
            g_boneCache.Update();
        }

        GraphicsManager::singleton().set_pipeline_state(has_bone ? PROGRAM_VOXELIZATION_ANIMATED : PROGRAM_VOXELIZATION_STATIC);

        g_perBatchCache.cache.c_projection_view_model_matrix = pass.projection_view_matrix * draw.world_matrix;
        g_perBatchCache.cache.c_model_matrix = draw.world_matrix;
        g_perBatchCache.Update();

        glBindVertexArray(draw.mesh_data->vao);

        for (const auto& subset : draw.subsets) {
            GraphicsManager::singleton().fill_material_constant_buffer(subset.material, g_materialCache.cache);
            g_materialCache.Update();

            glDrawElements(GL_TRIANGLES, subset.index_count, GL_UNSIGNED_INT, (void*)(subset.index_offset * sizeof(uint32_t)));
        }
    }

    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

    // post process
    GraphicsManager::singleton().set_pipeline_state(PROGRAM_VOXELIZATION_POST);

    constexpr GLuint workGroupX = 512;
    constexpr GLuint workGroupY = 512;
    const GLuint workGroupZ = (voxel_size * voxel_size * voxel_size) / (workGroupX * workGroupY);

    glDispatchCompute(workGroupX, workGroupY, workGroupZ);
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

    g_albedoVoxel.bind();
    g_albedoVoxel.genMipMap();
    g_normalVoxel.bind();
    g_normalVoxel.genMipMap();
}

void gbuffer_pass_func(int width, int height) {
    glViewport(0, 0, width, height);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    auto render_data = GraphicsManager::singleton().get_render_data();
    RenderData::Pass& pass = render_data->main_pass;

    for (const auto& draw : pass.draws) {
        const bool has_bone = draw.armature_id.is_valid();

        if (has_bone) {
            auto& armature = *render_data->scene->get_component<ArmatureComponent>(draw.armature_id);
            DEV_ASSERT(armature.bone_transforms.size() <= MAX_BONE_COUNT);

            memcpy(g_boneCache.cache.c_bones, armature.bone_transforms.data(), sizeof(mat4) * armature.bone_transforms.size());
            g_boneCache.Update();
        }

        GraphicsManager::singleton().set_pipeline_state(has_bone ? PROGRAM_GBUFFER_ANIMATED : PROGRAM_GBUFFER_STATIC);

        g_perBatchCache.cache.c_projection_view_model_matrix = pass.projection_view_matrix * draw.world_matrix;
        g_perBatchCache.cache.c_model_matrix = draw.world_matrix;
        g_perBatchCache.Update();

        glBindVertexArray(draw.mesh_data->vao);

        for (const auto& subset : draw.subsets) {
            GraphicsManager::singleton().fill_material_constant_buffer(subset.material, g_materialCache.cache);
            g_materialCache.Update();

            glDrawElements(GL_TRIANGLES, subset.index_count, GL_UNSIGNED_INT, (void*)(subset.index_offset * sizeof(uint32_t)));
        }
    }

    glUseProgram(0);
}

void ssao_pass_func(int width, int height) {
    glViewport(0, 0, width, height);

    GraphicsManager::singleton().set_pipeline_state(PROGRAM_SSAO);
    glClear(GL_COLOR_BUFFER_BIT);
    R_DrawQuad();
}

void lighting_pass_func(int width, int height) {
    glViewport(0, 0, width, height);
    // glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glClear(GL_COLOR_BUFFER_BIT);
    GraphicsManager::singleton().set_pipeline_state(PROGRAM_LIGHTING_VXGI);
    R_DrawQuad();
}

void debug_vxgi_pass_func(int width, int height) {
    glViewport(0, 0, width, height);
    glEnable(GL_DEPTH_TEST);
    // glClear(GL_COLOR_BUFFER_BIT);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    GraphicsManager::singleton().set_pipeline_state(PROGRAM_DEBUG_VOXEL);

    glBindVertexArray(g_box.vao);

    const int size = DVAR_GET_INT(r_voxel_size);
    glDrawElementsInstanced(GL_TRIANGLES, g_box.count, GL_UNSIGNED_INT, 0, size * size * size);
}

void fxaa_pass_func(int width, int height) {
    // HACK:
    if (DVAR_GET_BOOL(r_debug_vxgi)) {
        debug_vxgi_pass_func(width, height);
    } else {
        glViewport(0, 0, width, height);
        glClear(GL_COLOR_BUFFER_BIT);

        // glDisable(GL_DEPTH_TEST);
        GraphicsManager::singleton().set_pipeline_state(PROGRAM_FXAA);
        R_DrawQuad();
    }

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    // @TODO: draw gui stuff to the anti-aliased
    {
        // @DEBUG SKYBOX
        glEnable(GL_CULL_FACE);
        glCullFace(GL_FRONT);
        GraphicsManager::singleton().set_pipeline_state(PROGRAM_SKY_BOX);
        glBindVertexArray(g_skybox.vao);
        glDrawElementsInstanced(GL_TRIANGLES, g_skybox.count, GL_UNSIGNED_INT, 0, 1);
        glDisable(GL_CULL_FACE);
    }

    GraphicsManager::singleton().set_pipeline_state(PROGRAM_BILLBOARD);
    // draw billboards
    auto render_data = GraphicsManager::singleton().get_render_data();

    for (const auto& light : render_data->light_billboards) {
        // @TODO: sort same materials
        g_perBatchCache.cache.c_model_matrix = light.transform;
        g_perBatchCache.Update();
        g_materialCache.cache.c_albedo_map = light.image ? light.image->texture.resident_handle : 0;
        g_materialCache.Update();

        glBindVertexArray(g_billboard.vao);
        glDrawElementsInstanced(GL_TRIANGLES, g_billboard.count, GL_UNSIGNED_INT, 0, 1);
    }

    // glDepthFunc(GL_LESS);
    glUseProgram(0);
}

void create_render_graph_vxgi(RenderGraph& graph) {
    ivec2 frame_size = DVAR_GET_IVEC2(resolution);
    int w = frame_size.x;
    int h = frame_size.y;

    GraphicsManager& manager = GraphicsManager::singleton();

    auto gbuffer_attachment0 = manager.create_resource(RenderTargetDesc{ RT_RES_GBUFFER_POSITION,
                                                                         FORMAT_R16G16B16A16_FLOAT,
                                                                         RT_COLOR_ATTACHMENT,
                                                                         w, h });
    auto gbuffer_attachment1 = manager.create_resource(RenderTargetDesc{ RT_RES_GBUFFER_NORMAL,
                                                                         FORMAT_R16G16B16A16_FLOAT,
                                                                         RT_COLOR_ATTACHMENT,
                                                                         w, h });
    auto gbuffer_attachment2 = manager.create_resource(RenderTargetDesc{ RT_RES_GBUFFER_BASE_COLOR,
                                                                         FORMAT_R8G8B8A8_UINT,
                                                                         RT_COLOR_ATTACHMENT,
                                                                         w, h });
    auto gbuffer_depth = manager.create_resource(RenderTargetDesc{ RT_RES_GBUFFER_DEPTH,
                                                                   FORMAT_D32_FLOAT,
                                                                   RT_DEPTH_ATTACHMENT,
                                                                   w, h });
    auto ssao_attachment = manager.create_resource(RenderTargetDesc{ RT_RES_SSAO,
                                                                     FORMAT_R32_FLOAT,
                                                                     RT_COLOR_ATTACHMENT,
                                                                     w, h });
    auto lighting_attachment = manager.create_resource(RenderTargetDesc{ RT_RES_LIGHTING,
                                                                         FORMAT_R8G8B8A8_UINT,
                                                                         RT_COLOR_ATTACHMENT,
                                                                         w, h });
    auto fxaa_attachment = manager.create_resource(RenderTargetDesc{ RT_RES_FXAA,
                                                                     FORMAT_R8G8B8A8_UINT,
                                                                     RT_COLOR_ATTACHMENT,
                                                                     w, h });
    {  // shadow pass
        const int shadow_res = DVAR_GET_INT(r_shadow_res);
        DEV_ASSERT(math::is_power_of_two(shadow_res));
        const int point_shadow_res = DVAR_GET_INT(r_point_shadow_res);
        DEV_ASSERT(math::is_power_of_two(point_shadow_res));

        auto shadow_map = manager.create_resource(RenderTargetDesc{ RT_RES_SHADOW_MAP,
                                                                    FORMAT_D32_FLOAT,
                                                                    RT_SHADOW_MAP,
                                                                    MAX_CASCADE_COUNT * shadow_res, shadow_res });
        RenderPassDesc desc;
        desc.name = SHADOW_PASS;

        RenderPassFunc funcs[] = {
            [](int width, int height) {
                point_shadow_pass_func(width, height, 0);
            },
            [](int width, int height) {
                point_shadow_pass_func(width, height, 1);
            },
            [](int width, int height) {
                point_shadow_pass_func(width, height, 2);
            },
            [](int width, int height) {
                point_shadow_pass_func(width, height, 3);
            },
        };

        static_assert(array_length(funcs) == MAX_LIGHT_CAST_SHADOW_COUNT);

        for (int i = 0; i < MAX_LIGHT_CAST_SHADOW_COUNT; ++i) {
            auto point_shadow_map = manager.create_resource(RenderTargetDesc{ RT_RES_POINT_SHADOW_MAP + std::to_string(i),
                                                                              FORMAT_D32_FLOAT,
                                                                              RT_SHADOW_CUBE_MAP,
                                                                              point_shadow_res, point_shadow_res });

            desc.subpasses.emplace_back(SubPassDesc{
                .depth_attachment = point_shadow_map,
                .func = funcs[i],
            });
        }

        desc.subpasses.emplace_back(SubPassDesc{
            .depth_attachment = shadow_map,
            .func = shadow_pass_func,
        });
        graph.add_pass(desc);
    }
    {  // gbuffer pass
        RenderPassDesc desc;
        desc.name = GBUFFER_PASS;
        desc.subpasses.emplace_back(SubPassDesc{
            .color_attachments = { gbuffer_attachment0, gbuffer_attachment1, gbuffer_attachment2 },
            .depth_attachment = gbuffer_depth,
            .func = gbuffer_pass_func,
        });
        graph.add_pass(desc);
    }
    {  // voxel pass
        RenderPassDesc desc;
        desc.name = VOXELIZATION_PASS;
        desc.dependencies = { SHADOW_PASS };
        desc.subpasses.emplace_back(SubPassDesc{
            .func = voxelization_pass_func,
        });
        graph.add_pass(desc);
    }
    {  // ssao pass
        RenderPassDesc desc;
        desc.name = SSAO_PASS;
        desc.dependencies = { GBUFFER_PASS };
        desc.subpasses.emplace_back(SubPassDesc{
            .color_attachments = { ssao_attachment },
            .func = ssao_pass_func,
        });
        graph.add_pass(desc);
    }
    {  // lighting pass
        RenderPassDesc desc;
        desc.name = LIGHTING_PASS;
        desc.dependencies = { SHADOW_PASS, GBUFFER_PASS, SSAO_PASS, VOXELIZATION_PASS };
        desc.subpasses.emplace_back(SubPassDesc{
            .color_attachments = { lighting_attachment },
            .func = lighting_pass_func,
        });
        graph.add_pass(desc);
    }
    {  // fxaa pass
        RenderPassDesc desc;
        desc.name = FXAA_PASS;
        desc.dependencies = { LIGHTING_PASS };
        desc.subpasses.emplace_back(SubPassDesc{
            .color_attachments = { fxaa_attachment },
            .depth_attachment = gbuffer_depth,
            .func = fxaa_pass_func,
        });
        graph.add_pass(desc);
    }

    // @TODO: allow recompile
    graph.compile();
}

}  // namespace my::rg
