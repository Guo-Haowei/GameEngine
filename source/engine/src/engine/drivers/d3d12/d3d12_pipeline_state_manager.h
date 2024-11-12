#pragma once
#include "rendering/pipeline_state_manager.h"

namespace my {

class D3d12PipelineStateManager : public PipelineStateManager {
protected:
    std::shared_ptr<PipelineState> CreateInternal(const PipelineCreateInfo& p_info) final;
};

}  // namespace my
