#pragma once
#include "rendering/gpu_resource.h"
#include "rendering/pixel_format.h"
#include "rendering/render_graph/render_graph_defines.h"

// @TODO: refactor this
namespace my::rg {

class RenderGraph;

class RenderPassCreator {
public:
    struct Config {
        bool enableShadow = true;
        bool enablePointShadow = true;
        bool enableVxgi = true;
        bool enableIbl = true;
        bool enableBloom = true;

        int frameWidth;
        int frameHeight;
    };

    RenderPassCreator(const Config& p_config, RenderGraph& p_graph)
        : m_config(p_config),
          m_graph(p_graph) {}

    static void CreateDummy(RenderGraph& p_graph);
    static void CreateDefault(RenderGraph& p_graph);
    static void CreateVxgi(RenderGraph& p_graph);
    // @TODO: add this
    static void CreatePathTracer(RenderGraph& p_graph);

private:
    void AddGbufferPass();
    void AddHighlightPass();
    void AddShadowPass();
    void AddVoxelizationPass();
    void AddLightingPass();
    void AddEmitterPass();
    void AddBloomPass();
    void AddTonePass();

    static GpuTextureDesc BuildDefaultTextureDesc(RenderTargetResourceName p_name,
                                                  PixelFormat p_format,
                                                  AttachmentType p_type,
                                                  uint32_t p_width,
                                                  uint32_t p_height,
                                                  uint32_t p_array_size = 1);

    Config m_config;
    RenderGraph& m_graph;
};

}  // namespace my::rg
