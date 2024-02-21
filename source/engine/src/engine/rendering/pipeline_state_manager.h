#pragma once
#include "core/base/singleton.h"
#include "rendering/pipeline_state.h"

namespace my {

struct PipelineCreateInfo {
    std::string_view vs;
    std::string_view ps;
    std::string_view gs;
    std::string_view cs;
};

struct PipelineState {
    virtual ~PipelineState() = default;
};

class PipelineStateManager {
public:
    bool initialize();
    void finalize();

    PipelineState* find(PipelineStateName p_name);

protected:
    virtual std::shared_ptr<PipelineState> create(const PipelineCreateInfo& info) = 0;

    std::array<std::shared_ptr<PipelineState>, PIPELINE_STATE_MAX> m_cache;
};

}  // namespace my