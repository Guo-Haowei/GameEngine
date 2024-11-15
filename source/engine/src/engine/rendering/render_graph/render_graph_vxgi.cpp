#include "core/debugger/profiler.h"
#include "core/framework/graphics_manager.h"
#include "core/framework/scene_manager.h"
#include "core/math/frustum.h"
#include "core/math/matrix_transform.h"
#include "rendering/graphics_dvars.h"
#include "rendering/pipeline_state.h"
#include "rendering/render_graph/pass_creator.h"
#include "rendering/render_manager.h"

// @TODO: refactor
#include "drivers/opengl/opengl_graphics_manager.h"
#include "drivers/opengl/opengl_prerequisites.h"
#include "rendering/GpuTexture.h"

// @TODO: refactor
extern OldTexture g_albedoVoxel;
extern OldTexture g_normalVoxel;
extern OpenGLMeshBuffers* g_box;

namespace my::rg {

void voxelization_pass_func(const DrawPass*) {
    OPTICK_EVENT();
    auto& gm = GraphicsManager::GetSingleton();
    auto& ctx = gm.GetContext();

    if (!DVAR_GET_BOOL(gfx_enable_vxgi)) {
        return;
    }

    // @TODO: refactor pass to auto bind resources,
    // and make it a class so don't do a map search every frame
    auto bind_slot = [&](RenderTargetResourceName p_name, int p_slot, Dimension p_dimension = Dimension::TEXTURE_2D) {
        std::shared_ptr<RenderTarget> resource = gm.FindRenderTarget(p_name);
        if (!resource) {
            return;
        }

        gm.BindTexture(p_dimension, resource->texture->GetHandle(), p_slot);
    };

    // bind common textures
    bind_slot(RESOURCE_GBUFFER_BASE_COLOR, t_gbufferBaseColorMapSlot);
    bind_slot(RESOURCE_GBUFFER_POSITION, t_gbufferPositionMapSlot);
    bind_slot(RESOURCE_GBUFFER_NORMAL, t_gbufferNormalMapSlot);
    bind_slot(RESOURCE_GBUFFER_MATERIAL, t_gbufferMaterialMapSlot);

    bind_slot(RESOURCE_SHADOW_MAP, t_shadowMapSlot);
    bind_slot(RESOURCE_POINT_SHADOW_MAP_0, t_pointShadow0Slot, Dimension::TEXTURE_CUBE);
    bind_slot(RESOURCE_POINT_SHADOW_MAP_1, t_pointShadow1Slot, Dimension::TEXTURE_CUBE);
    bind_slot(RESOURCE_POINT_SHADOW_MAP_2, t_pointShadow2Slot, Dimension::TEXTURE_CUBE);
    bind_slot(RESOURCE_POINT_SHADOW_MAP_3, t_pointShadow3Slot, Dimension::TEXTURE_CUBE);

    g_albedoVoxel.clear();
    g_normalVoxel.clear();

    const int voxel_size = DVAR_GET_INT(gfx_voxel_size);

    glDisable(GL_BLEND);
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    glViewport(0, 0, voxel_size, voxel_size);

    // @TODO: fix
    g_albedoVoxel.bindImageTexture(IMAGE_VOXEL_ALBEDO_SLOT);
    g_normalVoxel.bindImageTexture(IMAGE_VOXEL_NORMAL_SLOT);

    PassContext& pass = gm.m_voxelPass;
    gm.BindConstantBufferSlot<PerPassConstantBuffer>(ctx.pass_uniform.get(), pass.pass_idx);

    for (const auto& draw : pass.draws) {
        bool has_bone = draw.bone_idx >= 0;
        if (has_bone) {
            gm.BindConstantBufferSlot<BoneConstantBuffer>(ctx.bone_uniform.get(), draw.bone_idx);
        }

        gm.SetPipelineState(has_bone ? PROGRAM_VOXELIZATION_ANIMATED : PROGRAM_VOXELIZATION_STATIC);

        gm.BindConstantBufferSlot<PerBatchConstantBuffer>(ctx.batch_uniform.get(), draw.batch_idx);

        gm.SetMesh(draw.mesh_data);

        for (const auto& subset : draw.subsets) {
            gm.BindConstantBufferSlot<MaterialConstantBuffer>(ctx.material_uniform.get(), subset.material_idx);

            gm.DrawElements(subset.index_count, subset.index_offset);
        }
    }

    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

    // post process
    GraphicsManager::GetSingleton().SetPipelineState(PROGRAM_VOXELIZATION_POST);

    constexpr GLuint workGroupX = 512;
    constexpr GLuint workGroupY = 512;
    const GLuint workGroupZ = (voxel_size * voxel_size * voxel_size) / (workGroupX * workGroupY);

    glDispatchCompute(workGroupX, workGroupY, workGroupZ);
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

    g_albedoVoxel.bind();
    g_albedoVoxel.genMipMap();
    g_normalVoxel.bind();
    g_normalVoxel.genMipMap();

    glEnable(GL_BLEND);

    // unbind stuff
    gm.UnbindTexture(Dimension::TEXTURE_2D, t_gbufferBaseColorMapSlot);
    gm.UnbindTexture(Dimension::TEXTURE_2D, t_gbufferPositionMapSlot);
    gm.UnbindTexture(Dimension::TEXTURE_2D, t_gbufferNormalMapSlot);
    gm.UnbindTexture(Dimension::TEXTURE_2D, t_gbufferMaterialMapSlot);
    gm.UnbindTexture(Dimension::TEXTURE_2D, t_shadowMapSlot);
    gm.UnbindTexture(Dimension::TEXTURE_CUBE, t_pointShadow0Slot);
    gm.UnbindTexture(Dimension::TEXTURE_CUBE, t_pointShadow1Slot);
    gm.UnbindTexture(Dimension::TEXTURE_CUBE, t_pointShadow2Slot);
    gm.UnbindTexture(Dimension::TEXTURE_CUBE, t_pointShadow3Slot);

    // @TODO: [SCRUM-28] refactor
    gm.SetRenderTarget(nullptr);
}

void hdr_to_cube_map_pass_func(const DrawPass* p_draw_pass) {
    OPTICK_EVENT();

    if (!renderer::need_update_env()) {
        return;
    }

    GraphicsManager::GetSingleton().SetPipelineState(PROGRAM_ENV_SKYBOX_TO_CUBE_MAP);
    auto cube_map = p_draw_pass->color_attachments[0];
    auto [width, height] = cube_map->getSize();

    mat4 projection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
    auto view_matrices = BuildOpenGLCubeMapViewMatrices(vec3(0.0f));
    for (int i = 0; i < 6; ++i) {
        GraphicsManager::GetSingleton().SetRenderTarget(p_draw_pass, i);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glViewport(0, 0, width, height);

        g_env_cache.cache.c_cubeProjectionViewMatrix = projection * view_matrices[i];
        g_env_cache.update();
        RenderManager::GetSingleton().draw_skybox();
    }

    glBindTexture(GL_TEXTURE_CUBE_MAP, cube_map->texture->GetHandle32());
    glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
}

// @TODO: refactor
void generate_brdf_func(const DrawPass* p_draw_pass) {
    OPTICK_EVENT();

    if (!renderer::need_update_env()) {
        return;
    }

    GraphicsManager::GetSingleton().SetPipelineState(PROGRAM_BRDF);
    auto [width, height] = p_draw_pass->color_attachments[0]->getSize();
    GraphicsManager::GetSingleton().SetRenderTarget(p_draw_pass);
    glClear(GL_COLOR_BUFFER_BIT);
    glViewport(0, 0, width, height);
    RenderManager::GetSingleton().draw_quad();
}

void diffuse_irradiance_pass_func(const DrawPass* p_draw_pass) {
    OPTICK_EVENT();
    if (!renderer::need_update_env()) {
        return;
    }

    GraphicsManager::GetSingleton().SetPipelineState(PROGRAM_DIFFUSE_IRRADIANCE);
    auto [width, height] = p_draw_pass->depth_attachment->getSize();

    mat4 projection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
    auto view_matrices = BuildOpenGLCubeMapViewMatrices(vec3(0.0f));

    for (int i = 0; i < 6; ++i) {
        GraphicsManager::GetSingleton().SetRenderTarget(p_draw_pass, i);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glViewport(0, 0, width, height);

        g_env_cache.cache.c_cubeProjectionViewMatrix = projection * view_matrices[i];
        g_env_cache.update();
        RenderManager::GetSingleton().draw_skybox();
    }
}

void prefilter_pass_func(const DrawPass* p_draw_pass) {
    OPTICK_EVENT();
    if (!renderer::need_update_env()) {
        return;
    }

    GraphicsManager::GetSingleton().SetPipelineState(PROGRAM_PREFILTER);
    auto [width, height] = p_draw_pass->depth_attachment->getSize();

    mat4 projection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
    auto view_matrices = BuildOpenGLCubeMapViewMatrices(vec3(0.0f));
    const uint32_t max_mip_levels = 5;

    for (int mip_idx = 0; mip_idx < max_mip_levels; ++mip_idx, width /= 2, height /= 2) {
        for (int face_id = 0; face_id < 6; ++face_id) {
            g_env_cache.cache.c_cubeProjectionViewMatrix = projection * view_matrices[face_id];
            g_env_cache.cache.c_envPassRoughness = (float)mip_idx / (float)(max_mip_levels - 1);
            g_env_cache.update();

            GraphicsManager::GetSingleton().SetRenderTarget(p_draw_pass, face_id, mip_idx);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            glViewport(0, 0, width, height);
            RenderManager::GetSingleton().draw_skybox();
        }
    }

    return;
}

static void highlight_select_pass_func(const DrawPass* p_draw_pass) {
    OPTICK_EVENT();

    auto& manager = GraphicsManager::GetSingleton();
    manager.SetRenderTarget(p_draw_pass);
    DEV_ASSERT(!p_draw_pass->color_attachments.empty());
    auto [width, height] = p_draw_pass->color_attachments[0]->getSize();

    glViewport(0, 0, width, height);

    manager.SetPipelineState(PROGRAM_HIGHLIGHT);
    manager.SetStencilRef(STENCIL_FLAG_SELECTED);
    glClear(GL_COLOR_BUFFER_BIT);
    RenderManager::GetSingleton().draw_quad();
    manager.SetStencilRef(0);
}

void debug_vxgi_pass_func(const DrawPass* p_draw_pass) {
    OPTICK_EVENT();

    GraphicsManager& gm = GraphicsManager::GetSingleton();
    gm.SetRenderTarget(p_draw_pass);
    DEV_ASSERT(!p_draw_pass->color_attachments.empty());
    auto depth_buffer = p_draw_pass->depth_attachment;
    auto [width, height] = p_draw_pass->color_attachments[0]->getSize();

    glViewport(0, 0, width, height);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    GraphicsManager::GetSingleton().SetPipelineState(PROGRAM_DEBUG_VOXEL);

    PassContext& pass = gm.m_mainPass;
    gm.BindConstantBufferSlot<PerPassConstantBuffer>(gm.m_context.pass_uniform.get(), pass.pass_idx);

    glBindVertexArray(g_box->vao);

    const int size = DVAR_GET_INT(gfx_voxel_size);
    glDrawElementsInstanced(GL_TRIANGLES, g_box->indexCount, GL_UNSIGNED_INT, 0, size * size * size);
}

// @TODO: refactor
static void debug_draw_quad(uint64_t p_handle, int p_channel, int p_screen_width, int p_screen_height, int p_width, int p_height) {
    float half_width_ndc = (float)p_width / p_screen_width;
    float half_height_ndc = (float)p_height / p_screen_height;

    vec2 size = vec2(half_width_ndc, half_height_ndc);
    vec2 pos;
    pos.x = 1.0f - half_width_ndc;
    pos.y = 1.0f - half_height_ndc;

    g_debug_draw_cache.cache.c_debugDrawSize = size;
    g_debug_draw_cache.cache.c_debugDrawPos = pos;
    g_debug_draw_cache.cache.c_displayChannel = p_channel;
    g_debug_draw_cache.cache.c_debugDrawMap = p_handle;
    g_debug_draw_cache.update();
    RenderManager::GetSingleton().draw_quad();
}

void final_pass_func(const DrawPass* p_draw_pass) {
    OPTICK_EVENT();

    GraphicsManager::GetSingleton().SetRenderTarget(p_draw_pass);
    DEV_ASSERT(!p_draw_pass->color_attachments.empty());
    auto [width, height] = p_draw_pass->color_attachments[0]->getSize();

    glViewport(0, 0, width, height);
    glClear(GL_COLOR_BUFFER_BIT);

    GraphicsManager::GetSingleton().SetPipelineState(PROGRAM_IMAGE_2D);

    // @TODO: clean up
    auto final_image_handle = GraphicsManager::GetSingleton().FindRenderTarget(RESOURCE_TONE)->texture->GetResidentHandle();
    debug_draw_quad(final_image_handle, DISPLAY_CHANNEL_RGB, width, height, width, height);

    // if (0) {
    //     auto handle = GraphicsManager::singleton().findRenderTarget(RESOURCE_BRDF)->texture->get_resident_handle();
    //     debug_draw_quad(handle, DISPLAY_CHANNEL_RGB, width, height, 512, 512);
    // }

    if (DVAR_GET_BOOL(gfx_debug_shadow)) {
        auto shadow_map_handle = GraphicsManager::GetSingleton().FindRenderTarget(RESOURCE_SHADOW_MAP)->texture->GetResidentHandle();
        debug_draw_quad(shadow_map_handle, DISPLAY_CHANNEL_RRR, width, height, 300, 300);
    }
}

void CreateRenderGraphVxgi(RenderGraph& p_graph) {
    // @TODO: early-z
    const ivec2 frame_size = DVAR_GET_IVEC2(resolution);
    const int w = frame_size.x;
    const int h = frame_size.y;

    RenderPassCreator::Config config;
    config.frame_width = w;
    config.frame_height = h;
    RenderPassCreator creator(config, p_graph);

    GraphicsManager& manager = GraphicsManager::GetSingleton();

    auto final_attachment = manager.CreateRenderTarget(RenderTargetDesc{ RESOURCE_FINAL,
                                                                         PixelFormat::R8G8B8A8_UINT,
                                                                         AttachmentType::COLOR_2D,
                                                                         w, h },
                                                       PointClampSampler());

    {  // environment pass
        RenderPassDesc desc;
        desc.name = RenderPassName::ENV;
        auto pass = p_graph.CreatePass(desc);

        auto create_cube_map_subpass = [&](RenderTargetResourceName cube_map_name, RenderTargetResourceName depth_name, int size, DrawPassExecuteFunc p_func, const SamplerDesc& p_sampler, bool gen_mipmap) {
            auto cube_map = manager.CreateRenderTarget(RenderTargetDesc{ cube_map_name,
                                                                         PixelFormat::R16G16B16_FLOAT,
                                                                         AttachmentType::COLOR_CUBE_MAP,
                                                                         size, size, gen_mipmap },
                                                       p_sampler);
            auto depth_map = manager.CreateRenderTarget(RenderTargetDesc{ depth_name,
                                                                          PixelFormat::D32_FLOAT,
                                                                          AttachmentType::DEPTH_2D,
                                                                          size, size, gen_mipmap },
                                                        PointClampSampler());

            auto draw_pass = manager.CreateDrawPass(DrawPassDesc{
                .color_attachments = { cube_map },
                .depth_attachment = depth_map,
                .exec_func = p_func,
            });
            return draw_pass;
        };

        auto brdf_image = manager.CreateRenderTarget(RenderTargetDesc{ RESOURCE_BRDF, PixelFormat::R16G16_FLOAT, AttachmentType::COLOR_2D, 512, 512, false },
                                                     LinearClampSampler());
        auto brdf_subpass = manager.CreateDrawPass(DrawPassDesc{
            .color_attachments = { brdf_image },
            .exec_func = generate_brdf_func,
        });

        pass->AddDrawPass(brdf_subpass);
        pass->AddDrawPass(create_cube_map_subpass(RESOURCE_ENV_SKYBOX_CUBE_MAP, RESOURCE_ENV_SKYBOX_DEPTH, 512, hdr_to_cube_map_pass_func, env_cube_map_sampler_mip(), true));
        pass->AddDrawPass(create_cube_map_subpass(RESOURCE_ENV_DIFFUSE_IRRADIANCE_CUBE_MAP, RESOURCE_ENV_DIFFUSE_IRRADIANCE_DEPTH, 32, diffuse_irradiance_pass_func, LinearClampSampler(), false));
        pass->AddDrawPass(create_cube_map_subpass(RESOURCE_ENV_PREFILTER_CUBE_MAP, RESOURCE_ENV_PREFILTER_DEPTH, 512, prefilter_pass_func, env_cube_map_sampler_mip(), true));
    }

    creator.AddShadowPass();
    creator.AddGBufferPass();

    auto gbuffer_depth = manager.FindRenderTarget(RESOURCE_GBUFFER_DEPTH);
    {  // highlight selected pass
        auto attachment = manager.CreateRenderTarget(RenderTargetDesc{ RESOURCE_HIGHLIGHT_SELECT,
                                                                       PixelFormat::R8_UINT,
                                                                       AttachmentType::COLOR_2D,
                                                                       w, h },
                                                     PointClampSampler());

        RenderPassDesc desc;
        desc.name = RenderPassName::HIGHLIGHT_SELECT;
        desc.dependencies = { RenderPassName::GBUFFER };
        auto pass = p_graph.CreatePass(desc);
        auto draw_pass = manager.CreateDrawPass(DrawPassDesc{
            .color_attachments = { attachment },
            .depth_attachment = gbuffer_depth,
            .exec_func = highlight_select_pass_func,
        });
        pass->AddDrawPass(draw_pass);
    }

    {  // voxel pass
        RenderPassDesc desc;
        desc.name = RenderPassName::VOXELIZATION;
        desc.dependencies = { RenderPassName::SHADOW };
        auto pass = p_graph.CreatePass(desc);
        auto draw_pass = manager.CreateDrawPass(DrawPassDesc{
            .exec_func = voxelization_pass_func,
        });
        pass->AddDrawPass(draw_pass);
    }

    creator.AddLightingPass();
    creator.AddEmitterPass();
    creator.AddBloomPass();
    creator.AddTonePass();

    {
        // final pass
        RenderPassDesc desc;
        desc.name = RenderPassName::FINAL;
        desc.dependencies = { RenderPassName::TONE };
        auto pass = p_graph.CreatePass(desc);
        auto draw_pass = manager.CreateDrawPass(DrawPassDesc{
            .color_attachments = { final_attachment },
            .exec_func = final_pass_func,
        });
        pass->AddDrawPass(draw_pass);
    }

    // @TODO: allow recompile
    p_graph.Compile();
}

}  // namespace my::rg
