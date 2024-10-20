#include "core/debugger/profiler.h"
#include "core/framework/graphics_manager.h"
#include "core/framework/scene_manager.h"
#include "core/math/frustum.h"
#include "rendering/pipeline_state.h"
#include "rendering/render_graph/pass_creator.h"
#include "rendering/render_manager.h"
#include "rendering/rendering_dvars.h"

// @TODO: refactor
#include "drivers/opengl/opengl_graphics_manager.h"
#include "rendering/GpuTexture.h"
extern GpuTexture g_albedoVoxel;
extern GpuTexture g_normalVoxel;
// @TODO: add as a object
extern OpenGLMeshBuffers* g_box;

// @TODO: refactor
void fill_camera_matrices(PerPassConstantBuffer& buffer) {
    auto camera = my::SceneManager::singleton().getScene().m_camera;
    buffer.g_view_matrix = camera->getViewMatrix();
    buffer.g_projection_matrix = camera->getProjectionMatrix();
}
// @TODO: fix

namespace my::rg {

void voxelization_pass_func(const DrawPass*) {
    OPTICK_EVENT();
    auto& gm = GraphicsManager::singleton();
    auto& ctx = gm.getContext();

    if (!DVAR_GET_BOOL(r_enable_vxgi)) {
        return;
    }

    g_albedoVoxel.clear();
    g_normalVoxel.clear();

    const int voxel_size = DVAR_GET_INT(r_voxel_size);

    glDisable(GL_BLEND);
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    glViewport(0, 0, voxel_size, voxel_size);

    // @TODO: fix
    g_albedoVoxel.bindImageTexture(IMAGE_VOXEL_ALBEDO_SLOT);
    g_normalVoxel.bindImageTexture(IMAGE_VOXEL_NORMAL_SLOT);

    PassContext& pass = gm.voxel_pass;
    gm.bindUniformSlot<PerPassConstantBuffer>(ctx.pass_uniform.get(), pass.pass_idx);

    for (const auto& draw : pass.draws) {
        bool has_bone = draw.bone_idx >= 0;
        if (has_bone) {
            gm.bindUniformSlot<BoneConstantBuffer>(ctx.bone_uniform.get(), draw.bone_idx);
        }

        gm.setPipelineState(has_bone ? PROGRAM_VOXELIZATION_ANIMATED : PROGRAM_VOXELIZATION_STATIC);

        gm.bindUniformSlot<PerBatchConstantBuffer>(ctx.batch_uniform.get(), draw.batch_idx);

        gm.setMesh(draw.mesh_data);

        for (const auto& subset : draw.subsets) {
            gm.bindUniformSlot<MaterialConstantBuffer>(ctx.material_uniform.get(), subset.material_idx);

            gm.drawElements(subset.index_count, subset.index_offset);
        }
    }

    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

    // post process
    GraphicsManager::singleton().setPipelineState(PROGRAM_VOXELIZATION_POST);

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
}

void hdr_to_cube_map_pass_func(const DrawPass* p_subpass) {
    OPTICK_EVENT();

    if (!renderer::need_update_env()) {
        return;
    }

    GraphicsManager::singleton().setPipelineState(PROGRAM_ENV_SKYBOX_TO_CUBE_MAP);
    auto cube_map = p_subpass->color_attachments[0];
    auto [width, height] = cube_map->getSize();

    mat4 projection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
    auto view_matrices = renderer::cube_map_view_matrices(vec3(0));
    for (int i = 0; i < 6; ++i) {
        GraphicsManager::singleton().setRenderTarget(p_subpass, i);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glViewport(0, 0, width, height);

        g_env_cache.cache.g_cube_projection_view_matrix = projection * view_matrices[i];
        g_env_cache.update();
        RenderManager::singleton().draw_skybox();
    }

    glBindTexture(GL_TEXTURE_CUBE_MAP, cube_map->texture->get_handle32());
    glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
}

// @TODO: refactor
void generate_brdf_func(const DrawPass* p_subpass) {
    OPTICK_EVENT();

    if (!renderer::need_update_env()) {
        return;
    }

    GraphicsManager::singleton().setPipelineState(PROGRAM_BRDF);
    auto [width, height] = p_subpass->color_attachments[0]->getSize();
    GraphicsManager::singleton().setRenderTarget(p_subpass);
    glClear(GL_COLOR_BUFFER_BIT);
    glViewport(0, 0, width, height);
    RenderManager::singleton().draw_quad();
}

void diffuse_irradiance_pass_func(const DrawPass* p_subpass) {
    OPTICK_EVENT();
    if (!renderer::need_update_env()) {
        return;
    }

    GraphicsManager::singleton().setPipelineState(PROGRAM_DIFFUSE_IRRADIANCE);
    auto [width, height] = p_subpass->depth_attachment->getSize();

    mat4 projection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
    auto view_matrices = renderer::cube_map_view_matrices(vec3(0));

    for (int i = 0; i < 6; ++i) {
        GraphicsManager::singleton().setRenderTarget(p_subpass, i);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glViewport(0, 0, width, height);

        g_env_cache.cache.g_cube_projection_view_matrix = projection * view_matrices[i];
        g_env_cache.update();
        RenderManager::singleton().draw_skybox();
    }
}

void prefilter_pass_func(const DrawPass* p_subpass) {
    OPTICK_EVENT();
    if (!renderer::need_update_env()) {
        return;
    }

    GraphicsManager::singleton().setPipelineState(PROGRAM_PREFILTER);
    auto [width, height] = p_subpass->depth_attachment->getSize();

    mat4 projection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
    auto view_matrices = renderer::cube_map_view_matrices(vec3(0));
    const uint32_t max_mip_levels = 5;

    for (int mip_idx = 0; mip_idx < max_mip_levels; ++mip_idx, width /= 2, height /= 2) {
        for (int face_id = 0; face_id < 6; ++face_id) {
            g_env_cache.cache.g_cube_projection_view_matrix = projection * view_matrices[face_id];
            g_env_cache.cache.g_env_pass_roughness = (float)mip_idx / (float)(max_mip_levels - 1);
            g_env_cache.update();

            GraphicsManager::singleton().setRenderTarget(p_subpass, face_id, mip_idx);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            glViewport(0, 0, width, height);
            RenderManager::singleton().draw_skybox();
        }
    }

    return;
}

static void highlight_select_pass_func(const DrawPass* p_subpass) {
    OPTICK_EVENT();

    auto& manager = GraphicsManager::singleton();
    manager.setRenderTarget(p_subpass);
    DEV_ASSERT(!p_subpass->color_attachments.empty());
    auto [width, height] = p_subpass->color_attachments[0]->getSize();

    glViewport(0, 0, width, height);

    manager.setPipelineState(PROGRAM_HIGHLIGHT);
    manager.setStencilRef(STENCIL_FLAG_SELECTED);
    glClear(GL_COLOR_BUFFER_BIT);
    RenderManager::singleton().draw_quad();
    manager.setStencilRef(0);
}

void debug_vxgi_pass_func(const DrawPass* p_subpass) {
    OPTICK_EVENT();

    GraphicsManager& gm = GraphicsManager::singleton();
    gm.setRenderTarget(p_subpass);
    DEV_ASSERT(!p_subpass->color_attachments.empty());
    auto depth_buffer = p_subpass->depth_attachment;
    auto [width, height] = p_subpass->color_attachments[0]->getSize();

    glViewport(0, 0, width, height);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    GraphicsManager::singleton().setPipelineState(PROGRAM_DEBUG_VOXEL);

    PassContext& pass = gm.main_pass;
    gm.bindUniformSlot<PerPassConstantBuffer>(gm.m_context.pass_uniform.get(), pass.pass_idx);

    glBindVertexArray(g_box->vao);

    const int size = DVAR_GET_INT(r_voxel_size);
    glDrawElementsInstanced(GL_TRIANGLES, g_box->index_count, GL_UNSIGNED_INT, 0, size * size * size);
}

static void tone_pass_func(const DrawPass* p_subpass) {
    OPTICK_EVENT();

    GraphicsManager::singleton().setRenderTarget(p_subpass);
    DEV_ASSERT(!p_subpass->color_attachments.empty());
    auto depth_buffer = p_subpass->depth_attachment;
    auto [width, height] = p_subpass->color_attachments[0]->getSize();

    GraphicsManager::singleton().setPipelineState(PROGRAM_BILLBOARD);

    // draw billboards
    GraphicsManager& gm = GraphicsManager::singleton();

    // HACK:
    if (DVAR_GET_BOOL(r_debug_vxgi)) {
        debug_vxgi_pass_func(p_subpass);
    } else {
        glViewport(0, 0, width, height);
        glClear(GL_COLOR_BUFFER_BIT);

        gm.setPipelineState(PROGRAM_TONE);
        RenderManager::singleton().draw_quad();
    }
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
    g_debug_draw_cache.update();
    RenderManager::singleton().draw_quad();
}

void final_pass_func(const DrawPass* p_subpass) {
    OPTICK_EVENT();

    GraphicsManager::singleton().setRenderTarget(p_subpass);
    DEV_ASSERT(!p_subpass->color_attachments.empty());
    auto [width, height] = p_subpass->color_attachments[0]->getSize();

    glViewport(0, 0, width, height);
    glClear(GL_COLOR_BUFFER_BIT);

    GraphicsManager::singleton().setPipelineState(PROGRAM_IMAGE_2D);

    // @TODO: clean up
    auto final_image_handle = GraphicsManager::singleton().findRenderTarget(RT_RES_TONE)->texture->get_resident_handle();
    debug_draw_quad(final_image_handle, DISPLAY_CHANNEL_RGB, width, height, width, height);

    if (0) {
        auto handle = GraphicsManager::singleton().findRenderTarget(RT_BRDF)->texture->get_resident_handle();
        debug_draw_quad(handle, DISPLAY_CHANNEL_RGB, width, height, 512, 512);
    }

    if (0) {
        int level = DVAR_GET_INT(r_debug_bloom_downsample);
        auto handle = GraphicsManager::singleton().findRenderTarget(std::format("{}_{}", RT_RES_BLOOM, level))->texture->get_resident_handle();
        debug_draw_quad(handle, DISPLAY_CHANNEL_RGB, width, height, 1980 / 4, 1080 / 4);
    }

    if (DVAR_GET_BOOL(gfx_debug_shadow)) {
        auto shadow_map_handle = GraphicsManager::singleton().findRenderTarget(RT_RES_SHADOW_MAP)->texture->get_resident_handle();
        debug_draw_quad(shadow_map_handle, DISPLAY_CHANNEL_RRR, width, height, 300, 300);
    }
}

void createRenderGraphVxgi(RenderGraph& p_graph) {
    // @TODO: early-z

    ivec2 frame_size = DVAR_GET_IVEC2(resolution);
    int w = frame_size.x;
    int h = frame_size.y;

    RenderPassCreator::Config config;
    config.frame_width = w;
    config.frame_height = h;
    RenderPassCreator creator(config, p_graph);

    GraphicsManager& manager = GraphicsManager::singleton();

    auto final_attachment = manager.createRenderTarget(RenderTargetDesc{ RT_RES_FINAL,
                                                                         PixelFormat::R8G8B8A8_UINT,
                                                                         AttachmentType::COLOR_2D,
                                                                         w, h },
                                                       nearest_sampler());

    {  // environment pass
        RenderPassDesc desc;
        desc.name = RenderPassName::ENV;
        auto pass = p_graph.createPass(desc);

        auto create_cube_map_subpass = [&](const char* cube_map_name, const char* depth_name, int size, DrawPassExecuteFunc p_func, const SamplerDesc& p_sampler, bool gen_mipmap) {
            auto cube_map = manager.createRenderTarget(RenderTargetDesc{ cube_map_name,
                                                                         PixelFormat::R16G16B16_FLOAT,
                                                                         AttachmentType::COLOR_CUBE_MAP,
                                                                         size, size, gen_mipmap },
                                                       p_sampler);
            auto depth_map = manager.createRenderTarget(RenderTargetDesc{ depth_name,
                                                                          PixelFormat::D32_FLOAT,
                                                                          AttachmentType::DEPTH_2D,
                                                                          size, size, gen_mipmap },
                                                        nearest_sampler());

            auto subpass = manager.createDrawPass(DrawPassDesc{
                .color_attachments = { cube_map },
                .depth_attachment = depth_map,
                .exec_func = p_func,
            });
            return subpass;
        };

        auto brdf_image = manager.createRenderTarget(RenderTargetDesc{ RT_BRDF, PixelFormat::R16G16_FLOAT, AttachmentType::COLOR_2D, 512, 512, false },
                                                     linear_clamp_sampler());
        auto brdf_subpass = manager.createDrawPass(DrawPassDesc{
            .color_attachments = { brdf_image },
            .exec_func = generate_brdf_func,
        });

        pass->addDrawPass(brdf_subpass);
        pass->addDrawPass(create_cube_map_subpass(RT_ENV_SKYBOX_CUBE_MAP, RT_ENV_SKYBOX_DEPTH, 512, hdr_to_cube_map_pass_func, env_cube_map_sampler_mip(), true));
        pass->addDrawPass(create_cube_map_subpass(RT_ENV_DIFFUSE_IRRADIANCE_CUBE_MAP, RT_ENV_DIFFUSE_IRRADIANCE_DEPTH, 32, diffuse_irradiance_pass_func, linear_clamp_sampler(), false));
        pass->addDrawPass(create_cube_map_subpass(RT_ENV_PREFILTER_CUBE_MAP, RT_ENV_PREFILTER_DEPTH, 512, prefilter_pass_func, env_cube_map_sampler_mip(), true));
    }

    creator.addShadowPass();
    creator.addGBufferPass();

    auto gbuffer_depth = manager.findRenderTarget(RT_RES_GBUFFER_DEPTH);
    {  // highlight selected pass
        auto attachment = manager.createRenderTarget(RenderTargetDesc{ RT_RES_HIGHLIGHT_SELECT,
                                                                       PixelFormat::R8_UINT,
                                                                       AttachmentType::COLOR_2D,
                                                                       w, h },
                                                     nearest_sampler());

        RenderPassDesc desc;
        desc.name = RenderPassName::HIGHLIGHT_SELECT;
        desc.dependencies = { RenderPassName::GBUFFER };
        auto pass = p_graph.createPass(desc);
        auto subpass = manager.createDrawPass(DrawPassDesc{
            .color_attachments = { attachment },
            .depth_attachment = gbuffer_depth,
            .exec_func = highlight_select_pass_func,
        });
        pass->addDrawPass(subpass);
    }

    {  // voxel pass
        RenderPassDesc desc;
        desc.name = RenderPassName::VOXELIZATION;
        desc.dependencies = { RenderPassName::SHADOW };
        auto pass = p_graph.createPass(desc);
        auto subpass = manager.createDrawPass(DrawPassDesc{
            .exec_func = voxelization_pass_func,
        });
        pass->addDrawPass(subpass);
    }

    creator.addLightingPass();
    creator.addBloomPass();

    {  // tone pass
        RenderPassDesc desc;
        desc.name = RenderPassName::TONE;
        desc.dependencies = { RenderPassName::BLOOM };
        auto pass = p_graph.createPass(desc);

        auto attachment = manager.createRenderTarget(RenderTargetDesc{ RT_RES_TONE,
                                                                       PixelFormat::R11G11B10_FLOAT,
                                                                       AttachmentType::COLOR_2D,
                                                                       w, h },
                                                     nearest_sampler());

        auto subpass = manager.createDrawPass(DrawPassDesc{
            .color_attachments = { attachment },
            .depth_attachment = gbuffer_depth,
            .exec_func = tone_pass_func,
        });
        pass->addDrawPass(subpass);
    }
    {
        // final pass
        RenderPassDesc desc;
        desc.name = RenderPassName::FINAL;
        desc.dependencies = { RenderPassName::TONE };
        auto pass = p_graph.createPass(desc);
        auto subpass = manager.createDrawPass(DrawPassDesc{
            .color_attachments = { final_attachment },
            .exec_func = final_pass_func,
        });
        pass->addDrawPass(subpass);
    }

    // @TODO: allow recompile
    p_graph.compile();
}

}  // namespace my::rg
