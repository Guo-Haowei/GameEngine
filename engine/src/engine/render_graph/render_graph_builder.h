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

protected:
    RenderGraphBuilderConfig m_config;
    IGraphicsManager& m_graphicsManager;

    std::vector<RenderPassBuilder> m_passes;
    std::vector<std::pair<std::string, std::string>> m_dependencies;
};

}  // namespace my
