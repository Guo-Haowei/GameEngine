#pragma once
#include "engine/renderer/gpu_resource.h"
#include "engine/renderer/pixel_format.h"
#include "engine/renderer/render_graph/render_graph_defines.h"

namespace my::renderer {

class RenderGraph;

class RenderPassCreator {
public:
    struct Config {
        bool enableShadow = true;
        bool enablePointShadow = true;
        bool enableVxgi = true;
        bool enableIbl = true;
        bool enableBloom = true;
        bool enableHighlight = true;

        int frameWidth;
        int frameHeight;
    };

    RenderPassCreator(const Config& p_config, RenderGraph& p_graph)
        : m_config(p_config),
          m_graph(p_graph) {}

    static std::unique_ptr<RenderGraph> CreateDummy();
    static std::unique_ptr<RenderGraph> CreateDefault();
    static std::unique_ptr<RenderGraph> CreateExperimental();
    static std::unique_ptr<RenderGraph> CreatePathTracer();

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

    static GpuTextureDesc BuildDefaultTextureDesc(RenderTargetResourceName p_name,
                                                  PixelFormat p_format,
                                                  AttachmentType p_type,
                                                  uint32_t p_width,
                                                  uint32_t p_height,
                                                  uint32_t p_array_size = 1);

    Config m_config;
    RenderGraph& m_graph;
};

}  // namespace my::renderer
