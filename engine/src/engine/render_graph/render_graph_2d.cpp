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

    const auto& tile = p_ctx.frameData.tiles;
    if (tile->m_tiles.size()) {
        const int w = tile->m_width;
        const int h = tile->m_height;
        std::vector<Vector2f> vertices;
        std::vector<Vector2f> uvs;
        std::vector<uint32_t> indices;
        vertices.reserve(w * h * 4);
        for (int y = 0; y < h; ++y) {
            for (int x = 0; x < w; ++x) {
                const float s = 1.0f;
                float x0 = s * x;
                float y0 = s * y;
                float x1 = s * (x + 1);
                float y1 = s * (y + 1);
                Vector2f bottom_left{ x0, y0 };
                Vector2f bottom_right{ x1, y0 };
                Vector2f top_left{ x0, y1 };
                Vector2f top_right{ x1, y1 };
                Vector2f uv0{ 0, 0 };
                Vector2f uv1{ 1, 0 };
                Vector2f uv2{ 0, 1 };
                Vector2f uv3{ 1, 1 };

                const uint32_t offset = (uint32_t)vertices.size();
                vertices.push_back(bottom_left);
                vertices.push_back(bottom_right);
                vertices.push_back(top_left);
                vertices.push_back(top_right);

                uvs.push_back(uv0);
                uvs.push_back(uv1);
                uvs.push_back(uv2);
                uvs.push_back(uv3);

                indices.push_back(0 + offset);
                indices.push_back(1 + offset);
                indices.push_back(3 + offset);

                indices.push_back(0 + offset);
                indices.push_back(3 + offset);
                indices.push_back(2 + offset);
            }
        }
        uint32_t count = (uint32_t)indices.size();

        GpuBufferDesc buffers[2];
        GpuBufferDesc buffer_desc;
        buffer_desc.type = GpuBufferType::VERTEX;
        buffer_desc.elementSize = sizeof(Vector2f);
        buffer_desc.elementCount = (uint32_t)vertices.size();
        buffer_desc.initialData = vertices.data();

        buffers[0] = buffer_desc;

        buffer_desc.initialData = uvs.data();
        buffers[1] = buffer_desc;

        GpuBufferDesc index_desc;
        index_desc.type = GpuBufferType::INDEX;
        index_desc.elementSize = sizeof(uint32_t);
        index_desc.elementCount = count;
        index_desc.initialData = indices.data();

        GpuMeshDesc desc;
        desc.drawCount = count;
        desc.enabledVertexCount = 2;
        desc.vertexLayout[0] = GpuMeshDesc::VertexLayout{ 0, sizeof(Vector2f), 0 };
        desc.vertexLayout[1] = GpuMeshDesc::VertexLayout{ 1, sizeof(Vector2f), 0 };

        auto mesh = *cmd.CreateMeshImpl(desc, 2, buffers, &index_desc);
        cmd.SetMesh(mesh.get());
        cmd.SetPipelineState(PSO_SPRITE);
        cmd.DrawElementsInstanced(1, count);
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
