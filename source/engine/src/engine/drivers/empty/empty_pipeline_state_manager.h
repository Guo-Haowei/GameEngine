#pragma once
#include "core/framework/pipeline_state_manager.h"

namespace my {

class EmptyPipelineStateManager : public PipelineStateManager {
public:
protected:
    std::shared_ptr<PipelineState> CreateGraphicsPipeline(const PipelineStateDesc& p_desc) override {
        unused(p_desc);
        return nullptr;
    }

    std::shared_ptr<PipelineState> CreateComputePipeline(const PipelineStateDesc& p_desc) override {
        unused(p_desc);
        return nullptr;
    }
};

}  // namespace my
