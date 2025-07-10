#pragma once

namespace my {

struct RenderGraphBuilderConfig;
class RenderGraph;

auto RenderGraph2D(RenderGraphBuilderConfig& p_config) -> Result<std::shared_ptr<RenderGraph>>;

}  // namespace my
