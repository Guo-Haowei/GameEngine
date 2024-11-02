#include "pass_creator.h"

#include "core/debugger/profiler.h"
#include "core/framework/graphics_manager.h"
#include "rendering/render_manager.h"
#include "rendering/rendering_dvars.h"

// @TODO: this is temporary
#include "core/framework/scene_manager.h"
#include "drivers/opengl/opengl_prerequisites.h"
namespace {

GLuint g_particleSsbo;
GLuint g_counterSsbo;
GLuint g_deadSsbo;
GLuint g_aliveSsbo[2];

}  // namespace

namespace my::rg {

/// Gbuffer
static void gbufferPassFunc(const DrawPass* p_draw_pass) {
    OPTICK_EVENT();

    auto& gm = GraphicsManager::GetSingleton();
    auto& ctx = gm.GetContext();
    auto [width, height] = p_draw_pass->depth_attachment->getSize();

    gm.SetRenderTarget(p_draw_pass);

    gm.SetViewport(Viewport(width, height));

    float clear_color[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    gm.Clear(p_draw_pass, CLEAR_COLOR_BIT | CLEAR_DEPTH_BIT | CLEAR_STENCIL_BIT, clear_color);

    PassContext& pass = gm.m_mainPass;
    gm.BindConstantBufferSlot<PerPassConstantBuffer>(ctx.pass_uniform.get(), pass.pass_idx);

    for (const auto& draw : pass.draws) {
        bool has_bone = draw.bone_idx >= 0;
        if (has_bone) {
            gm.BindConstantBufferSlot<BoneConstantBuffer>(ctx.bone_uniform.get(), draw.bone_idx);
        }

        gm.SetPipelineState(has_bone ? PROGRAM_GBUFFER_ANIMATED : PROGRAM_GBUFFER_STATIC);

        if (draw.flags) {
            gm.SetStencilRef(draw.flags);
        }

        gm.BindConstantBufferSlot<PerBatchConstantBuffer>(ctx.batch_uniform.get(), draw.batch_idx);

        gm.SetMesh(draw.mesh_data);

        for (const auto& subset : draw.subsets) {
            const MaterialConstantBuffer& material = gm.m_context.material_cache.buffer[subset.material_idx];
            gm.BindTexture(Dimension::TEXTURE_2D, material.u_base_color_map_handle, u_base_color_map_slot);
            gm.BindTexture(Dimension::TEXTURE_2D, material.u_normal_map_handle, u_normal_map_slot);
            gm.BindTexture(Dimension::TEXTURE_2D, material.u_material_map_handle, u_material_map_slot);

            gm.BindConstantBufferSlot<MaterialConstantBuffer>(ctx.material_uniform.get(), subset.material_idx);

            // @TODO: set material

            gm.DrawElements(subset.index_count, subset.index_offset);

            // @TODO: unbind
        }

        if (draw.flags) {
            gm.SetStencilRef(0);
        }
    }

    const bool is_opengl = gm.GetBackend() == Backend::OPENGL;
    if (is_opengl) {
        if (g_particleSsbo == 0) {
            {  // particles
                glGenBuffers(1, &g_particleSsbo);
                glBindBuffer(GL_SHADER_STORAGE_BUFFER, g_particleSsbo);
                constexpr size_t buffer_size = MAX_PARTICLE_COUNT * 4 * sizeof(vec4);
                glBufferData(GL_SHADER_STORAGE_BUFFER, buffer_size, nullptr, GL_STATIC_DRAW);
                glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, g_particleSsbo);
            }
            {  // counters
                glGenBuffers(1, &g_counterSsbo);
                glBindBuffer(GL_SHADER_STORAGE_BUFFER, g_counterSsbo);
                constexpr size_t buffer_size = 256;
                glBufferData(GL_SHADER_STORAGE_BUFFER, buffer_size, nullptr, GL_STATIC_DRAW);
                glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, g_counterSsbo);
            }
            {  // dead particles
                glGenBuffers(1, &g_deadSsbo);
                glBindBuffer(GL_SHADER_STORAGE_BUFFER, g_deadSsbo);
                constexpr size_t buffer_size = MAX_PARTICLE_COUNT * sizeof(int);
                glBufferData(GL_SHADER_STORAGE_BUFFER, buffer_size, nullptr, GL_STATIC_DRAW);
                glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, g_deadSsbo);
            }
            {  // alive particles 0
                glGenBuffers(1, &g_aliveSsbo[0]);
                glBindBuffer(GL_SHADER_STORAGE_BUFFER, g_aliveSsbo[0]);
                constexpr size_t buffer_size = MAX_PARTICLE_COUNT * sizeof(int);
                glBufferData(GL_SHADER_STORAGE_BUFFER, buffer_size, nullptr, GL_STATIC_DRAW);
            }
            {  // alive particles 1
                glGenBuffers(1, &g_aliveSsbo[1]);
                glBindBuffer(GL_SHADER_STORAGE_BUFFER, g_aliveSsbo[1]);
                constexpr size_t buffer_size = MAX_PARTICLE_COUNT * sizeof(int);
                glBufferData(GL_SHADER_STORAGE_BUFFER, buffer_size, nullptr, GL_STATIC_DRAW);
            }

            // @TODO: only run once
            gm.SetPipelineState(PROGRAM_PARTICLE_INIT);
            gm.Dispatch(MAX_PARTICLE_COUNT / 32, 1, 1);
        }

