#pragma once
#include "rendering/pipeline_state_manager.h"

namespace my {

class EmptyPipelineStateManager : public PipelineStateManager {
public:
protected:
    virtual std::shared_ptr<PipelineState> CreateInternal(const PipelineStateDesc& p_info) {
        unused(p_info);
        return nullptr;
    }
};

}  // namespace my
