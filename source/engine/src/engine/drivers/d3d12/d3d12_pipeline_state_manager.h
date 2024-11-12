#pragma once
#include <d3d12.h>
#include <wrl/client.h>

#include "rendering/pipeline_state_manager.h"

namespace my {

struct D3d12PipelineState : public PipelineState {
    using PipelineState::PipelineState;

    Microsoft::WRL::ComPtr<ID3D12PipelineState> pso;
};

class D3d12PipelineStateManager : public PipelineStateManager {
protected:
    std::shared_ptr<PipelineState> CreateInternal(const PipelineStateDesc& p_desc) final;
};

}  // namespace my
