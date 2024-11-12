#include "d3d12_pipeline_state_manager.h"

namespace my {

std::shared_ptr<PipelineState> D3d12PipelineStateManager::CreateInternal(const PipelineCreateInfo& p_info) {
    unused(p_info);
    return nullptr;
}

}  // namespace my
