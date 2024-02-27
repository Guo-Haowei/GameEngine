#include "core/debugger/profiler.h"
#include "core/framework/graphics_manager.h"
#include "core/framework/scene_manager.h"
#include "core/math/frustum.h"
#include "render_graphs.h"
#include "rendering/pipeline_state.h"
#include "rendering/render_data.h"
#include "rendering/renderer.h"
#include "rendering/rendering_dvars.h"

// @TODO: refactor
#include "drivers/opengl/opengl_graphics_manager.h"
#include "rendering/r_cbuffers.h"
extern GpuTexture g_albedoVoxel;
extern GpuTexture g_normalVoxel;
extern OpenGLMeshBuffers g_box;
extern OpenGLMeshBuffers g_skybox;
extern OpenGLMeshBuffers g_billboard;

namespace my::rg {

// @TODO: refactor render passes to have multiple frame buffers
// const int shadow_res = DVAR_GET_INT(r_point_shadow_res);
void point_shadow_pass_func(const Subpass* p_subpass, int p_pass_id) {
    OPTICK_EVENT();

    auto& gm = GraphicsManager::singleton();
    auto render_data = gm.get_render_data();
    if (render_data->point_shadow_passes.size() <= p_pass_id) {
        return;
    }

    // prepare render data
    gm.set_render_target(p_subpass);
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

        gm.set_pipeline_state(has_bone ? PROGRAM_POINT_SHADOW_ANIMATED : PROGRAM_POINT_SHADOW_STATIC);

        g_perBatchCache.cache.c_model_matrix = draw.world_matrix;
        g_perBatchCache.cache.c_light_index = pass.light_index;
        g_perBatchCache.Update();

        gm.set_mesh(draw.mesh_data);
        gm.draw_elements(draw.mesh_data->index_count);
    }
}

void shadow_pass_func(const Subpass* p_subpass) {
    OPTICK_EVENT();

    auto& gm = GraphicsManager::singleton();
    gm.set_render_target(p_subpass);
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

            gm.set_pipeline_state(has_bone ? PROGRAM_DPETH_ANIMATED : PROGRAM_DPETH_STATIC);

            g_perBatchCache.cache.c_model_matrix = draw.world_matrix;
            g_perBatchCache.Update();

            gm.set_mesh(draw.mesh_data);
            gm.draw_elements(draw.mesh_data->index_count);
        }

        glCullFace(GL_BACK);
        glUseProgram(0);
    }
}