        // @TODO: use 1 bit
        static int mSimIndex = 1;
        mSimIndex = 1 - mSimIndex;

        int pre_sim_idx = mSimIndex;
        int post_sim_idx = 1 - mSimIndex;
        g_particleCache.cache.u_PreSimIdx = pre_sim_idx;
        g_particleCache.cache.u_PostSimIdx = post_sim_idx;
        g_particleCache.cache.u_LifeSpan = 2.0f;
        g_particleCache.cache.u_ElapsedTime = SceneManager::GetScene().m_elapsedTime;
        g_particleCache.update();

        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, g_aliveSsbo[pre_sim_idx]);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, g_aliveSsbo[post_sim_idx]);

        gm.SetPipelineState(PROGRAM_PARTICLE_KICKOFF);
        gm.Dispatch(1, 1, 1);

        gm.SetPipelineState(PROGRAM_PARTICLE_EMIT);
        gm.Dispatch(MAX_PARTICLE_COUNT / 32, 1, 1);

        gm.SetPipelineState(PROGRAM_PARTICLE_SIM);
        gm.Dispatch(MAX_PARTICLE_COUNT / 32, 1, 1);
    }

    gm.SetPipelineState(PROGRAM_PARTICLE_RENDERING);
    RenderManager::GetSingleton().draw_quad_instanced(MAX_PARTICLE_COUNT);
}

void RenderPassCreator::addGBufferPass() {
    GraphicsManager& manager = GraphicsManager::GetSingleton();

    int p_width = m_config.frame_width;
    int p_height = m_config.frame_height;

    // @TODO: decouple sampler and render target
    auto gbuffer_depth = manager.CreateRenderTarget(RenderTargetDesc(RESOURCE_GBUFFER_DEPTH,
                                                                     PixelFormat::D24_UNORM_S8_UINT,
                                                                     AttachmentType::DEPTH_STENCIL_2D,
                                                                     p_width, p_height),
                                                    nearest_sampler());

    auto attachment0 = manager.CreateRenderTarget(RenderTargetDesc{ RESOURCE_GBUFFER_BASE_COLOR,
                                                                    PixelFormat::R11G11B10_FLOAT,
                                                                    AttachmentType::COLOR_2D,
                                                                    p_width, p_height },
                                                  nearest_sampler());

    auto attachment1 = manager.CreateRenderTarget(RenderTargetDesc{ RESOURCE_GBUFFER_POSITION,
                                                                    PixelFormat::R16G16B16_FLOAT,
                                                                    AttachmentType::COLOR_2D,
                                                                    p_width, p_height },
                                                  nearest_sampler());

    auto attachment2 = manager.CreateRenderTarget(RenderTargetDesc{ RESOURCE_GBUFFER_NORMAL,
                                                                    PixelFormat::R16G16B16_FLOAT,
                                                                    AttachmentType::COLOR_2D,
                                                                    p_width, p_height },
                                                  nearest_sampler());

    auto attachment3 = manager.CreateRenderTarget(RenderTargetDesc{ RESOURCE_GBUFFER_MATERIAL,
                                                                    PixelFormat::R11G11B10_FLOAT,
                                                                    AttachmentType::COLOR_2D,
                                                                    p_width, p_height },
                                                  nearest_sampler());

    RenderPassDesc desc;
    desc.name = RenderPassName::GBUFFER;
    auto pass = m_graph.createPass(desc);
    auto draw_pass = manager.CreateDrawPass(DrawPassDesc{
        .color_attachments = { attachment0, attachment1, attachment2, attachment3 },
        .depth_attachment = gbuffer_depth,
        .exec_func = gbufferPassFunc,
    });
    pass->addDrawPass(draw_pass);
}

