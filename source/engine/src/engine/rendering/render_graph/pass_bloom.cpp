#include "core/debugger/profiler.h"
#include "core/framework/graphics_manager.h"
#include "rendering/render_data.h"
#include "rendering/render_graph/pass_creator.h"

// @TODO: remove API sepcific code
#include "drivers/opengl/opengl_prerequisites.h"

namespace my::rg {

// @TODO: refactor to math
static int divide_and_roundup(int p_dividend, int p_divisor) {
    return (p_dividend + p_divisor - 1) / p_divisor;
}

static void down_sample_func(const Subpass*) {
    GraphicsManager& manager = GraphicsManager::singleton();

    // Step 1, select pixels contribute to bloom
    {
        manager.setPipelineState(PROGRAM_BLOOM_SETUP);
        auto input = manager.findRenderTarget(RT_RES_LIGHTING);
        auto output = manager.findRenderTarget(RT_RES_BLOOM "_0");

        auto [width, height] = input->get_size();
        const uint32_t work_group_x = divide_and_roundup(width, 16);
        const uint32_t work_group_y = divide_and_roundup(height, 16);

        g_per_pass_cache.cache.u_tmp_bloom_input = input->texture->get_resident_handle();
        g_per_pass_cache.update();
        glBindImageTexture(IMAGE_BLOOM_DOWNSAMPLE_OUTPUT_SLOT, output->texture->get_handle32(), 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_R11F_G11F_B10F);

        glDispatchCompute(work_group_x, work_group_y, 1);
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
    }

    // Step 2, down sampling
    manager.setPipelineState(PROGRAM_BLOOM_DOWNSAMPLE);
    for (int i = 1; i < BLOOM_MIP_CHAIN_MAX; ++i) {
        auto input = manager.findRenderTarget(std::format("{}_{}", RT_RES_BLOOM, i - 1));
        auto output = manager.findRenderTarget(std::format("{}_{}", RT_RES_BLOOM, i));
        DEV_ASSERT(input && output);

        g_per_pass_cache.cache.u_tmp_bloom_input = input->texture->get_resident_handle();
        g_per_pass_cache.update();
        // @TODO: refactor image slot
        glBindImageTexture(IMAGE_BLOOM_DOWNSAMPLE_OUTPUT_SLOT, output->texture->get_handle32(), 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_R11F_G11F_B10F);

        auto [width, height] = output->get_size();
        const uint32_t work_group_x = divide_and_roundup(width, 16);
        const uint32_t work_group_y = divide_and_roundup(height, 16);
        glDispatchCompute(work_group_x, work_group_y, 1);
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
    }

    // Step 3, up sampling
    manager.setPipelineState(PROGRAM_BLOOM_UPSAMPLE);
    for (int i = BLOOM_MIP_CHAIN_MAX - 1; i > 0; --i) {
        auto input = manager.findRenderTarget(std::format("{}_{}", RT_RES_BLOOM, i));
        auto output = manager.findRenderTarget(std::format("{}_{}", RT_RES_BLOOM, i - 1));

        g_per_pass_cache.cache.u_tmp_bloom_input = input->texture->get_resident_handle();
        g_per_pass_cache.update();

        glBindImageTexture(IMAGE_BLOOM_DOWNSAMPLE_OUTPUT_SLOT, output->texture->get_handle32(), 0, GL_TRUE, 0, GL_READ_WRITE, GL_R11F_G11F_B10F);

        auto [width, height] = output->get_size();
        const uint32_t work_group_x = divide_and_roundup(width, 16);
        const uint32_t work_group_y = divide_and_roundup(height, 16);
        glDispatchCompute(work_group_x, work_group_y, 1);
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
    }
}

void RenderPassCreator::add_bloom_pass() {
    GraphicsManager& manager = GraphicsManager::singleton();

    RenderPassDesc desc;
    desc.name = BLOOM_PASS;
    desc.dependencies = { LIGHTING_PASS };
    auto pass = m_graph.create_pass(desc);

    int width = m_config.frame_width;
    int height = m_config.frame_height;
    for (int i = 0; i < BLOOM_MIP_CHAIN_MAX; ++i, width /= 2, height /= 2) {
        DEV_ASSERT(width > 1);
        DEV_ASSERT(height > 1);

        LOG_WARN("bloom size {}x{}", width, height);

        auto attachment = manager.createRenderTarget(RenderTargetDesc{ std::format("{}_{}", RT_RES_BLOOM, i),
                                                                       PixelFormat::R11G11B10_FLOAT,
                                                                       AttachmentType::COLOR_2D,
                                                                       width, height },
                                                     linear_clamp_sampler());
    }

    auto subpass = manager.createSubpass(SubpassDesc{
        .color_attachments = {},
        .func = down_sample_func,
    });
    pass->add_sub_pass(subpass);
}

}  // namespace my::rg
