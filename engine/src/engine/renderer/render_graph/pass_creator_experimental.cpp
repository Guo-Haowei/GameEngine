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
#include "engine/drivers/opengl/opengl_prerequisites.h"

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

    creator.AddGenerateSkylightPass();
    creator.AddShadowPass();
    creator.AddGbufferPass();
    creator.AddHighlightPass();
    creator.AddVoxelizationPass();
    creator.AddLightingPass();
    creator.AddEmitterPass();
    creator.AddBloomPass();
    creator.AddTonePass();
    creator.AddDebugImagePass();

    // @TODO: allow recompile
    graph->Compile();
    return graph;
}

}  // namespace my::renderer