/// Shadow
static void pointShadowPassFunc(const DrawPass* p_draw_pass, int p_pass_id) {
    OPTICK_EVENT();

    auto& gm = GraphicsManager::GetSingleton();
    auto& ctx = gm.GetContext();

    auto& pass_ptr = gm.m_pointShadowPasses[p_pass_id];
    if (!pass_ptr) {
        return;
    }

    // prepare render data
    auto [width, height] = p_draw_pass->depth_attachment->getSize();

    // @TODO: instead of render the same object 6 times
    // set up different object list for different pass
    const PassContext& pass = *pass_ptr.get();

    const auto& light_matrices = pass.light_component.GetMatrices();
    for (int i = 0; i < 6; ++i) {
        g_point_shadow_cache.cache.g_point_light_matrix = light_matrices[i];
        g_point_shadow_cache.cache.g_point_light_position = pass.light_component.GetPosition();
        g_point_shadow_cache.cache.g_point_light_far = pass.light_component.GetMaxDistance();
        g_point_shadow_cache.update();

        gm.SetRenderTarget(p_draw_pass, i);
        gm.Clear(p_draw_pass, CLEAR_DEPTH_BIT, nullptr, i);

        Viewport viewport{ width, height };
        gm.SetViewport(viewport);

        for (const auto& draw : pass.draws) {
            bool has_bone = draw.bone_idx >= 0;
            if (has_bone) {
                gm.BindConstantBufferSlot<BoneConstantBuffer>(ctx.bone_uniform.get(), draw.bone_idx);
            }

            gm.SetPipelineState(has_bone ? PROGRAM_POINT_SHADOW_ANIMATED : PROGRAM_POINT_SHADOW_STATIC);

            gm.BindConstantBufferSlot<PerBatchConstantBuffer>(ctx.batch_uniform.get(), draw.batch_idx);

            gm.SetMesh(draw.mesh_data);
            gm.DrawElements(draw.mesh_data->indexCount);
        }
    }
}

static void shadowPassFunc(const DrawPass* p_draw_pass) {
    OPTICK_EVENT();

    auto& gm = GraphicsManager::GetSingleton();
    auto& ctx = gm.GetContext();

    gm.SetRenderTarget(p_draw_pass);
    auto [width, height] = p_draw_pass->depth_attachment->getSize();

    gm.Clear(p_draw_pass, CLEAR_DEPTH_BIT);

    Viewport viewport{ width, height };
    gm.SetViewport(viewport);

    PassContext& pass = gm.m_shadowPasses[0];
    gm.BindConstantBufferSlot<PerPassConstantBuffer>(ctx.pass_uniform.get(), pass.pass_idx);

    for (const auto& draw : pass.draws) {
        bool has_bone = draw.bone_idx >= 0;
        if (has_bone) {
            gm.BindConstantBufferSlot<BoneConstantBuffer>(ctx.bone_uniform.get(), draw.bone_idx);
        }

        // @TODO: sort the objects so there's no need to switch pipeline
        gm.SetPipelineState(has_bone ? PROGRAM_DPETH_ANIMATED : PROGRAM_DPETH_STATIC);

        gm.BindConstantBufferSlot<PerBatchConstantBuffer>(ctx.batch_uniform.get(), draw.batch_idx);

        gm.SetMesh(draw.mesh_data);
        gm.DrawElements(draw.mesh_data->indexCount);
    }

    gm.UnsetRenderTarget();
}

