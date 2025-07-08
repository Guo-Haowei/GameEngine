#pragma once
#include "render_graph_builder.h"

namespace my {

class RenderGraphBuilderExt : public RenderGraphBuilder {
public:
// @TODO: make it more extendable
#if 0
    static std::unique_ptr<RenderGraph> CreateDummy(RenderGraphBuilderConfig& p_config);
#endif

    // @TODO: create 2D
    // @TODO: create 3D cel

    [[nodiscard]] static auto CreateEmpty(RenderGraphBuilderConfig& p_config) -> Result<std::shared_ptr<RenderGraph>>;
    [[nodiscard]] static auto CreateDefault(RenderGraphBuilderConfig& p_config) -> Result<std::shared_ptr<RenderGraph>>;
    [[nodiscard]] static auto CreatePathTracer(RenderGraphBuilderConfig& p_config) -> Result<std::shared_ptr<RenderGraph>>;

    GpuTextureDesc BuildDefaultTextureDesc(PixelFormat p_format,
                                           AttachmentType p_type,
                                           uint32_t p_width,
                                           uint32_t p_height,
                                           uint32_t p_array_size = 1,
                                           ResourceMiscFlags p_misc_flag = RESOURCE_MISC_NONE,
                                           uint32_t p_mips_level = 0);

    GpuTextureDesc BuildDefaultTextureDesc(PixelFormat p_format,
                                           AttachmentType p_type,
                                           uint32_t p_array_size = 1,
                                           ResourceMiscFlags p_misc_flag = RESOURCE_MISC_NONE,
                                           uint32_t p_mips_level = 0) {

        return BuildDefaultTextureDesc(p_format,
                                       p_type,
                                       m_config.frameWidth,
                                       m_config.frameHeight,
                                       p_array_size,
                                       p_misc_flag,
                                       p_mips_level);
    }

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

    void AddPathTracerPass();
    void AddPathTracerTonePass();

#if 0
    void AddGenerateSkylightPass();
#endif
};

}  // namespace my
