#pragma once
#include "core/framework/pipeline_state_manager.h"

namespace my {

struct OpenGlPipelineState : public PipelineState {
    using PipelineState::PipelineState;

    uint32_t programId;

    ~OpenGlPipelineState();
};

class OpenGlPipelineStateManager : public PipelineStateManager {
protected:
    std::shared_ptr<PipelineState> CreateGraphicsPipeline(const PipelineStateDesc& p_desc) final;
    std::shared_ptr<PipelineState> CreateComputePipeline(const PipelineStateDesc& p_desc) final;

private:
    std::shared_ptr<PipelineState> CreatePipelineImpl(const PipelineStateDesc& p_desc);
};

}  // namespace my