void RenderPassCreator::addShadowPass() {
    GraphicsManager& manager = GraphicsManager::GetSingleton();

    const int shadow_res = DVAR_GET_INT(r_shadow_res);
    DEV_ASSERT(math::IsPowerOfTwo(shadow_res));
    const int point_shadow_res = DVAR_GET_INT(r_point_shadow_res);
    DEV_ASSERT(math::IsPowerOfTwo(point_shadow_res));

    auto shadow_map = manager.CreateRenderTarget(RenderTargetDesc{ RESOURCE_SHADOW_MAP,
                                                                   PixelFormat::D32_FLOAT,
                                                                   AttachmentType::SHADOW_2D,
                                                                   1 * shadow_res, shadow_res },
                                                 shadow_map_sampler());
    RenderPassDesc desc;
    desc.name = RenderPassName::SHADOW;
    auto pass = m_graph.createPass(desc);
    {
        auto draw_pass = manager.CreateDrawPass(DrawPassDesc{
            .depth_attachment = shadow_map,
            .exec_func = shadowPassFunc,
        });
        pass->addDrawPass(draw_pass);
    }

    // @TODO: refactor
    DrawPassExecuteFunc funcs[] = {
        [](const DrawPass* p_draw_pass) {
            pointShadowPassFunc(p_draw_pass, 0);
        },
        [](const DrawPass* p_draw_pass) {
            pointShadowPassFunc(p_draw_pass, 1);
        },
        [](const DrawPass* p_draw_pass) {
            pointShadowPassFunc(p_draw_pass, 2);
        },
        [](const DrawPass* p_draw_pass) {
            pointShadowPassFunc(p_draw_pass, 3);
        },
    };

    static_assert(array_length(funcs) == MAX_LIGHT_CAST_SHADOW_COUNT);

    {
        for (int i = 0; i < MAX_LIGHT_CAST_SHADOW_COUNT; ++i) {
            auto point_shadow_map = manager.CreateRenderTarget(RenderTargetDesc{ static_cast<RenderTargetResourceName>(RESOURCE_POINT_SHADOW_MAP_0 + i),
                                                                                 PixelFormat::D32_FLOAT,
                                                                                 AttachmentType::SHADOW_CUBE_MAP,
                                                                                 point_shadow_res, point_shadow_res },
                                                               shadow_cube_map_sampler());

            auto draw_pass = manager.CreateDrawPass(DrawPassDesc{
                .depth_attachment = point_shadow_map,
                .exec_func = funcs[i],
            });
            pass->addDrawPass(draw_pass);
        }
    }
}

