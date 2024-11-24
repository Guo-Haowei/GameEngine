#include "core/debugger/profiler.h"
#include "core/framework/graphics_manager.h"
#include "core/framework/scene_manager.h"
#include "core/math/frustum.h"
#include "core/math/matrix_transform.h"
#include "rendering/graphics_dvars.h"
#include "rendering/pipeline_state.h"
#include "rendering/render_graph/pass_creator.h"
#include "rendering/render_manager.h"
#include "shader_resource_defines.hlsl.h"

// @TODO: refactor
#include "drivers/opengl/opengl_graphics_manager.h"
#include "drivers/opengl/opengl_prerequisites.h"

extern OpenGlMeshBuffers* g_box;

namespace my::rg {

void hdr_to_cube_map_pass_func(const DrawPass* p_draw_pass) {
    OPTICK_EVENT();

    if (!renderer::need_update_env()) {
        return;
    }

    GraphicsManager::GetSingleton().SetPipelineState(PSO_ENV_SKYBOX_TO_CUBE_MAP);
    auto cube_map = p_draw_pass->desc.colorAttachments[0];
    const auto [width, height] = p_draw_pass->GetBufferSize();

    Matrix4x4f projection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
    auto view_matrices = BuildOpenGlCubeMapViewMatrices(Vector3f(0.0f));
    for (int i = 0; i < 6; ++i) {
        GraphicsManager::GetSingleton().SetRenderTarget(p_draw_pass, i);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glViewport(0, 0, width, height);

        g_env_cache.cache.c_cubeProjectionViewMatrix = projection * view_matrices[i];
        g_env_cache.update();
        RenderManager::GetSingleton().draw_skybox();
    }

    glBindTexture(GL_TEXTURE_CUBE_MAP, cube_map->GetHandle32());
    glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
}

// @TODO: refactor
void generate_brdf_func(const DrawPass* p_draw_pass) {
    OPTICK_EVENT();

    if (!renderer::need_update_env()) {
        return;
    }

    GraphicsManager::GetSingleton().SetPipelineState(PSO_BRDF);
    const auto [width, height] = p_draw_pass->GetBufferSize();
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

    GraphicsManager::GetSingleton().SetPipelineState(PSO_DIFFUSE_IRRADIANCE);
    const auto [width, height] = p_draw_pass->GetBufferSize();

    Matrix4x4f projection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
    auto view_matrices = BuildOpenGlCubeMapViewMatrices(Vector3f(0.0f));

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

    GraphicsManager::GetSingleton().SetPipelineState(PSO_PREFILTER);
    auto [width, height] = p_draw_pass->GetBufferSize();

    Matrix4x4f projection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
    auto view_matrices = BuildOpenGlCubeMapViewMatrices(Vector3f(0.0f));
    constexpr int max_mip_levels = 5;

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

void debug_vxgi_pass_func(const DrawPass* p_draw_pass) {
    OPTICK_EVENT();

    GraphicsManager& gm = GraphicsManager::GetSingleton();
    gm.SetRenderTarget(p_draw_pass);
    auto depth_buffer = p_draw_pass->desc.depthAttachment;
    const auto [width, height] = p_draw_pass->GetBufferSize();

    glEnable(GL_BLEND);
    gm.SetViewport(Viewport(width, height));
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    GraphicsManager::GetSingleton().SetPipelineState(PSO_DEBUG_VOXEL);

    PassContext& pass = gm.m_mainPass;
    gm.BindConstantBufferSlot<PerPassConstantBuffer>(gm.GetCurrentFrame().passCb.get(), pass.pass_idx);

    glBindVertexArray(g_box->vao);

    const int size = DVAR_GET_INT(gfx_voxel_size);
    glDrawElementsInstanced(GL_TRIANGLES, g_box->indexCount, GL_UNSIGNED_INT, 0, size * size * size);
    glDisable(GL_BLEND);
}

// @TODO: refactor
static void debug_draw_quad(uint64_t p_handle, int p_channel, int p_screen_width, int p_screen_height, int p_width, int p_height) {
    float half_width_ndc = (float)p_width / p_screen_width;
    float half_height_ndc = (float)p_height / p_screen_height;

    Vector2f size = Vector2f(half_width_ndc, half_height_ndc);
    Vector2f pos;
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
    const auto [width, height] = p_draw_pass->GetBufferSize();

    glViewport(0, 0, width, height);
    glClear(GL_COLOR_BUFFER_BIT);

    GraphicsManager::GetSingleton().SetPipelineState(PSO_RW_TEXTURE_2D);

    // @TODO: clean up
    auto final_image_handle = GraphicsManager::GetSingleton().FindTexture(RESOURCE_TONE)->GetResidentHandle();
    debug_draw_quad(final_image_handle, DISPLAY_CHANNEL_RGB, width, height, width, height);

    // if (0) {
    //     auto handle = GraphicsManager::singleton().findRenderTarget(RESOURCE_BRDF)->texture->get_resident_handle();
    //     debug_draw_quad(handle, DISPLAY_CHANNEL_RGB, width, height, 512, 512);
    // }

    if (DVAR_GET_BOOL(gfx_debug_shadow)) {
        auto shadow_map_handle = GraphicsManager::GetSingleton().FindTexture(RESOURCE_SHADOW_MAP)->GetResidentHandle();
        debug_draw_quad(shadow_map_handle, DISPLAY_CHANNEL_RRR, width, height, 300, 300);
    }
}

void RenderPassCreator::CreateExperimental(RenderGraph& p_graph) {
    // @TODO: early-z
    const Vector2i frame_size = DVAR_GET_IVEC2(resolution);
    const int w = frame_size.x;
    const int h = frame_size.y;

    RenderPassCreator::Config config;
    config.frameWidth = w;
    config.frameHeight = h;
    RenderPassCreator creator(config, p_graph);

    GraphicsManager& manager = GraphicsManager::GetSingleton();

    auto final_attachment = manager.CreateTexture(BuildDefaultTextureDesc(RESOURCE_FINAL,
                                                                          PixelFormat::R8G8B8A8_UINT,
                                                                          AttachmentType::COLOR_2D,
                                                                          w, h),
                                                  PointClampSampler());

    // @TODO: refactor
    {  // environment pass
        RenderPassDesc desc;
        desc.name = RenderPassName::ENV;
        auto pass = p_graph.CreatePass(desc);

        auto create_cube_map_subpass = [&](RenderTargetResourceName cube_map_name, RenderTargetResourceName depth_name, int size, DrawPassExecuteFunc p_func, const SamplerDesc& p_sampler, bool gen_mipmap) {
            auto cube_texture_desc = BuildDefaultTextureDesc(cube_map_name,
                                                             PixelFormat::R16G16B16_FLOAT,
                                                             AttachmentType::COLOR_CUBE,
                                                             size, size, 6);
            cube_texture_desc.miscFlags |= gen_mipmap ? RESOURCE_MISC_GENERATE_MIPS : RESOURCE_MISC_NONE;
            auto cube_map = manager.CreateTexture(cube_texture_desc, p_sampler);

            auto depth_texture_desc = BuildDefaultTextureDesc(depth_name,
                                                              PixelFormat::D32_FLOAT,
                                                              AttachmentType::DEPTH_2D,
                                                              size, size, 6);
            depth_texture_desc.miscFlags |= gen_mipmap ? RESOURCE_MISC_GENERATE_MIPS : RESOURCE_MISC_NONE;
            auto depth_map = manager.CreateTexture(depth_texture_desc, PointClampSampler());

            auto draw_pass = manager.CreateDrawPass(DrawPassDesc{
                .colorAttachments = { cube_map },
                .depthAttachment = depth_map,
                .execFunc = p_func,
            });
            return draw_pass;
        };

        auto brdf_image = manager.CreateTexture(BuildDefaultTextureDesc(RESOURCE_BRDF, PixelFormat::R16G16_FLOAT, AttachmentType::COLOR_2D, 512, 512), LinearClampSampler());
        auto brdf_subpass = manager.CreateDrawPass(DrawPassDesc{
            .colorAttachments = { brdf_image },
            .execFunc = generate_brdf_func,
        });

        pass->AddDrawPass(brdf_subpass);
        pass->AddDrawPass(create_cube_map_subpass(RESOURCE_ENV_SKYBOX_CUBE_MAP, RESOURCE_ENV_SKYBOX_DEPTH, 512, hdr_to_cube_map_pass_func, env_cube_map_sampler_mip(), true));
        pass->AddDrawPass(create_cube_map_subpass(RESOURCE_ENV_DIFFUSE_IRRADIANCE_CUBE_MAP, RESOURCE_ENV_DIFFUSE_IRRADIANCE_DEPTH, 32, diffuse_irradiance_pass_func, LinearClampSampler(), false));
        pass->AddDrawPass(create_cube_map_subpass(RESOURCE_ENV_PREFILTER_CUBE_MAP, RESOURCE_ENV_PREFILTER_DEPTH, 512, prefilter_pass_func, env_cube_map_sampler_mip(), true));
    }

    creator.AddShadowPass();
    creator.AddGbufferPass();
    creator.AddHighlightPass();
    creator.AddVoxelizationPass();
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
            .colorAttachments = { final_attachment },
            .execFunc = final_pass_func,
        });
        pass->AddDrawPass(draw_pass);
    }

    // @TODO: allow recompile
    p_graph.Compile();
}

}  // namespace my::rg
