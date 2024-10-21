#include "core/debugger/profiler.h"
#include "core/framework/graphics_manager.h"
#include "rendering/render_graph/pass_creator.h"
#include "rendering/rendering_dvars.h"

// @TODO: remove API sepcific code
#include "drivers/opengl/opengl_prerequisites.h"
#include "rendering/render_manager.h"

namespace my::rg {

// @TODO: remove
extern void debug_vxgi_pass_func(const DrawPass* p_draw_pass);

// @TODO: refactor to math
static int divideAndRoundup(int p_dividend, int p_divisor) {
    return (p_dividend + p_divisor - 1) / p_divisor;
}

static void tonePassFunc(const DrawPass* p_draw_pass) {
    OPTICK_EVENT();

    GraphicsManager& gm = GraphicsManager::singleton();
    gm.setRenderTarget(p_draw_pass);

    DEV_ASSERT(!p_draw_pass->color_attachments.empty());
    auto depth_buffer = p_draw_pass->depth_attachment;
    auto [width, height] = p_draw_pass->color_attachments[0]->getSize();

    // draw billboards

    // HACK:
    if (DVAR_GET_BOOL(r_debug_vxgi) && gm.getBackend() == Backend::OPENGL) {
        debug_vxgi_pass_func(p_draw_pass);
    } else {
        auto bind_slot = [&](RenderTargetResourceName p_name, int p_slot, Dimension p_dimension = Dimension::TEXTURE_2D) {
            std::shared_ptr<RenderTarget> resource = gm.findRenderTarget(p_name);
            if (!resource) {
                return;
            }

            gm.bindTexture(p_dimension, resource->texture->get_handle(), p_slot);
        };
        bind_slot(RESOURCE_LIGHTING, g_texture_lighting_slot);

        gm.setViewport(Viewport(width, height));
        gm.clear(p_draw_pass, CLEAR_COLOR_BIT);

        gm.setPipelineState(PROGRAM_TONE);
        RenderManager::singleton().draw_quad();

        gm.unbindTexture(Dimension::TEXTURE_2D, g_texture_lighting_slot);
    }
}

static void downSampleFunc(const DrawPass*) {
    GraphicsManager& gm = GraphicsManager::singleton();

    // Step 1, select pixels contribute to bloom
    {
        gm.setPipelineState(PROGRAM_BLOOM_SETUP);
        auto input = gm.findRenderTarget(RESOURCE_LIGHTING);
        auto output = gm.findRenderTarget(RESOURCE_BLOOM_0);

        auto [width, height] = input->getSize();
        const uint32_t work_group_x = divideAndRoundup(width, 16);
        const uint32_t work_group_y = divideAndRoundup(height, 16);

        g_bloom_cache.cache.g_bloom_input = input->texture->get_resident_handle();
        g_bloom_cache.update();
        glBindImageTexture(IMAGE_BLOOM_DOWNSAMPLE_OUTPUT_SLOT, output->texture->get_handle32(), 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_R11F_G11F_B10F);

        glDispatchCompute(work_group_x, work_group_y, 1);
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
    }

    // Step 2, down sampling
    gm.setPipelineState(PROGRAM_BLOOM_DOWNSAMPLE);
    for (int i = 1; i < BLOOM_MIP_CHAIN_MAX; ++i) {
        auto input = gm.findRenderTarget(static_cast<RenderTargetResourceName>(RESOURCE_BLOOM_0 + i - 1));
        auto output = gm.findRenderTarget(static_cast<RenderTargetResourceName>(RESOURCE_BLOOM_0 + i));

        DEV_ASSERT(input && output);

        g_bloom_cache.cache.g_bloom_input = input->texture->get_resident_handle();
        g_bloom_cache.update();
        // @TODO: refactor image slot
        glBindImageTexture(IMAGE_BLOOM_DOWNSAMPLE_OUTPUT_SLOT, output->texture->get_handle32(), 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_R11F_G11F_B10F);

        auto [width, height] = output->getSize();
        const uint32_t work_group_x = divideAndRoundup(width, 16);
        const uint32_t work_group_y = divideAndRoundup(height, 16);
        glDispatchCompute(work_group_x, work_group_y, 1);
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
    }

    // Step 3, up sampling
    gm.setPipelineState(PROGRAM_BLOOM_UPSAMPLE);
    for (int i = BLOOM_MIP_CHAIN_MAX - 1; i > 0; --i) {
        auto input = gm.findRenderTarget(static_cast<RenderTargetResourceName>(RESOURCE_BLOOM_0 + i));
        auto output = gm.findRenderTarget(static_cast<RenderTargetResourceName>(RESOURCE_BLOOM_0 + i - 1));

        g_bloom_cache.cache.g_bloom_input = input->texture->get_resident_handle();
        g_bloom_cache.update();

        glBindImageTexture(IMAGE_BLOOM_DOWNSAMPLE_OUTPUT_SLOT, output->texture->get_handle32(), 0, GL_TRUE, 0, GL_READ_WRITE, GL_R11F_G11F_B10F);

        auto [width, height] = output->getSize();
        const uint32_t work_group_x = divideAndRoundup(width, 16);
        const uint32_t work_group_y = divideAndRoundup(height, 16);
        glDispatchCompute(work_group_x, work_group_y, 1);
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
    }
}

void RenderPassCreator::addBloomPass() {
    GraphicsManager& gm = GraphicsManager::singleton();

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

        auto attachment = gm.createRenderTarget(RenderTargetDesc{ static_cast<RenderTargetResourceName>(RESOURCE_BLOOM_0 + i),
                                                                  PixelFormat::R11G11B10_FLOAT,
                                                                  AttachmentType::COLOR_2D,
                                                                  width, height },
                                                linear_clamp_sampler());
    }

    auto draw_pass = gm.createDrawPass(DrawPassDesc{
        .color_attachments = {},
        .exec_func = downSampleFunc,
    });
    pass->addDrawPass(draw_pass);
}

void RenderPassCreator::addTonePass(bool p_skip_bloom) {
    GraphicsManager& gm = GraphicsManager::singleton();

    RenderPassDesc desc;
    desc.name = RenderPassName::TONE;
    desc.dependencies = { RenderPassName::BLOOM };

    // HACK:
    if (p_skip_bloom) {
        desc.dependencies = { RenderPassName::LIGHTING };
    }

    auto pass = m_graph.createPass(desc);

    int width = m_config.frame_width;
    int height = m_config.frame_height;

    auto attachment = gm.createRenderTarget(RenderTargetDesc{ RESOURCE_TONE,
                                                              PixelFormat::R11G11B10_FLOAT,
                                                              AttachmentType::COLOR_2D,
                                                              width, height },
                                            nearest_sampler());

    auto gbuffer_depth = gm.findRenderTarget(RESOURCE_GBUFFER_DEPTH);

    auto draw_pass = gm.createDrawPass(DrawPassDesc{
        .color_attachments = { attachment },
        .depth_attachment = gbuffer_depth,
        .exec_func = tonePassFunc,
    });
    pass->addDrawPass(draw_pass);
}

}  // namespace my::rg
