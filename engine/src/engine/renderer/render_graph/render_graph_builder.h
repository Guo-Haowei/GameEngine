#pragma once
#include "engine/renderer/gpu_resource.h"
#include "engine/renderer/pixel_format.h"
#include "render_pass_builder.h"

// clang-format off
namespace my { struct Framebuffer; }
namespace my { class IGraphicsManager; }
namespace my::renderer { class RenderGraph; }
namespace my::renderer { struct RenderSystem; }
// clang-format on

namespace my::renderer {

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
    RenderGraphBuilder(const RenderGraphBuilderConfig& p_config, RenderGraph& p_graph);

    // @TODO: make it more extendable
    static std::unique_ptr<RenderGraph> CreateEmpty(RenderGraphBuilderConfig& p_config);
    static std::unique_ptr<RenderGraph> CreateDummy(RenderGraphBuilderConfig& p_config);
    static std::unique_ptr<RenderGraph> CreateDefault(RenderGraphBuilderConfig& p_config);
    static std::unique_ptr<RenderGraph> CreatePathTracer(RenderGraphBuilderConfig& p_config);

    static void DrawDebugImages(const RenderSystem& p_data, int p_width, int p_height, IGraphicsManager& p_graphics_manager);

    GpuTextureDesc BuildDefaultTextureDesc(RenderTargetResourceName p_name,
                                           PixelFormat p_format,
                                           AttachmentType p_type,
                                           uint32_t p_width,
                                           uint32_t p_height,
                                           uint32_t p_array_size = 1,
                                           ResourceMiscFlags p_misc_flag = RESOURCE_MISC_NONE,
                                           uint32_t p_mips_level = 0);

    GpuTextureDesc BuildDefaultTextureDesc(RenderTargetResourceName p_name,
                                           PixelFormat p_format,
                                           AttachmentType p_type,
                                           uint32_t p_array_size = 1,
                                           ResourceMiscFlags p_misc_flag = RESOURCE_MISC_NONE,
                                           uint32_t p_mips_level = 0) {
        return BuildDefaultTextureDesc(p_name, p_format, p_type, m_frameWidth, m_frameHeight, p_array_size, p_misc_flag, p_mips_level);
    }

    RenderPassBuilder& AddPass(std::string_view p_name);
    void AddDependency(std::string_view p_from, std::string_view p_to);

    [[nodiscard]] auto Compile() -> Result<void>;

private:
    void AddEmpty();

    void AddEarlyZPass();
    void AddGbufferPass();
    void AddHighlightPass();
    void AddShadowPass();
    void AddVoxelizationPass();
    void AddSsaoPass();
    void AddLightingPass();
    void AddForwardPass();
    void AddBloomPass();
    void AddTonePass();
    void AddDebugImagePass();

#if 0
    void AddGenerateSkylightPass();
    void AddPathTracerPass();
    void AddPathTracerTonePass();
#endif

    RenderGraphBuilderConfig m_config;
    RenderGraph& m_graph;
    IGraphicsManager& m_graphicsManager;

    std::vector<RenderPassBuilder> m_passes;
    std::vector<std::pair<std::string_view, std::string_view>> m_dependencies;
    uint32_t m_frameWidth;
    uint32_t m_frameHeight;
};

}  // namespace my::renderer
