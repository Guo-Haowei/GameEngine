#pragma once
#include "engine/renderer/gpu_resource.h"
#include "engine/renderer/pixel_format.h"
#include "engine/renderer/render_graph/render_graph_defines.h"

namespace my {
struct Framebuffer;
class GraphicsManager;
}  // namespace my

namespace my::renderer {

struct DrawData;
class RenderGraph;

struct PassCreatorConfig {
    bool enableShadow = true;
    bool enablePointShadow = true;
    bool enableVxgi = true;
    bool enableIbl = true;
    bool enableBloom = true;
    bool enableHighlight = true;

    bool is_runtime;
    int frameWidth;
    int frameHeight;
};

class RenderPassCreator {
public:
    RenderPassCreator(const PassCreatorConfig& p_config, RenderGraph& p_graph)
        : m_config(p_config),
          m_graph(p_graph) {}

    static std::unique_ptr<RenderGraph> CreateDummy(PassCreatorConfig& p_config);
    static std::unique_ptr<RenderGraph> CreateDefault(PassCreatorConfig& p_config);
    static std::unique_ptr<RenderGraph> CreateExperimental(PassCreatorConfig& p_config);
    static std::unique_ptr<RenderGraph> CreatePathTracer(PassCreatorConfig& p_config);

    static void DrawDebugImages(const DrawData& p_data, int p_width, int p_height, GraphicsManager& p_graphics_manager);

private:
    void AddGbufferPass();
    void AddHighlightPass();
    void AddShadowPass();
    void AddVoxelizationPass();
    void AddLightingPass();
    void AddEmitterPass();
    void AddBloomPass();
    void AddTonePass();
    void AddGenerateSkylightPass();
    void AddPathTracerPass();
    void AddPathTracerTonePass();
    void AddDebugImagePass();

    static GpuTextureDesc BuildDefaultTextureDesc(RenderTargetResourceName p_name,
                                                  PixelFormat p_format,
                                                  AttachmentType p_type,
                                                  uint32_t p_width,
                                                  uint32_t p_height,
                                                  uint32_t p_array_size = 1);

    PassCreatorConfig m_config;
    RenderGraph& m_graph;
};

}  // namespace my::renderer
