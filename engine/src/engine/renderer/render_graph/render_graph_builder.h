#pragma once
#include "engine/renderer/gpu_resource.h"
#include "engine/renderer/pixel_format.h"

namespace my {
struct Framebuffer;
class IGraphicsManager;
}  // namespace my

namespace my::renderer {

struct RenderData;
struct RenderPassCreateInfo;
class RenderGraph;

struct RenderGraphBuilderConfig {
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

class RenderGraphBuilder {
public:
    RenderGraphBuilder(const RenderGraphBuilderConfig& p_config,
                       RenderGraph& p_graph) : m_config(p_config),
                                               m_graph(p_graph) {}

    // @TODO: make it more extendable
    static std::unique_ptr<RenderGraph> CreateEmpty(RenderGraphBuilderConfig& p_config);
    static std::unique_ptr<RenderGraph> CreateDummy(RenderGraphBuilderConfig& p_config);
    static std::unique_ptr<RenderGraph> CreateDefault(RenderGraphBuilderConfig& p_config);
    static std::unique_ptr<RenderGraph> CreatePathTracer(RenderGraphBuilderConfig& p_config);

    static void DrawDebugImages(const RenderData& p_data, int p_width, int p_height, IGraphicsManager& p_graphics_manager);

    static GpuTextureDesc BuildDefaultTextureDesc(RenderTargetResourceName p_name,
                                                  PixelFormat p_format,
                                                  AttachmentType p_type,
                                                  uint32_t p_width,
                                                  uint32_t p_height,
                                                  uint32_t p_array_size = 1,
                                                  ResourceMiscFlags p_misc_flag = RESOURCE_MISC_NONE,
                                                  uint32_t p_mips_level = 0);

    // @TODO: refactor
    static void CreateResources();

private:
    void AddForward();

    void AddPrepass();
    void AddGbufferPass();
    void AddHighlightPass();
    void AddShadowPass();
    void AddVoxelizationPass();
    void AddSsaoPass();
    void AddLightingPass();
    void AddSkyPass();
    void AddBloomPass();
    void AddTonePass();
    void AddGenerateSkylightPass();
    void AddPathTracerPass();
    void AddPathTracerTonePass();
    void AddDebugImagePass();

    void CreateRenderPass(RenderPassCreateInfo& p_info);

    RenderGraphBuilderConfig m_config;
    RenderGraph& m_graph;
};

}  // namespace my::renderer