/// Lighting
static void lightingPassFunc(const DrawPass* p_draw_pass) {
    OPTICK_EVENT();

    auto& gm = GraphicsManager::GetSingleton();
    DEV_ASSERT(!p_draw_pass->color_attachments.empty());
    auto [width, height] = p_draw_pass->color_attachments[0]->getSize();

    gm.SetRenderTarget(p_draw_pass);

    Viewport viewport(width, height);
    gm.SetViewport(viewport);
    gm.Clear(p_draw_pass, CLEAR_COLOR_BIT);
    gm.SetPipelineState(PROGRAM_LIGHTING);

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
    bind_slot(RESOURCE_GBUFFER_BASE_COLOR, u_gbuffer_base_color_map_slot);
    bind_slot(RESOURCE_GBUFFER_POSITION, u_gbuffer_position_map_slot);
    bind_slot(RESOURCE_GBUFFER_NORMAL, u_gbuffer_normal_map_slot);
    bind_slot(RESOURCE_GBUFFER_MATERIAL, u_gbuffer_material_map_slot);

    bind_slot(RESOURCE_SHADOW_MAP, t_shadow_map_slot);
    bind_slot(RESOURCE_POINT_SHADOW_MAP_0, t_point_shadow_0_slot, Dimension::TEXTURE_CUBE);
    bind_slot(RESOURCE_POINT_SHADOW_MAP_1, t_point_shadow_1_slot, Dimension::TEXTURE_CUBE);
    bind_slot(RESOURCE_POINT_SHADOW_MAP_2, t_point_shadow_2_slot, Dimension::TEXTURE_CUBE);
    bind_slot(RESOURCE_POINT_SHADOW_MAP_3, t_point_shadow_3_slot, Dimension::TEXTURE_CUBE);

    // @TODO: fix it
    RenderManager::GetSingleton().draw_quad();

    PassContext& pass = gm.m_mainPass;
    gm.BindConstantBufferSlot<PerPassConstantBuffer>(gm.m_context.pass_uniform.get(), pass.pass_idx);

    // if (0) {
    //     // draw billboard grass here for now
    //     manager.SetPipelineState(PROGRAM_BILLBOARD);
    //     manager.setMesh(&g_grass);
    //     glDrawElementsInstanced(GL_TRIANGLES, g_grass.index_count, GL_UNSIGNED_INT, 0, 64);
    // }

    // @TODO: fix skybox
    if (gm.GetBackend() == Backend::OPENGL) {
        GraphicsManager::GetSingleton().SetPipelineState(PROGRAM_ENV_SKYBOX);
        RenderManager::GetSingleton().draw_skybox();
    }

    // unbind stuff
    gm.UnbindTexture(Dimension::TEXTURE_2D, u_gbuffer_base_color_map_slot);
    gm.UnbindTexture(Dimension::TEXTURE_2D, u_gbuffer_position_map_slot);
    gm.UnbindTexture(Dimension::TEXTURE_2D, u_gbuffer_normal_map_slot);
    gm.UnbindTexture(Dimension::TEXTURE_2D, u_gbuffer_material_map_slot);
    gm.UnbindTexture(Dimension::TEXTURE_2D, t_shadow_map_slot);
    gm.UnbindTexture(Dimension::TEXTURE_CUBE, t_point_shadow_0_slot);
    gm.UnbindTexture(Dimension::TEXTURE_CUBE, t_point_shadow_1_slot);
    gm.UnbindTexture(Dimension::TEXTURE_CUBE, t_point_shadow_2_slot);
    gm.UnbindTexture(Dimension::TEXTURE_CUBE, t_point_shadow_3_slot);

    // @TODO: [SCRUM-28] refactor
    gm.SetRenderTarget(nullptr);
}

void RenderPassCreator::addLightingPass() {
    GraphicsManager& manager = GraphicsManager::GetSingleton();

    auto gbuffer_depth = manager.FindRenderTarget(RESOURCE_GBUFFER_DEPTH);

    auto lighting_attachment = manager.CreateRenderTarget(RenderTargetDesc{ RESOURCE_LIGHTING,
                                                                            PixelFormat::R11G11B10_FLOAT,
                                                                            AttachmentType::COLOR_2D,
                                                                            m_config.frame_width, m_config.frame_height },
                                                          nearest_sampler());

    RenderPassDesc desc;
    desc.name = RenderPassName::LIGHTING;

    desc.dependencies = { RenderPassName::GBUFFER };
    if (m_config.enable_shadow) {
        desc.dependencies.push_back(RenderPassName::SHADOW);
    }
    if (m_config.enable_voxel_gi) {
        desc.dependencies.push_back(RenderPassName::VOXELIZATION);
    }
    if (m_config.enable_ibl) {
        desc.dependencies.push_back(RenderPassName::ENV);
    }

    auto pass = m_graph.createPass(desc);
    auto drawpass = manager.CreateDrawPass(DrawPassDesc{
        .color_attachments = { lighting_attachment },
        .depth_attachment = gbuffer_depth,
        .exec_func = lightingPassFunc,
    });
    pass->addDrawPass(drawpass);
}

