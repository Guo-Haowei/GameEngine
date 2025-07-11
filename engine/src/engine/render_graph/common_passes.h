#pragma once
#include "render_graph_builder.h"

namespace my {

class RenderGraphBuilderExt : public RenderGraphBuilder {
public:
    // @TODO: create 2D
    [[nodiscard]] static auto Create3D(RenderGraphBuilderConfig& p_config) -> Result<std::shared_ptr<RenderGraph>>;
    [[nodiscard]] static auto CreatePathTracer(RenderGraphBuilderConfig& p_config) -> Result<std::shared_ptr<RenderGraph>>;

private:
    void AddSprite();

    void AddEarlyZPass();
    void AddGbufferPass();
    void AddHighlightPass();
    void AddShadowPass();
    void AddVoxelizationPass();
    void AddSsaoPass();
    void AddLightingPass();
    void AddForwardPass();
    void AddBloomPass();
    void AddPostProcessPass();

    void AddPathTracerPass();
    void AddPathTracerTonePass();

    void AddGenerateSkylightPass();
};

}  // namespace my
