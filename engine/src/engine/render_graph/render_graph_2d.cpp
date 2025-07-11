#include "engine/core/debugger/profiler.h"
#include "engine/render_graph/render_graph.h"
#include "engine/render_graph/render_graph_builder.h"
#include "engine/renderer/frame_data.h"
#include "engine/renderer/graphics_manager.h"
#include "engine/renderer/pipeline_state.h"
#include "engine/scene/scene_component.h"
#include "render_graph_predefined.h"

namespace my {

static void Pass2DDrawFunc(RenderPassExcutionContext& p_ctx) {
    HBN_PROFILE_EVENT();
    auto& cmd = p_ctx.cmd;
    Framebuffer* fb = p_ctx.framebuffer;
    const uint32_t width = fb->desc.depthAttachment->desc.width;
    const uint32_t height = fb->desc.depthAttachment->desc.height;

    cmd.SetRenderTarget(fb);
    cmd.SetViewport(Viewport(width, height));
    float clear_color[] = { 0.3f, 0.3f, 0.3f, 1.0f };
    cmd.Clear(fb, CLEAR_COLOR_BIT | CLEAR_DEPTH_BIT, clear_color, 0.0f);

    auto& frame = cmd.GetCurrentFrame();
    const PassContext& pass = p_ctx.frameData.mainPass;
    cmd.BindConstantBufferSlot<PerPassConstantBuffer>(frame.passCb.get(), pass.pass_idx);

    cmd.SetPipelineState(PSO_SPRITE);

    for (const RenderCommand& render_cmd : p_ctx.frameData.tile_maps) {
        const DrawCommand& draw = render_cmd.draw;
        const auto tile = draw.mesh_data;
        if (draw.texture) {
            cmd.BindTexture(Dimension::TEXTURE_2D, draw.texture->GetHandle(), 0);
        }
        cmd.SetMesh(tile);
        cmd.BindConstantBufferSlot<PerBatchConstantBuffer>(frame.batchCb.get(), draw.batch_idx);
        cmd.DrawElementsInstanced(1, draw.indexCount);
    }
}

auto RenderGraph2D(RenderGraphBuilderConfig& p_config) -> Result<std::shared_ptr<RenderGraph>> {
    p_config.enableBloom = false;
    p_config.enableIbl = false;
    p_config.enableVxgi = false;

    RenderGraphBuilder builder(p_config);

    auto color_desc = builder.BuildDefaultTextureDesc(DEFAULT_SURFACE_FORMAT,
                                                      AttachmentType::COLOR_2D);
    color_desc.bindFlags |= BIND_SHADER_RESOURCE;

    auto depth_desc = builder.BuildDefaultTextureDesc(RT_FMT_GBUFFER_DEPTH,
                                                      AttachmentType::DEPTH_STENCIL_2D);

    auto& pass = builder.AddPass(RG_PASS_2D);
    pass.Create(RG_RES_POST_PROCESS, { color_desc })
        .Create(RG_RES_DEPTH_STENCIL, { depth_desc })
        .Write(ResourceAccess::RTV, RG_RES_POST_PROCESS)
        .Write(ResourceAccess::DSV, RG_RES_DEPTH_STENCIL)
        .SetExecuteFunc(Pass2DDrawFunc);

    return builder.Compile();
}

}  // namespace my