void voxelization_pass_func(const Subpass*) {
    OPTICK_EVENT();
    auto& gm = GraphicsManager::singleton();

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

    auto render_data = gm.get_render_data();
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

        gm.set_pipeline_state(has_bone ? PROGRAM_VOXELIZATION_ANIMATED : PROGRAM_VOXELIZATION_STATIC);

        g_perBatchCache.cache.c_model_matrix = draw.world_matrix;
        g_perBatchCache.Update();

        gm.set_mesh(draw.mesh_data);

        for (const auto& subset : draw.subsets) {
            GraphicsManager::singleton().fill_material_constant_buffer(subset.material, g_materialCache.cache);
            g_materialCache.Update();

            gm.draw_elements(subset.index_count, subset.index_offset);
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

    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
}

static void draw_cube_map() {
    glBindVertexArray(g_skybox.vao);
    glDrawElementsInstanced(GL_TRIANGLES, g_skybox.index_count, GL_UNSIGNED_INT, 0, 1);
}

// @TODO: fix

void hdr_to_cube_map_pass_func(const Subpass* p_subpass) {
    OPTICK_EVENT();

    if (!renderer::need_update_env()) {
        return;
    }

    GraphicsManager::singleton().set_pipeline_state(PROGRAM_ENV_SKYBOX_TO_CUBE_MAP);
    auto cube_map = p_subpass->color_attachments[0];
    auto [width, height] = cube_map->get_size();

    mat4 projection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
    auto view_matrices = renderer::cube_map_view_matrices(vec3(0));
    for (int i = 0; i < 6; ++i) {
        GraphicsManager::singleton().set_render_target(p_subpass, i);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glViewport(0, 0, width, height);

        g_per_pass_cache.cache.c_projection_matrix = projection;
        g_per_pass_cache.cache.c_view_matrix = view_matrices[i];
        g_per_pass_cache.cache.c_projection_view_matrix = projection * view_matrices[i];
        g_per_pass_cache.Update();
        draw_cube_map();
    }

    glBindTexture(GL_TEXTURE_CUBE_MAP, cube_map->texture->get_handle());
    glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
}

// @TODO: refactor
void generate_brdf_func(const Subpass* p_subpass) {
    OPTICK_EVENT();

    if (!renderer::need_update_env()) {
        return;
    }

    GraphicsManager::singleton().set_pipeline_state(PROGRAM_BRDF);
    auto [width, height] = p_subpass->color_attachments[0]->get_size();
    GraphicsManager::singleton().set_render_target(p_subpass);
    glClear(GL_COLOR_BUFFER_BIT);
    glViewport(0, 0, width, height);
    glDisable(GL_DEPTH_TEST);
    R_DrawQuad();
    glEnable(GL_DEPTH_TEST);
}

void diffuse_irradiance_pass_func(const Subpass* p_subpass) {
    OPTICK_EVENT();
    if (!renderer::need_update_env()) {
        return;
    }

    GraphicsManager::singleton().set_pipeline_state(PROGRAM_DIFFUSE_IRRADIANCE);
    auto [width, height] = p_subpass->depth_attachment->get_size();

    mat4 projection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
    auto view_matrices = renderer::cube_map_view_matrices(vec3(0));

    for (int i = 0; i < 6; ++i) {
        GraphicsManager::singleton().set_render_target(p_subpass, i);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glViewport(0, 0, width, height);

        g_per_pass_cache.cache.c_projection_matrix = projection;
        g_per_pass_cache.cache.c_view_matrix = view_matrices[i];
        g_per_pass_cache.cache.c_projection_view_matrix = projection * view_matrices[i];
        g_per_pass_cache.Update();
        draw_cube_map();
    }
}

void prefilter_pass_func(const Subpass* p_subpass) {
    OPTICK_EVENT();
    if (!renderer::need_update_env()) {
        return;
    }

    GraphicsManager::singleton().set_pipeline_state(PROGRAM_PREFILTER);
    auto [width, height] = p_subpass->depth_attachment->get_size();

    mat4 projection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
    auto view_matrices = renderer::cube_map_view_matrices(vec3(0));
    const uint32_t max_mip_levels = 5;

    for (int mip_idx = 0; mip_idx < max_mip_levels; ++mip_idx, width /= 2, height /= 2) {
        for (int face_id = 0; face_id < 6; ++face_id) {
            g_per_pass_cache.cache.c_projection_matrix = projection;
            g_per_pass_cache.cache.c_view_matrix = view_matrices[face_id];
            g_per_pass_cache.cache.c_projection_view_matrix = projection * view_matrices[face_id];
            g_per_pass_cache.cache.c_per_pass_roughness = (float)mip_idx / (float)(max_mip_levels - 1);
            g_per_pass_cache.Update();

            GraphicsManager::singleton().set_render_target(p_subpass, face_id, mip_idx);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            glViewport(0, 0, width, height);
            draw_cube_map();
        }
    }

    return;
}

void gbuffer_pass_func(const Subpass* p_subpass) {
    OPTICK_EVENT();

    auto& gm = GraphicsManager::singleton();
    gm.set_render_target(p_subpass);
    auto [width, height] = p_subpass->depth_attachment->get_size();

    glViewport(0, 0, width, height);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    auto render_data = gm.get_render_data();
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

        gm.set_mesh(draw.mesh_data);

        for (const auto& subset : draw.subsets) {
            GraphicsManager::singleton().fill_material_constant_buffer(subset.material, g_materialCache.cache);
            g_materialCache.Update();

            gm.draw_elements(subset.index_count, subset.index_offset);
        }
    }

    glUseProgram(0);
}

void ssao_pass_func(const Subpass* p_subpass) {
    OPTICK_EVENT();

    GraphicsManager::singleton().set_render_target(p_subpass);
    DEV_ASSERT(!p_subpass->color_attachments.empty());
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

    GraphicsManager::singleton().set_render_target(p_subpass);
    DEV_ASSERT(!p_subpass->color_attachments.empty());
    auto [width, height] = p_subpass->color_attachments[0]->get_size();

    glViewport(0, 0, width, height);

    glClear(GL_COLOR_BUFFER_BIT);

    glEnable(GL_DEPTH_TEST);

    GraphicsManager::singleton().set_pipeline_state(PROGRAM_LIGHTING_VXGI);

    // @TODO: fix
    auto camera = SceneManager::singleton().get_scene().m_camera;
    fill_camera_matrices(g_per_pass_cache.cache);
    g_per_pass_cache.Update();

    R_DrawQuad();

    glDepthFunc(GL_LEQUAL);

    // draw skybox here
    {
        auto render_data = GraphicsManager::singleton().get_render_data();
        RenderData::Pass& pass = render_data->main_pass;

        pass.fill_perpass(g_per_pass_cache.cache);
        g_per_pass_cache.Update();
        GraphicsManager::singleton().set_pipeline_state(PROGRAM_ENV_SKYBOX);
        draw_cube_map();
    }
}

