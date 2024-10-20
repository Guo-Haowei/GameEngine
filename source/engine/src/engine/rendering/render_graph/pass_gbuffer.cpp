#include "core/debugger/profiler.h"
#include "core/framework/graphics_manager.h"
#include "rendering/render_graph/pass_creator.h"

namespace my::rg {

static void gbufferPassFunc(const DrawPass* p_draw_pass) {
    OPTICK_EVENT();

    auto& gm = GraphicsManager::singleton();
    auto& ctx = gm.getContext();
    auto [width, height] = p_draw_pass->depth_attachment->getSize();

    gm.setRenderTarget(p_draw_pass);

    Viewport viewport{ width, height };
    gm.setViewport(viewport);

    float clear_color[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    gm.clear(p_draw_pass, CLEAR_COLOR_BIT | CLEAR_DEPTH_BIT | CLEAR_STENCIL_BIT, clear_color);

    PassContext& pass = gm.main_pass;
    gm.bindUniformSlot<PerPassConstantBuffer>(ctx.pass_uniform.get(), pass.pass_idx);

    for (const auto& draw : pass.draws) {
        bool has_bone = draw.bone_idx >= 0;
        if (has_bone) {
            gm.bindUniformSlot<BoneConstantBuffer>(ctx.bone_uniform.get(), draw.bone_idx);
        }

        gm.setPipelineState(has_bone ? PROGRAM_GBUFFER_ANIMATED : PROGRAM_GBUFFER_STATIC);

        if (draw.flags) {
            gm.setStencilRef(draw.flags);
        }

        gm.bindUniformSlot<PerBatchConstantBuffer>(ctx.batch_uniform.get(), draw.batch_idx);

        gm.setMesh(draw.mesh_data);

        for (const auto& subset : draw.subsets) {
            const MaterialConstantBuffer& material = gm.m_context.material_cache.buffer[subset.material_idx];
            gm.bindTexture(Dimension::TEXTURE_2D, material.u_base_color_map_handle, u_base_color_map_slot);
            gm.bindTexture(Dimension::TEXTURE_2D, material.u_normal_map_handle, u_normal_map_slot);
            gm.bindTexture(Dimension::TEXTURE_2D, material.u_material_map_handle, u_material_map_slot);

            gm.bindUniformSlot<MaterialConstantBuffer>(ctx.material_uniform.get(), subset.material_idx);

            // @TODO: set material

            gm.drawElements(subset.index_count, subset.index_offset);

            // @TODO: unbind
        }

        if (draw.flags) {
            gm.setStencilRef(0);
        }
    }
}

void RenderPassCreator::addGBufferPass() {
    GraphicsManager& manager = GraphicsManager::singleton();

    int p_width = m_config.frame_width;
    int p_height = m_config.frame_height;

    // @TODO: decouple sampler and render target
    auto gbuffer_depth = manager.createRenderTarget(RenderTargetDesc(RESOURCE_GBUFFER_DEPTH,
                                                                     PixelFormat::D24_UNORM_S8_UINT,
                                                                     AttachmentType::DEPTH_STENCIL_2D,
                                                                     p_width, p_height),
                                                    nearest_sampler());

    auto attachment0 = manager.createRenderTarget(RenderTargetDesc{ RESOURCE_GBUFFER_BASE_COLOR,
                                                                    PixelFormat::R11G11B10_FLOAT,
                                                                    AttachmentType::COLOR_2D,
                                                                    p_width, p_height },
                                                  nearest_sampler());

    auto attachment1 = manager.createRenderTarget(RenderTargetDesc{ RESOURCE_GBUFFER_POSITION,
                                                                    PixelFormat::R16G16B16_FLOAT,
                                                                    AttachmentType::COLOR_2D,
                                                                    p_width, p_height },
                                                  nearest_sampler());

    auto attachment2 = manager.createRenderTarget(RenderTargetDesc{ RESOURCE_GBUFFER_NORMAL,
                                                                    PixelFormat::R16G16B16_FLOAT,
                                                                    AttachmentType::COLOR_2D,
                                                                    p_width, p_height },
                                                  nearest_sampler());

    auto attachment3 = manager.createRenderTarget(RenderTargetDesc{ RESOURCE_GBUFFER_MATERIAL,
                                                                    PixelFormat::R11G11B10_FLOAT,
                                                                    AttachmentType::COLOR_2D,
                                                                    p_width, p_height },
                                                  nearest_sampler());

    RenderPassDesc desc;
    desc.name = RenderPassName::GBUFFER;
    auto pass = m_graph.createPass(desc);
    auto draw_pass = manager.createDrawPass(DrawPassDesc{
        .color_attachments = { attachment0, attachment1, attachment2, attachment3 },
        .depth_attachment = gbuffer_depth,
        .exec_func = gbufferPassFunc,
    });
    pass->addDrawPass(draw_pass);
}

}  // namespace my::rg
