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
    auto CreateGraphicsPipeline(const PipelineStateDesc& p_desc) -> std::expected<std::shared_ptr<PipelineState>, ErrorRef> final;
    auto CreateComputePipeline(const PipelineStateDesc& p_desc) -> std::expected<std::shared_ptr<PipelineState>, ErrorRef> final;

private:
    auto CreatePipelineImpl(const PipelineStateDesc& p_desc) -> std::expected<std::shared_ptr<PipelineState>, ErrorRef>;
};

}  // namespace my