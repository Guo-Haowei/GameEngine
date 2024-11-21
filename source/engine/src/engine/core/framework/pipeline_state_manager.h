#pragma once
#include "rendering/graphics_defines.h"
#include "rendering/pipeline_state.h"

namespace my {

class PipelineStateManager {
public:
    virtual ~PipelineStateManager() = default;

    bool Initialize();
    void Finalize();

    PipelineState* Find(PipelineStateName p_name);

    static const BlendDesc& GetBlendDescDefault();
    static const BlendDesc& GetBlendDescDisable();

protected:
    virtual std::shared_ptr<PipelineState> CreateGraphicsPipeline(const PipelineStateDesc& p_desc) = 0;
    virtual std::shared_ptr<PipelineState> CreateComputePipeline(const PipelineStateDesc& p_desc) = 0;

private:
    bool Create(PipelineStateName p_name, const PipelineStateDesc& p_desc);

    std::array<std::shared_ptr<PipelineState>, PSO_MAX> m_cache;
};

}  // namespace my