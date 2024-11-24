#pragma once
#include "core/framework/pipeline_state_manager.h"

namespace my {

class EmptyPipelineStateManager : public PipelineStateManager {
public:
protected:
    auto CreateGraphicsPipeline(const PipelineStateDesc& p_desc) -> Result<std::shared_ptr<PipelineState>> override {
        unused(p_desc);
        return HBN_ERROR(ErrorCode::FAILURE);
    }

    auto CreateComputePipeline(const PipelineStateDesc& p_desc) -> Result<std::shared_ptr<PipelineState>> override {
        unused(p_desc);
        return HBN_ERROR(ErrorCode::FAILURE);
    }
};

}  // namespace my
