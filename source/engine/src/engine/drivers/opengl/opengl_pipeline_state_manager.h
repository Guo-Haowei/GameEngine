#pragma once
#include "rendering/pipeline_state_manager.h"

namespace my {

struct OpenGlPipelineState : public PipelineState {
    using PipelineState::PipelineState;

    uint32_t programId;

    ~OpenGlPipelineState();
};

class OpenGlPipelineStateManager : public PipelineStateManager {
protected:
    std::shared_ptr<PipelineState> CreateInternal(const PipelineStateDesc& p_desc) final;
};

}  // namespace my