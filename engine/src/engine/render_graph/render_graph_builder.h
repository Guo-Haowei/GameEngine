#pragma once
#include "engine/renderer/gpu_resource.h"
#include "engine/renderer/pixel_format.h"
#include "render_pass_builder.h"

// clang-format off
namespace my { struct Framebuffer; }
namespace my { class IGraphicsManager; }
// clang-format on

namespace my {

class RenderGraph;
struct FrameData;

struct RenderGraphBuilderConfig {
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
    RenderGraphBuilder(const RenderGraphBuilderConfig& p_config);

    RenderPassBuilder& AddPass(std::string_view p_name);
    void AddDependency(std::string_view p_from, std::string_view p_to);

    [[nodiscard]] auto Compile() -> Result<std::shared_ptr<RenderGraph>>;

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

protected:
    RenderGraphBuilderConfig m_config;
    IGraphicsManager& m_graphicsManager;

    std::vector<RenderPassBuilder> m_passes;
    std::vector<std::pair<std::string, std::string>> m_dependencies;
};

}  // namespace my