/// Bloom
static void bloomFunction(const DrawPass*) {
    GraphicsManager& gm = GraphicsManager::GetSingleton();

    // Step 1, select pixels contribute to bloom
    {
        gm.SetPipelineState(PROGRAM_BLOOM_SETUP);
        auto input = gm.FindRenderTarget(RESOURCE_LIGHTING);
        auto output = gm.FindRenderTarget(RESOURCE_BLOOM_0);

        auto [width, height] = input->getSize();
        const uint32_t work_group_x = math::CeilingDivision(width, 16);
        const uint32_t work_group_y = math::CeilingDivision(height, 16);

        gm.BindTexture(Dimension::TEXTURE_2D, input->texture->GetHandle(), g_bloom_input_image_slot);
        gm.SetUnorderedAccessView(IMAGE_BLOOM_DOWNSAMPLE_OUTPUT_SLOT, output->texture.get());
        gm.Dispatch(work_group_x, work_group_y, 1);
        gm.SetUnorderedAccessView(IMAGE_BLOOM_DOWNSAMPLE_OUTPUT_SLOT, nullptr);
        gm.UnbindTexture(Dimension::TEXTURE_2D, g_bloom_input_image_slot);
    }

    // Step 2, down sampling
    gm.SetPipelineState(PROGRAM_BLOOM_DOWNSAMPLE);
    for (int i = 1; i < BLOOM_MIP_CHAIN_MAX; ++i) {
        auto input = gm.FindRenderTarget(static_cast<RenderTargetResourceName>(RESOURCE_BLOOM_0 + i - 1));
        auto output = gm.FindRenderTarget(static_cast<RenderTargetResourceName>(RESOURCE_BLOOM_0 + i));

        DEV_ASSERT(input && output);

        auto [width, height] = output->getSize();
        const uint32_t work_group_x = math::CeilingDivision(width, 16);
        const uint32_t work_group_y = math::CeilingDivision(height, 16);

        gm.BindTexture(Dimension::TEXTURE_2D, input->texture->GetHandle(), g_bloom_input_image_slot);
        gm.SetUnorderedAccessView(IMAGE_BLOOM_DOWNSAMPLE_OUTPUT_SLOT, output->texture.get());
        gm.Dispatch(work_group_x, work_group_y, 1);
        gm.SetUnorderedAccessView(IMAGE_BLOOM_DOWNSAMPLE_OUTPUT_SLOT, nullptr);
        gm.UnbindTexture(Dimension::TEXTURE_2D, g_bloom_input_image_slot);
    }

    // Step 3, up sampling
    gm.SetPipelineState(PROGRAM_BLOOM_UPSAMPLE);
    for (int i = BLOOM_MIP_CHAIN_MAX - 1; i > 0; --i) {
        auto input = gm.FindRenderTarget(static_cast<RenderTargetResourceName>(RESOURCE_BLOOM_0 + i));
        auto output = gm.FindRenderTarget(static_cast<RenderTargetResourceName>(RESOURCE_BLOOM_0 + i - 1));

        auto [width, height] = output->getSize();
        const uint32_t work_group_x = math::CeilingDivision(width, 16);
        const uint32_t work_group_y = math::CeilingDivision(height, 16);

        gm.BindTexture(Dimension::TEXTURE_2D, input->texture->GetHandle(), g_bloom_input_image_slot);
        gm.SetUnorderedAccessView(IMAGE_BLOOM_DOWNSAMPLE_OUTPUT_SLOT, output->texture.get());
        gm.Dispatch(work_group_x, work_group_y, 1);
        gm.SetUnorderedAccessView(IMAGE_BLOOM_DOWNSAMPLE_OUTPUT_SLOT, nullptr);
        gm.UnbindTexture(Dimension::TEXTURE_2D, g_bloom_input_image_slot);
    }
}

