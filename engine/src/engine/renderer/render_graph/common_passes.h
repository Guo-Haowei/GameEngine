#pragma once
#include "render_graph_builder.h"

constexpr const char RG_PASS_EMPTY[] = "p:empty";
constexpr const char RG_PASS_EARLY_Z[] = "p:early_z";
constexpr const char RG_PASS_SHADOW[] = "p:shadow";
constexpr const char RG_PASS_GBUFFER[] = "p:gbuffer";
constexpr const char RG_PASS_VOXELIZATION[] = "p:voxelization";
constexpr const char RG_PASS_LIGHTING[] = "p:lighting";
constexpr const char RG_PASS_FORWARD[] = "p:forward";
constexpr const char RG_PASS_BLOOM_SETUP[] = "p:bloom_setup";
constexpr const char RG_PASS_POST_PROCESS[] = "p:post_process";
constexpr const char RG_PASS_OVERLAY[] = "p:overlay";
constexpr const char RG_PASS_SSAO[] = "p:ssao";
constexpr const char RG_PASS_OUTLINE[] = "p:outline";
constexpr const char RG_PASS_PATHTRACER[] = "p:pathtracer";
constexpr const char RG_PASS_PATHTRACER_PRESENT[] = "p:pathtracer_present";

constexpr const char RG_RES_DEPTH_STENCIL[] = "r:depth";
constexpr const char RG_RES_SHADOW_MAP[] = "r:shadow";
constexpr const char RG_RES_GBUFFER_COLOR0[] = "r:gbuffer0";
constexpr const char RG_RES_GBUFFER_COLOR1[] = "r:gbuffer1";
constexpr const char RG_RES_GBUFFER_COLOR2[] = "r:gbuffer2";
constexpr const char RG_RES_SSAO[] = "r:ssao";
constexpr const char RG_RES_SSAO_NOISE[] = "r:ssao_noise";
constexpr const char RG_RES_LIGHTING[] = "r:lighting";
constexpr const char RG_RES_POST_PROCESS[] = "r:post_process";
constexpr const char RG_RES_OVERLAY[] = "r:overlay";
constexpr const char RG_RES_VOXEL_LIGHTING[] = "r:voxel_lighting";
constexpr const char RG_RES_VOXEL_NORMAL[] = "r:voxel_normal";
constexpr const char RG_RES_OUTLINE[] = "r:outline";
constexpr const char RG_RES_PATHTRACER[] = "r:pathtracer";

#define RG_PASS_BLOOM_DOWN_PREFIX "p:bloom_downsample_"
#define RG_PASS_BLOOM_UP_PREFIX   "p:bloom_upsample_"
#define RG_RES_BLOOM_PREFIX       "r:bloom_"

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

        return BuildDefaultTextureDesc(p_name,
                                       p_format,
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
