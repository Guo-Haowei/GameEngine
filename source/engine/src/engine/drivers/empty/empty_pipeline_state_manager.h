#pragma once
#include "core/framework/pipeline_state_manager.h"

namespace my {

class EmptyPipelineStateManager : public PipelineStateManager {
public:
protected:
    auto CreateGraphicsPipeline(const PipelineStateDesc& p_desc) -> std::expected<std::shared_ptr<PipelineState>, Error<ErrorCode>> override {
        unused(p_desc);
        return VCT_ERROR(FAILURE);
    }

    auto CreateComputePipeline(const PipelineStateDesc& p_desc) -> std::expected<std::shared_ptr<PipelineState>, Error<ErrorCode>> override {
        unused(p_desc);
        return VCT_ERROR(FAILURE);
    }
};

}  // namespace my
