#pragma once
#include "engine/core/framework/pipeline_state_manager.h"

namespace my {

struct OpenGlPipelineState : public PipelineState {
    using PipelineState::PipelineState;

    uint32_t programId;

    ~OpenGlPipelineState();
};

class OpenGlPipelineStateManager : public PipelineStateManager {
protected:
    auto CreateGraphicsPipeline(const PipelineStateDesc& p_desc) -> Result<std::shared_ptr<PipelineState>> final;
    auto CreateComputePipeline(const PipelineStateDesc& p_desc) -> Result<std::shared_ptr<PipelineState>> final;

private:
    auto CreatePipelineImpl(const PipelineStateDesc& p_desc) -> Result<std::shared_ptr<PipelineState>>;
};

}  // namespace my