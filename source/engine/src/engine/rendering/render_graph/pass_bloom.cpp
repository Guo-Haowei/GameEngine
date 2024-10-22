#include "core/debugger/profiler.h"
#include "core/framework/graphics_manager.h"
#include "rendering/render_graph/pass_creator.h"
#include "rendering/rendering_dvars.h"

// @TODO: remove API sepcific code
#include "drivers/opengl/opengl_prerequisites.h"
#include "rendering/render_manager.h"

namespace my::rg {

static void downSampleFunc(const DrawPass*) {
    GraphicsManager& gm = GraphicsManager::singleton();

    const bool is_opengl = gm.getBackend() == Backend::OPENGL;

    // Step 1, select pixels contribute to bloom
    {
        gm.setPipelineState(PROGRAM_BLOOM_SETUP);
        auto input = gm.findRenderTarget(RESOURCE_LIGHTING);
        auto output = gm.findRenderTarget(RESOURCE_BLOOM_0);

        auto [width, height] = input->getSize();
        const uint32_t work_group_x = math::ceilingDivision(width, 16);
        const uint32_t work_group_y = math::ceilingDivision(height, 16);

        g_bloom_cache.cache.g_bloom_input = input->texture->get_resident_handle();
        g_bloom_cache.update();

        gm.setUnorderedAccessView(IMAGE_BLOOM_DOWNSAMPLE_OUTPUT_SLOT, output->texture.get());
        gm.dispatch(work_group_x, work_group_y, 1);
        if (is_opengl) {
            glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
        }
    }

    // Step 2, down sampling
    gm.setPipelineState(PROGRAM_BLOOM_DOWNSAMPLE);
    for (int i = 1; i < BLOOM_MIP_CHAIN_MAX; ++i) {
        auto input = gm.findRenderTarget(static_cast<RenderTargetResourceName>(RESOURCE_BLOOM_0 + i - 1));
        auto output = gm.findRenderTarget(static_cast<RenderTargetResourceName>(RESOURCE_BLOOM_0 + i));

        DEV_ASSERT(input && output);

        g_bloom_cache.cache.g_bloom_input = input->texture->get_resident_handle();
        g_bloom_cache.update();

        auto [width, height] = output->getSize();
        const uint32_t work_group_x = math::ceilingDivision(width, 16);
        const uint32_t work_group_y = math::ceilingDivision(height, 16);

        // @TODO: refactor image slot
        gm.setUnorderedAccessView(IMAGE_BLOOM_DOWNSAMPLE_OUTPUT_SLOT, output->texture.get());
        gm.dispatch(work_group_x, work_group_y, 1);
        if (is_opengl) {
            glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
        }
    }

    // Step 3, up sampling
    gm.setPipelineState(PROGRAM_BLOOM_UPSAMPLE);
    for (int i = BLOOM_MIP_CHAIN_MAX - 1; i > 0; --i) {
        auto input = gm.findRenderTarget(static_cast<RenderTargetResourceName>(RESOURCE_BLOOM_0 + i));
        auto output = gm.findRenderTarget(static_cast<RenderTargetResourceName>(RESOURCE_BLOOM_0 + i - 1));

        g_bloom_cache.cache.g_bloom_input = input->texture->get_resident_handle();
        g_bloom_cache.update();

        auto [width, height] = output->getSize();
        const uint32_t work_group_x = math::ceilingDivision(width, 16);
        const uint32_t work_group_y = math::ceilingDivision(height, 16);

        gm.setUnorderedAccessView(IMAGE_BLOOM_DOWNSAMPLE_OUTPUT_SLOT, output->texture.get());
        gm.dispatch(work_group_x, work_group_y, 1);
        if (is_opengl) {
            glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
        }
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

        auto attachment = gm.createRenderTarget(RenderTargetDesc(static_cast<RenderTargetResourceName>(RESOURCE_BLOOM_0 + i),
                                                                 PixelFormat::R11G11B10_FLOAT,
                                                                 AttachmentType::COLOR_2D,
                                                                 width, height, false, true),
                                                linear_clamp_sampler());
    }

    auto draw_pass = gm.createDrawPass(DrawPassDesc{
        .color_attachments = {},
        .exec_func = downSampleFunc,
    });
    pass->addDrawPass(draw_pass);
}

}  // namespace my::rg
