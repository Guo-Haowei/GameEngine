#include "render_graph_vxgi.h"

#include "core/base/rid_owner.h"
#include "core/debugger/profiler.h"
#include "core/framework/graphics_manager.h"
#include "core/framework/scene_manager.h"
#include "core/math/frustum.h"
#include "rendering/pipeline_state.h"
#include "rendering/render_data.h"
#include "rendering/rendering_dvars.h"
#include "rendering/rendering_misc.h"

// @TODO: refactor
#include "rendering/r_cbuffers.h"
extern GpuTexture g_albedoVoxel;
extern GpuTexture g_normalVoxel;
extern MeshData g_box;
extern MeshData g_skybox;
extern MeshData g_billboard;

extern my::RIDAllocator<MeshData> g_meshes;

namespace my::rg {

// @TODO: refactor render passes to have multiple frame buffers
// const int shadow_res = DVAR_GET_INT(r_point_shadow_res);
void point_shadow_pass_func(const Subpass* p_subpass, int p_pass_id) {
    OPTICK_EVENT();

    auto render_data = GraphicsManager::singleton().get_render_data();
    if (render_data->point_shadow_passes.size() <= p_pass_id) {
        return;
    }

    // prepare render data
    p_subpass->set_render_target();
    auto [width, height] = p_subpass->depth_attachment->get_size();
    glViewport(0, 0, width, height);

    // @TODO: fix this
    const Scene& scene = SceneManager::get_scene();
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_FRONT);
    glClear(GL_DEPTH_BUFFER_BIT);

    RenderData::Pass& pass = render_data->point_shadow_passes[p_pass_id];

    pass.fill_perpass(g_per_pass_cache.cache);
    g_per_pass_cache.Update();

    for (const auto& draw : pass.draws) {
        const bool has_bone = draw.armature_id.is_valid();

        if (has_bone) {
            auto& armature = *scene.get_component<ArmatureComponent>(draw.armature_id);
            DEV_ASSERT(armature.bone_transforms.size() <= MAX_BONE_COUNT);

            memcpy(g_boneCache.cache.c_bones, armature.bone_transforms.data(), sizeof(mat4) * armature.bone_transforms.size());
            g_boneCache.Update();
        }

        GraphicsManager::singleton().set_pipeline_state(has_bone ? PROGRAM_POINT_SHADOW_ANIMATED : PROGRAM_POINT_SHADOW_STATIC);

        g_perBatchCache.cache.c_model_matrix = draw.world_matrix;
        g_perBatchCache.cache.c_light_index = pass.light_index;
        g_perBatchCache.Update();

        glBindVertexArray(draw.mesh_data->vao);
        glDrawElements(GL_TRIANGLES, draw.mesh_data->count, GL_UNSIGNED_INT, 0);
    }
}

void shadow_pass_func(const Subpass* p_subpass) {
    OPTICK_EVENT();

    p_subpass->set_render_target();
    auto [width, height] = p_subpass->depth_attachment->get_size();

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
        pass.fill_perpass(g_per_pass_cache.cache);
        g_per_pass_cache.Update();

        for (const auto& draw : pass.draws) {
            const bool has_bone = draw.armature_id.is_valid();

            if (has_bone) {
                auto& armature = *scene.get_component<ArmatureComponent>(draw.armature_id);
                DEV_ASSERT(armature.bone_transforms.size() <= MAX_BONE_COUNT);

                memcpy(g_boneCache.cache.c_bones, armature.bone_transforms.data(), sizeof(mat4) * armature.bone_transforms.size());
                g_boneCache.Update();
            }

            GraphicsManager::singleton().set_pipeline_state(has_bone ? PROGRAM_DPETH_ANIMATED : PROGRAM_DPETH_STATIC);

            g_perBatchCache.cache.c_model_matrix = draw.world_matrix;
            g_perBatchCache.Update();

            glBindVertexArray(draw.mesh_data->vao);
            glDrawElements(GL_TRIANGLES, draw.mesh_data->count, GL_UNSIGNED_INT, 0);
        }

        glCullFace(GL_BACK);
        glUseProgram(0);
    }
}

