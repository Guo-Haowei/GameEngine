#pragma once
#include "rendering/pipeline_state.h"

namespace my {

class PipelineStateManager {
public:
    virtual ~PipelineStateManager() = default;

    bool Initialize();
    void Finalize();

    PipelineState* Find(PipelineStateName p_name);

protected:
    virtual std::shared_ptr<PipelineState> CreateInternal(const PipelineCreateInfo& p_info) = 0;

private:
    bool Create(PipelineStateName p_name, const PipelineCreateInfo& p_info);

    std::array<std::shared_ptr<PipelineState>, PIPELINE_STATE_MAX> m_cache;
};

}  // namespace my