void debug_vxgi_pass_func(const Subpass* p_subpass) {
    OPTICK_EVENT();

    GraphicsManager::singleton().set_render_target(p_subpass);
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
    glDrawElementsInstanced(GL_TRIANGLES, g_box.index_count, GL_UNSIGNED_INT, 0, size * size * size);
}

void fxaa_pass_func(const Subpass* p_subpass) {
    OPTICK_EVENT();

    GraphicsManager::singleton().set_render_target(p_subpass);
    DEV_ASSERT(!p_subpass->color_attachments.empty());
    auto depth_buffer = p_subpass->depth_attachment;
    auto [width, height] = p_subpass->color_attachments[0]->get_size();

    GraphicsManager::singleton().set_pipeline_state(PROGRAM_BILLBOARD);

    // draw billboards
    auto render_data = GraphicsManager::singleton().get_render_data();

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

    GraphicsManager::singleton().set_render_target(p_subpass);
    DEV_ASSERT(!p_subpass->color_attachments.empty());
    auto [width, height] = p_subpass->color_attachments[0]->get_size();

    glViewport(0, 0, width, height);
    glDisable(GL_DEPTH_TEST);
    glClear(GL_COLOR_BUFFER_BIT);

    GraphicsManager::singleton().set_pipeline_state(PROGRAM_IMAGE_2D);

    // @TODO: clean up
    auto final_image_handle = GraphicsManager::singleton().find_render_target(RT_RES_FXAA)->texture->get_resident_handle();
    debug_draw_quad(final_image_handle, DISPLAY_CHANNEL_RGB, width, height, width, height);

#if 0
    auto handle = GraphicsManager::singleton().find_resource(RT_BRDF)->get_resident_handle();
    debug_draw_quad(handle, DISPLAY_CHANNEL_RGB, width, height, 512, 512);
#endif
#if 0
    auto shadow_map_handle = GraphicsManager::singleton().find_resource(RT_RES_SHADOW_MAP)->get_resident_handle();
    debug_draw_quad(shadow_map_handle, DISPLAY_CHANNEL_RRR, width, height, 800, 200);
#endif
}

