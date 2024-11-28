#pragma once
#include "engine/rendering/graphics_defines.h"
#include "engine/rendering/pipeline_state.h"

namespace my {

class PipelineStateManager {
public:
    virtual ~PipelineStateManager() = default;

    auto Initialize() -> Result<void>;
    void Finalize();

    PipelineState* Find(PipelineStateName p_name);

    static const BlendDesc& GetBlendDescDefault();
    static const BlendDesc& GetBlendDescDisable();

protected:
    virtual auto CreateGraphicsPipeline(const PipelineStateDesc& p_desc) -> Result<std::shared_ptr<PipelineState>> = 0;
    virtual auto CreateComputePipeline(const PipelineStateDesc& p_desc) -> Result<std::shared_ptr<PipelineState>> = 0;

private:
    auto Create(PipelineStateName p_name, const PipelineStateDesc& p_desc) -> Result<void>;

    std::array<std::shared_ptr<PipelineState>, PSO_NAME_MAX> m_cache;
};

}  // namespace my