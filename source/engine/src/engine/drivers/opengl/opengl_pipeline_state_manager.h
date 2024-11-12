#pragma once
#include "rendering/pipeline_state_manager.h"

namespace my {

struct OpenGLPipelineState : public PipelineState {
    using PipelineState::PipelineState;

    uint32_t programId;

    ~OpenGLPipelineState();
};

class OpenGLPipelineStateManager : public PipelineStateManager {
protected:
    std::shared_ptr<PipelineState> CreateInternal(const PipelineStateDesc& p_desc) final;
};

}  // namespace my