void create_render_graph_vxgi(RenderGraph& graph) {
    ivec2 frame_size = DVAR_GET_IVEC2(resolution);
    int w = frame_size.x;
    int h = frame_size.y;

    GraphicsManager& manager = GraphicsManager::singleton();

    // @TODO: refactor
    auto gbuffer_attachment0 = manager.create_render_target(RenderTargetDesc{ RT_RES_GBUFFER_POSITION,
                                                                              PixelFormat::R16G16B16A16_FLOAT,
                                                                              AttachmentType::COLOR_2D,
                                                                              w, h },
                                                            nearest_sampler());
    auto gbuffer_attachment1 = manager.create_render_target(RenderTargetDesc{ RT_RES_GBUFFER_NORMAL,
                                                                              PixelFormat::R16G16B16A16_FLOAT,
                                                                              AttachmentType::COLOR_2D,
                                                                              w, h },
                                                            nearest_sampler());
    auto gbuffer_attachment2 = manager.create_render_target(RenderTargetDesc{ RT_RES_GBUFFER_BASE_COLOR,
                                                                              PixelFormat::R8G8B8A8_UINT,
                                                                              AttachmentType::COLOR_2D,
                                                                              w, h },
                                                            nearest_sampler());
    auto gbuffer_depth = manager.create_render_target(RenderTargetDesc{ RT_RES_GBUFFER_DEPTH,
                                                                        PixelFormat::D32_FLOAT,
                                                                        AttachmentType::DEPTH_2D,
                                                                        w, h },
                                                      nearest_sampler());

    auto ssao_attachment = manager.create_render_target(RenderTargetDesc{ RT_RES_SSAO,
                                                                          PixelFormat::R32_FLOAT,
                                                                          AttachmentType::COLOR_2D,
                                                                          w, h },
                                                        nearest_sampler());
    auto lighting_attachment = manager.create_render_target(RenderTargetDesc{ RT_RES_LIGHTING,
                                                                              PixelFormat::R8G8B8A8_UINT,
                                                                              AttachmentType::COLOR_2D,
                                                                              w, h },
                                                            nearest_sampler());
    auto fxaa_attachment = manager.create_render_target(RenderTargetDesc{ RT_RES_FXAA,
                                                                          PixelFormat::R8G8B8A8_UINT,
                                                                          AttachmentType::COLOR_2D,
                                                                          w, h },
                                                        nearest_sampler());
    auto final_attachment = manager.create_render_target(RenderTargetDesc{ RT_RES_FINAL,
                                                                           PixelFormat::R8G8B8A8_UINT,
                                                                           AttachmentType::COLOR_2D,
                                                                           w, h },
                                                         nearest_sampler());

    {  // environment pass
        RenderPassDesc desc;
        desc.name = ENV_PASS;
        auto pass = graph.create_pass(desc);

        auto create_cube_map_subpass = [&](const char* cube_map_name, const char* depth_name, int size, SubPassFunc p_func, const SamplerDesc& p_sampler, bool gen_mipmap) {
            auto cube_map = manager.create_render_target(RenderTargetDesc{ cube_map_name,
                                                                           PixelFormat::R16G16B16_FLOAT,
                                                                           AttachmentType::COLOR_CUBE_MAP,
                                                                           size, size, gen_mipmap },
                                                         p_sampler);
            auto depth_map = manager.create_render_target(RenderTargetDesc{ depth_name,
                                                                            PixelFormat::D32_FLOAT,
                                                                            AttachmentType::DEPTH_2D,
                                                                            size, size, gen_mipmap },
                                                          nearest_sampler());

            auto subpass = manager.create_subpass(SubpassDesc{
                .color_attachments = { cube_map },
                .depth_attachment = depth_map,
                .func = p_func,
            });
            return subpass;
        };

        auto brdf_image = manager.create_render_target(RenderTargetDesc{ RT_BRDF, PixelFormat::R16G16_FLOAT, AttachmentType::COLOR_2D, 512, 512, false },
                                                       linear_clamp_sampler());
        auto brdf_subpass = manager.create_subpass(SubpassDesc{
            .color_attachments = { brdf_image },
            .func = generate_brdf_func,
        });

        pass->add_sub_pass(brdf_subpass);
        pass->add_sub_pass(create_cube_map_subpass(RT_ENV_SKYBOX_CUBE_MAP, RT_ENV_SKYBOX_DEPTH, 512, hdr_to_cube_map_pass_func, env_cube_map_sampler_mip(), true));
        pass->add_sub_pass(create_cube_map_subpass(RT_ENV_DIFFUSE_IRRADIANCE_CUBE_MAP, RT_ENV_DIFFUSE_IRRADIANCE_DEPTH, 32, diffuse_irradiance_pass_func, linear_clamp_sampler(), false));
        pass->add_sub_pass(create_cube_map_subpass(RT_ENV_PREFILTER_CUBE_MAP, RT_ENV_PREFILTER_DEPTH, 512, prefilter_pass_func, env_cube_map_sampler_mip(), true));
    }

    {  // shadow pass
        const int shadow_res = DVAR_GET_INT(r_shadow_res);
        DEV_ASSERT(math::is_power_of_two(shadow_res));
        const int point_shadow_res = DVAR_GET_INT(r_point_shadow_res);
        DEV_ASSERT(math::is_power_of_two(point_shadow_res));

        auto shadow_map = manager.create_render_target(RenderTargetDesc{ RT_RES_SHADOW_MAP,
                                                                         PixelFormat::D32_FLOAT,
                                                                         AttachmentType::SHADOW_2D,
                                                                         MAX_CASCADE_COUNT * shadow_res, shadow_res },
                                                       shadow_map_sampler());
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
            auto point_shadow_map = manager.create_render_target(RenderTargetDesc{ RT_RES_POINT_SHADOW_MAP + std::to_string(i),
                                                                                   PixelFormat::D32_FLOAT,
                                                                                   AttachmentType::SHADOW_CUBE_MAP,
                                                                                   point_shadow_res, point_shadow_res },
                                                                 shadow_cube_map_sampler());

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
            .depth_attachment = gbuffer_depth,
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
