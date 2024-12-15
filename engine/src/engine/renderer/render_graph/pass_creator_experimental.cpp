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

    glBindVertexArray(g_box->vao);

    CRASH_NOW();
    // const int size = DVAR_GET_INT(gfx_voxel_size);
    //  glDrawElementsInstanced(GL_TRIANGLES, g_box->indexCount, GL_UNSIGNED_INT, 0, size * size * size);
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
    g_debug_draw_cache.cache.c_debugDrawMap.Set64(p_handle);
    g_debug_draw_cache.update();
    GraphicsManager::GetSingleton().DrawQuad();
}

void final_pass_func(const DrawData&, const DrawPass* p_draw_pass) {
    OPTICK_EVENT();

    GraphicsManager::GetSingleton().SetRenderTarget(p_draw_pass);
    const auto [width, height] = p_draw_pass->GetBufferSize();

    glViewport(0, 0, width, height);
    glClear(GL_COLOR_BUFFER_BIT);

    GraphicsManager::GetSingleton().SetPipelineState(PSO_RW_TEXTURE_2D);

    // @TODO: clean up
    auto final_image_handle = GraphicsManager::GetSingleton().FindTexture(RESOURCE_TONE)->GetResidentHandle();
    debug_draw_quad(final_image_handle, DISPLAY_CHANNEL_RGB, width, height, width, height);

    if (DVAR_GET_BOOL(gfx_debug_shadow)) {
        auto shadow_map_handle = GraphicsManager::GetSingleton().FindTexture(RESOURCE_SHADOW_MAP)->GetResidentHandle();
        debug_draw_quad(shadow_map_handle, DISPLAY_CHANNEL_RRR, width, height, 300, 300);
    }
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
            .execFunc = final_pass_func,
        });
        pass->AddDrawPass(draw_pass);
    }

    // @TODO: allow recompile
    graph->Compile();
    return graph;
}

}  // namespace my::renderer
