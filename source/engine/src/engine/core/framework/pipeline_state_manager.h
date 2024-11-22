#pragma once
#include "rendering/graphics_defines.h"
#include "rendering/pipeline_state.h"

namespace my {

class PipelineStateManager {
public:
    virtual ~PipelineStateManager() = default;

    auto Initialize() -> std::expected<void, ErrorRef>;
    void Finalize();

    PipelineState* Find(PipelineStateName p_name);

    static const BlendDesc& GetBlendDescDefault();
    static const BlendDesc& GetBlendDescDisable();

protected:
    virtual auto CreateGraphicsPipeline(const PipelineStateDesc& p_desc) -> std::expected<std::shared_ptr<PipelineState>, ErrorRef> = 0;
    virtual auto CreateComputePipeline(const PipelineStateDesc& p_desc) -> std::expected<std::shared_ptr<PipelineState>, ErrorRef> = 0;

private:
    auto Create(PipelineStateName p_name, const PipelineStateDesc& p_desc) -> std::expected<void, ErrorRef>;

    std::array<std::shared_ptr<PipelineState>, PSO_NAME_MAX> m_cache;
};

}  // namespace my