void voxelization_pass_func(const Subpass*) {
    OPTICK_EVENT();

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
    pass.fill_perpass(g_per_pass_cache.cache);
    g_per_pass_cache.Update();

    for (const auto& draw : pass.draws) {
        const bool has_bone = draw.armature_id.is_valid();

        if (has_bone) {
            auto& armature = *render_data->scene->get_component<ArmatureComponent>(draw.armature_id);
            DEV_ASSERT(armature.bone_transforms.size() <= MAX_BONE_COUNT);

            memcpy(g_boneCache.cache.c_bones, armature.bone_transforms.data(), sizeof(mat4) * armature.bone_transforms.size());
            g_boneCache.Update();
        }

        GraphicsManager::singleton().set_pipeline_state(has_bone ? PROGRAM_VOXELIZATION_ANIMATED : PROGRAM_VOXELIZATION_STATIC);

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

static void draw_skybox() {
    glBindVertexArray(g_skybox.vao);
    glDrawElementsInstanced(GL_TRIANGLES, g_skybox.count, GL_UNSIGNED_INT, 0, 1);
}

void diffuse_irradiance_pass_func(const Subpass* p_subpass) {
    OPTICK_EVENT();

    auto [width, height] = p_subpass->depth_attachment->get_size();

    mat4 projection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
    auto view_matrices = cube_map_view_matrices(vec3(0));
    for (int i = 0; i < 6; ++i) {
        p_subpass->set_render_target(i);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glViewport(0, 0, width, height);

        GraphicsManager::singleton().set_pipeline_state(PROGRAM_ENV_SKYBOX_TO_CUBE_MAP);

        g_per_pass_cache.cache.c_projection_matrix = projection;
        g_per_pass_cache.cache.c_view_matrix = view_matrices[i];
        g_per_pass_cache.cache.c_projection_view_matrix = projection * view_matrices[i];
        g_per_pass_cache.Update();
        draw_skybox();
    }
}

void gbuffer_pass_func(const Subpass* p_subpass) {
    OPTICK_EVENT();

    p_subpass->set_render_target();
    auto [width, height] = p_subpass->depth_attachment->get_size();

    glViewport(0, 0, width, height);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    auto render_data = GraphicsManager::singleton().get_render_data();
    RenderData::Pass& pass = render_data->main_pass;

    pass.fill_perpass(g_per_pass_cache.cache);
    g_per_pass_cache.Update();

    for (const auto& draw : pass.draws) {
        const bool has_bone = draw.armature_id.is_valid();

        if (has_bone) {
            auto& armature = *render_data->scene->get_component<ArmatureComponent>(draw.armature_id);
            DEV_ASSERT(armature.bone_transforms.size() <= MAX_BONE_COUNT);

            memcpy(g_boneCache.cache.c_bones, armature.bone_transforms.data(), sizeof(mat4) * armature.bone_transforms.size());
            g_boneCache.Update();
        }

        GraphicsManager::singleton().set_pipeline_state(has_bone ? PROGRAM_GBUFFER_ANIMATED : PROGRAM_GBUFFER_STATIC);

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

void ssao_pass_func(const Subpass* p_subpass) {
    OPTICK_EVENT();

    p_subpass->set_render_target();
    DEV_ASSERT(!p_subpass->color_attachments.empty());
    auto depth_buffer = p_subpass->depth_attachment;
    auto [width, height] = p_subpass->color_attachments[0]->get_size();

    glViewport(0, 0, width, height);

    GraphicsManager::singleton().set_pipeline_state(PROGRAM_SSAO);
    glClear(GL_COLOR_BUFFER_BIT);
    R_DrawQuad();
}

// @TODO: refactor
static void fill_camera_matrices(PerPassConstantBuffer& buffer) {
    auto camera = SceneManager::singleton().get_scene().m_camera;
    buffer.c_projection_view_matrix = camera->get_projection_view_matrix();
    buffer.c_view_matrix = camera->get_view_matrix();
    buffer.c_projection_matrix = camera->get_projection_matrix();
}

void lighting_pass_func(const Subpass* p_subpass) {
    OPTICK_EVENT();

    p_subpass->set_render_target();
    DEV_ASSERT(!p_subpass->color_attachments.empty());
    auto depth_buffer = p_subpass->depth_attachment;
    auto [width, height] = p_subpass->color_attachments[0]->get_size();

    glViewport(0, 0, width, height);
    // glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glClear(GL_COLOR_BUFFER_BIT);
    GraphicsManager::singleton().set_pipeline_state(PROGRAM_LIGHTING_VXGI);

    // @TODO: fix
    auto camera = SceneManager::singleton().get_scene().m_camera;
    fill_camera_matrices(g_per_pass_cache.cache);
    g_per_pass_cache.Update();

    R_DrawQuad();
}

void debug_vxgi_pass_func(const Subpass* p_subpass) {
    OPTICK_EVENT();

    p_subpass->set_render_target();
    DEV_ASSERT(!p_subpass->color_attachments.empty());
    auto depth_buffer = p_subpass->depth_attachment;
    auto [width, height] = p_subpass->color_attachments[0]->get_size();

    glViewport(0, 0, width, height);
    glEnable(GL_DEPTH_TEST);
    // glClear(GL_COLOR_BUFFER_BIT);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    GraphicsManager::singleton().set_pipeline_state(PROGRAM_DEBUG_VOXEL);

    // @TODO: fix
    auto camera = SceneManager::singleton().get_scene().m_camera;
    fill_camera_matrices(g_per_pass_cache.cache);
    g_per_pass_cache.Update();

    glBindVertexArray(g_box.vao);

    const int size = DVAR_GET_INT(r_voxel_size);
    glDrawElementsInstanced(GL_TRIANGLES, g_box.count, GL_UNSIGNED_INT, 0, size * size * size);
}

void fxaa_pass_func(const Subpass* p_subpass) {
    OPTICK_EVENT();

    p_subpass->set_render_target();
    DEV_ASSERT(!p_subpass->color_attachments.empty());
    auto depth_buffer = p_subpass->depth_attachment;
    auto [width, height] = p_subpass->color_attachments[0]->get_size();

    // HACK:
    if (DVAR_GET_BOOL(r_debug_vxgi)) {
        debug_vxgi_pass_func(p_subpass);
    } else {
        glViewport(0, 0, width, height);
        glClear(GL_COLOR_BUFFER_BIT);

        // glDisable(GL_DEPTH_TEST);
        GraphicsManager::singleton().set_pipeline_state(PROGRAM_FXAA);
        R_DrawQuad();
    }

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    // draw skybox here
    {
        GraphicsManager::singleton().set_pipeline_state(PROGRAM_ENV_SKYBOX);
        draw_skybox();
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

// @TODO: refactor
static void debug_draw_quad(uint64_t p_handle, int p_channel, int p_screen_width, int p_screen_height, int p_width, int p_height) {
    float half_width_ndc = (float)p_width / p_screen_width;
    float half_height_ndc = (float)p_height / p_screen_height;

    vec2 size = vec2(half_width_ndc, half_height_ndc);
    vec2 pos;
    pos.x = 1.0f - half_width_ndc;
    pos.y = 1.0f - half_height_ndc;

    g_debug_draw_cache.cache.c_debug_draw_size = size;
    g_debug_draw_cache.cache.c_debug_draw_pos = pos;
    g_debug_draw_cache.cache.c_display_channel = p_channel;
    g_debug_draw_cache.cache.c_debug_draw_map = p_handle;
    g_debug_draw_cache.Update();
    R_DrawQuad();
}

void final_pass_func(const Subpass* p_subpass) {
    OPTICK_EVENT();

    p_subpass->set_render_target();
    DEV_ASSERT(!p_subpass->color_attachments.empty());
    auto [width, height] = p_subpass->color_attachments[0]->get_size();

    glViewport(0, 0, width, height);
    glDisable(GL_DEPTH_TEST);
    glClear(GL_COLOR_BUFFER_BIT);

    GraphicsManager::singleton().set_pipeline_state(PROGRAM_IMAGE_2D);

    // @TODO: clean up
    auto final_image_handle = GraphicsManager::singleton().find_resource(RT_RES_FXAA)->get_resident_handle();
    debug_draw_quad(final_image_handle, DISPLAY_CHANNEL_RGB, width, height, width, height);

#if 1
    auto shadow_map_handle = GraphicsManager::singleton().find_resource(RT_RES_SHADOW_MAP)->get_resident_handle();
    debug_draw_quad(shadow_map_handle, DISPLAY_CHANNEL_RRR, width, height, 800, 200);
#endif
}

void create_render_graph_vxgi(RenderGraph& graph) {
    ivec2 frame_size = DVAR_GET_IVEC2(resolution);
    int w = frame_size.x;
    int h = frame_size.y;

    GraphicsManager& manager = GraphicsManager::singleton();

    auto gbuffer_attachment0 = manager.create_resource(RenderTargetDesc{ RT_RES_GBUFFER_POSITION,
                                                                         FORMAT_R16G16B16A16_FLOAT,
                                                                         RT_COLOR_ATTACHMENT_2D,
                                                                         w, h });
    auto gbuffer_attachment1 = manager.create_resource(RenderTargetDesc{ RT_RES_GBUFFER_NORMAL,
                                                                         FORMAT_R16G16B16A16_FLOAT,
                                                                         RT_COLOR_ATTACHMENT_2D,
                                                                         w, h });
    auto gbuffer_attachment2 = manager.create_resource(RenderTargetDesc{ RT_RES_GBUFFER_BASE_COLOR,
                                                                         FORMAT_R8G8B8A8_UINT,
                                                                         RT_COLOR_ATTACHMENT_2D,
                                                                         w, h });
    auto gbuffer_depth = manager.create_resource(RenderTargetDesc{ RT_RES_GBUFFER_DEPTH,
                                                                   FORMAT_D32_FLOAT,
                                                                   RT_DEPTH_ATTACHMENT_2D,
                                                                   w, h });
    auto ssao_attachment = manager.create_resource(RenderTargetDesc{ RT_RES_SSAO,
                                                                     FORMAT_R32_FLOAT,
                                                                     RT_COLOR_ATTACHMENT_2D,
                                                                     w, h });
    auto lighting_attachment = manager.create_resource(RenderTargetDesc{ RT_RES_LIGHTING,
                                                                         FORMAT_R8G8B8A8_UINT,
                                                                         RT_COLOR_ATTACHMENT_2D,
                                                                         w, h });
    auto fxaa_attachment = manager.create_resource(RenderTargetDesc{ RT_RES_FXAA,
                                                                     FORMAT_R8G8B8A8_UINT,
                                                                     RT_COLOR_ATTACHMENT_2D,
                                                                     w, h });
    auto final_attachment = manager.create_resource(RenderTargetDesc{ RT_RES_FINAL,
                                                                      FORMAT_R8G8B8A8_UINT,
                                                                      RT_COLOR_ATTACHMENT_2D,
                                                                      w, h });

    {  // environment pass
        RenderPassDesc desc;
        desc.name = ENV_PASS;
        auto pass = graph.create_pass(desc);

        constexpr int environment_map_size = 512;
        int size = environment_map_size;
        auto irradiance_map = manager.create_resource(RenderTargetDesc{ RT_ENV_SKYBOX_CUBE_MAP,
                                                                        FORMAT_R16G16B16_FLOAT,
                                                                        RT_COLOR_ATTACHMENT_CUBE_MAP,
                                                                        size, size });
        auto irradiance_depth = manager.create_resource(RenderTargetDesc{ RT_ENV_SKYBOX_DEPTH,
                                                                          FORMAT_D32_FLOAT,
                                                                          RT_DEPTH_ATTACHMENT_2D,
                                                                          size, size });

        auto subpass = manager.create_subpass(SubpassDesc{
            .color_attachments = { irradiance_map },
            .depth_attachment = irradiance_depth,
            .func = diffuse_irradiance_pass_func,
        });
        pass->add_sub_pass(subpass);
    }

    {  // shadow pass
        const int shadow_res = DVAR_GET_INT(r_shadow_res);
        DEV_ASSERT(math::is_power_of_two(shadow_res));
        const int point_shadow_res = DVAR_GET_INT(r_point_shadow_res);
        DEV_ASSERT(math::is_power_of_two(point_shadow_res));

        auto shadow_map = manager.create_resource(RenderTargetDesc{ RT_RES_SHADOW_MAP,
                                                                    FORMAT_D32_FLOAT,
                                                                    RT_SHADOW_2D,
                                                                    MAX_CASCADE_COUNT * shadow_res, shadow_res });
        RenderPassDesc desc;
        desc.name = SHADOW_PASS;
        auto pass = graph.create_pass(desc);

        // @TODO: refactor
        SubPassFunc funcs[] = {
            [](const Subpass* p_subpass) {
                point_shadow_pass_func(p_subpass, 0);
            },
            [](const Subpass* p_subpass) {
                point_shadow_pass_func(p_subpass, 1);
            },
            [](const Subpass* p_subpass) {
                point_shadow_pass_func(p_subpass, 2);
            },
            [](const Subpass* p_subpass) {
                point_shadow_pass_func(p_subpass, 3);
            },
        };

        static_assert(array_length(funcs) == MAX_LIGHT_CAST_SHADOW_COUNT);

        for (int i = 0; i < MAX_LIGHT_CAST_SHADOW_COUNT; ++i) {
            auto point_shadow_map = manager.create_resource(RenderTargetDesc{ RT_RES_POINT_SHADOW_MAP + std::to_string(i),
                                                                              FORMAT_D32_FLOAT,
                                                                              RT_SHADOW_CUBE_MAP,
                                                                              point_shadow_res, point_shadow_res });

            auto subpass = manager.create_subpass(SubpassDesc{
                .depth_attachment = point_shadow_map,
                .func = funcs[i],
            });
            pass->add_sub_pass(subpass);
        }

        auto subpass = manager.create_subpass(SubpassDesc{
            .depth_attachment = shadow_map,
            .func = shadow_pass_func,
        });
        pass->add_sub_pass(subpass);
    }
    {  // gbuffer pass
        RenderPassDesc desc;
        desc.name = GBUFFER_PASS;
        auto pass = graph.create_pass(desc);
        auto subpass = manager.create_subpass(SubpassDesc{
            .color_attachments = { gbuffer_attachment0, gbuffer_attachment1, gbuffer_attachment2 },
            .depth_attachment = gbuffer_depth,
            .func = gbuffer_pass_func,
        });
        pass->add_sub_pass(subpass);
    }
    {  // voxel pass
        RenderPassDesc desc;
        desc.name = VOXELIZATION_PASS;
        desc.dependencies = { SHADOW_PASS };
        auto pass = graph.create_pass(desc);
        auto subpass = manager.create_subpass(SubpassDesc{
            .func = voxelization_pass_func,
        });
        pass->add_sub_pass(subpass);
    }
    {  // ssao pass
        RenderPassDesc desc;
        desc.name = SSAO_PASS;
        desc.dependencies = { GBUFFER_PASS };
        auto pass = graph.create_pass(desc);
        auto subpass = manager.create_subpass(SubpassDesc{
            .color_attachments = { ssao_attachment },
            .func = ssao_pass_func,
        });
        pass->add_sub_pass(subpass);
    }
    {  // lighting pass
        RenderPassDesc desc;
        desc.name = LIGHTING_PASS;
        desc.dependencies = { SHADOW_PASS, GBUFFER_PASS, SSAO_PASS, VOXELIZATION_PASS, ENV_PASS };
        auto pass = graph.create_pass(desc);
        auto subpass = manager.create_subpass(SubpassDesc{
            .color_attachments = { lighting_attachment },
            .func = lighting_pass_func,
        });
        pass->add_sub_pass(subpass);
    }
    {  // fxaa pass
        RenderPassDesc desc;
        desc.name = FXAA_PASS;
        desc.dependencies = { LIGHTING_PASS };
        auto pass = graph.create_pass(desc);
        auto subpass = manager.create_subpass(SubpassDesc{
            .color_attachments = { fxaa_attachment },
            .depth_attachment = gbuffer_depth,
            .func = fxaa_pass_func,
        });
        pass->add_sub_pass(subpass);
    }
    {
        // final pass
        RenderPassDesc desc;
        desc.name = FINAL_PASS;
        desc.dependencies = { FXAA_PASS };
        auto pass = graph.create_pass(desc);
        auto subpass = manager.create_subpass(SubpassDesc{
            .color_attachments = { final_attachment },
            .func = final_pass_func,
        });
        pass->add_sub_pass(subpass);
    }

    // @TODO: allow recompile
    graph.compile();
}

}  // namespace my::rg
