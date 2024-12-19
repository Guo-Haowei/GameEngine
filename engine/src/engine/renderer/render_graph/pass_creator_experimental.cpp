#include "engine/core/debugger/profiler.h"
#include "engine/core/framework/graphics_manager.h"
#include "engine/core/framework/scene_manager.h"
#include "engine/core/math/frustum.h"
#include "engine/core/math/matrix_transform.h"
#include "engine/renderer/draw_data.h"
#include "engine/renderer/graphics_dvars.h"
#include "engine/renderer/pipeline_state.h"
#include "engine/renderer/render_graph/pass_creator.h"
#include "shader_resource_defines.hlsl.h"

// @TODO: refactor
#include "engine/drivers/opengl/opengl_graphics_manager.h"
#include "engine/drivers/opengl/opengl_prerequisites.h"

extern my::OpenGlMeshBuffers* g_box;

namespace my::renderer {

void debug_vxgi_pass_func(const DrawData& p_data, const DrawPass* p_draw_pass) {
    OPTICK_EVENT();

    GraphicsManager& gm = GraphicsManager::GetSingleton();
    gm.SetRenderTarget(p_draw_pass);
    auto depth_buffer = p_draw_pass->desc.depthAttachment;
    const auto [width, height] = p_draw_pass->GetBufferSize();

    glEnable(GL_BLEND);
    gm.SetViewport(Viewport(width, height));
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    GraphicsManager::GetSingleton().SetPipelineState(PSO_DEBUG_VOXEL);

    const PassContext& pass = p_data.mainPass;
    gm.BindConstantBufferSlot<PerPassConstantBuffer>(gm.GetCurrentFrame().passCb.get(), pass.pass_idx);

    gm.SetMesh(gm.m_boxBuffers.get());
    const uint32_t size = DVAR_GET_INT(gfx_voxel_size);
    gm.DrawElementsInstanced(size * size * size, gm.m_boxBuffers->desc.drawCount);

    glDisable(GL_BLEND);
}

std::unique_ptr<RenderGraph> RenderPassCreator::CreateExperimental() {
    // @TODO: early-z
    const NewVector2i frame_size = DVAR_GET_IVEC2(resolution);
    const int w = frame_size.x;
    const int h = frame_size.y;

    RenderPassCreator::Config config;
    config.frameWidth = w;
    config.frameHeight = h;

    auto graph = std::make_unique<RenderGraph>();
    RenderPassCreator creator(config, *graph.get());

    GraphicsManager& manager = GraphicsManager::GetSingleton();

    auto final_attachment = manager.CreateTexture(BuildDefaultTextureDesc(RESOURCE_FINAL,
                                                                          PixelFormat::R8G8B8A8_UINT,
                                                                          AttachmentType::COLOR_2D,
                                                                          w, h),
                                                  PointClampSampler());

    creator.AddGenerateSkylightPass();
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
        auto pass = graph->CreatePass(desc);
        auto draw_pass = manager.CreateDrawPass(DrawPassDesc{
            .colorAttachments = { final_attachment },
            .execFunc = [](const DrawData& p_data, const DrawPass* p_draw_pass) {
                OPTICK_EVENT();

                auto& gm = GraphicsManager::GetSingleton();
                auto& frame = gm.GetCurrentFrame();
                const uint32_t width = p_draw_pass->desc.depthAttachment->desc.width;
                const uint32_t height = p_draw_pass->desc.depthAttachment->desc.height;

                gm.SetRenderTarget(p_draw_pass);
                gm.SetViewport(Viewport(width, height));
                gm.Clear(p_draw_pass, CLEAR_COLOR_BIT);
                gm.SetPipelineState(PSO_RW_TEXTURE_2D);

                for (int i = 0; i < (int)p_data.drawImageContext.size(); ++i) {
                    const auto& data = p_data.drawImageContext[i];
                    gm.BindConstantBufferSlot<PerBatchConstantBuffer>(frame.batchCb.get(), i);
                    gm.DrawQuad();
                }
            } });
        pass->AddDrawPass(draw_pass);
    }

    // @TODO: allow recompile
    graph->Compile();
    return graph;
}

}  // namespace my::renderer
