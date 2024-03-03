#pragma once
#include "rendering/pipeline_state.h"

namespace my {

class PipelineStateManager {
public:
    virtual ~PipelineStateManager() = default;

    bool initialize();
    void finalize();

    PipelineState* find(PipelineStateName p_name);

protected:
    virtual std::shared_ptr<PipelineState> create(const PipelineCreateInfo& info) = 0;

    std::array<std::shared_ptr<PipelineState>, PIPELINE_STATE_MAX> m_cache;
};

}  // namespace my