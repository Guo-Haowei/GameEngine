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
    auto CreateGraphicsPipeline(const PipelineStateDesc& p_desc) -> std::expected<std::shared_ptr<PipelineState>, Error<ErrorCode>> final;
    auto CreateComputePipeline(const PipelineStateDesc& p_desc) -> std::expected<std::shared_ptr<PipelineState>, Error<ErrorCode>> final;

private:
    std::vector<D3D_SHADER_MACRO> m_defines;
};

}  // namespace my