void RenderPassCreator::addBloomPass() {
    GraphicsManager& gm = GraphicsManager::GetSingleton();

    RenderPassDesc desc;
    desc.name = RenderPassName::BLOOM;
    desc.dependencies = { RenderPassName::LIGHTING };
    auto pass = m_graph.createPass(desc);

    int width = m_config.frame_width;
    int height = m_config.frame_height;
    for (int i = 0; i < BLOOM_MIP_CHAIN_MAX; ++i, width /= 2, height /= 2) {
        DEV_ASSERT(width > 1);
        DEV_ASSERT(height > 1);

        LOG_WARN("bloom size {}x{}", width, height);

        auto attachment = gm.CreateRenderTarget(RenderTargetDesc(static_cast<RenderTargetResourceName>(RESOURCE_BLOOM_0 + i),
                                                                 PixelFormat::R11G11B10_FLOAT,
                                                                 AttachmentType::COLOR_2D,
                                                                 width, height, false, true),
                                                linear_clamp_sampler());
    }

    auto draw_pass = gm.CreateDrawPass(DrawPassDesc{
        .color_attachments = {},
        .exec_func = bloomFunction,
    });
    pass->addDrawPass(draw_pass);
}

/// Tone
static void tonePassFunc(const DrawPass* p_draw_pass) {
    OPTICK_EVENT();

    GraphicsManager& gm = GraphicsManager::GetSingleton();
    gm.SetRenderTarget(p_draw_pass);

    DEV_ASSERT(!p_draw_pass->color_attachments.empty());
    auto depth_buffer = p_draw_pass->depth_attachment;
    auto [width, height] = p_draw_pass->color_attachments[0]->getSize();

    // draw billboards

    // HACK:
    if (DVAR_GET_BOOL(r_debug_vxgi) && gm.GetBackend() == Backend::OPENGL) {
        // @TODO: remove
        extern void debug_vxgi_pass_func(const DrawPass* p_draw_pass);
        debug_vxgi_pass_func(p_draw_pass);
    } else {
        auto bind_slot = [&](RenderTargetResourceName p_name, int p_slot, Dimension p_dimension = Dimension::TEXTURE_2D) {
            std::shared_ptr<RenderTarget> resource = gm.FindRenderTarget(p_name);
            if (!resource) {
                return;
            }

            gm.BindTexture(p_dimension, resource->texture->GetHandle(), p_slot);
        };
        bind_slot(RESOURCE_LIGHTING, g_texture_lighting_slot);
        bind_slot(RESOURCE_BLOOM_0, g_bloom_input_image_slot);

        gm.SetViewport(Viewport(width, height));
        gm.Clear(p_draw_pass, CLEAR_COLOR_BIT);

        gm.SetPipelineState(PROGRAM_TONE);
        RenderManager::GetSingleton().draw_quad();

        gm.UnbindTexture(Dimension::TEXTURE_2D, g_texture_lighting_slot);
        gm.UnbindTexture(Dimension::TEXTURE_2D, g_bloom_input_image_slot);
    }
}

void RenderPassCreator::addTonePass() {
    GraphicsManager& gm = GraphicsManager::GetSingleton();

    RenderPassDesc desc;
    desc.name = RenderPassName::TONE;
    desc.dependencies = { RenderPassName::BLOOM };

    auto pass = m_graph.createPass(desc);

    int width = m_config.frame_width;
    int height = m_config.frame_height;

    auto attachment = gm.CreateRenderTarget(RenderTargetDesc{ RESOURCE_TONE,
                                                              PixelFormat::R11G11B10_FLOAT,
                                                              AttachmentType::COLOR_2D,
                                                              width, height },
                                            nearest_sampler());

    auto gbuffer_depth = gm.FindRenderTarget(RESOURCE_GBUFFER_DEPTH);

    auto draw_pass = gm.CreateDrawPass(DrawPassDesc{
        .color_attachments = { attachment },
        .depth_attachment = gbuffer_depth,
        .exec_func = tonePassFunc,
    });
    pass->addDrawPass(draw_pass);
}

}  // namespace my::rg
