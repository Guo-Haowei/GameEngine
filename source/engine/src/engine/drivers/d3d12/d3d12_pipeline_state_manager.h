#pragma once
#include <d3d12.h>
#include <wrl/client.h>

#include "core/framework/pipeline_state_manager.h"

namespace my {

struct D3d12PipelineState : public PipelineState {
    using PipelineState::PipelineState;

    Microsoft::WRL::ComPtr<ID3D12PipelineState> pso;
};

class D3d12PipelineStateManager : public PipelineStateManager {
public:
    D3d12PipelineStateManager();

protected:
    std::shared_ptr<PipelineState> CreateGraphicsPipeline(const PipelineStateDesc& p_desc) final;
    std::shared_ptr<PipelineState> CreateComputePipeline(const PipelineStateDesc& p_desc) final;

private:
    std::vector<D3D_SHADER_MACRO> m_defines;
};

}  // namespace my
