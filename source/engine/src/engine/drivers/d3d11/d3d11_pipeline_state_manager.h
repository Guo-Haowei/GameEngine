#pragma once
#include <d3d11.h>
#include <wrl/client.h>

#include "rendering/pipeline_state_manager.h"

namespace my {

struct D3d11PipelineState : public PipelineState {
    using PipelineState::PipelineState;

    Microsoft::WRL::ComPtr<ID3D11VertexShader> vertexShader;
    Microsoft::WRL::ComPtr<ID3D11GeometryShader> geometryShader;
    Microsoft::WRL::ComPtr<ID3D11PixelShader> pixelShader;
    Microsoft::WRL::ComPtr<ID3D11ComputeShader> computeShader;
    Microsoft::WRL::ComPtr<ID3D11InputLayout> inputLayout;

    Microsoft::WRL::ComPtr<ID3D11RasterizerState> rasterizerState;
    Microsoft::WRL::ComPtr<ID3D11DepthStencilState> depthStencilState;
};

class D3d11PipelineStateManager : public PipelineStateManager {
protected:
    std::shared_ptr<PipelineState> CreateInternal(const PipelineCreateInfo& p_info) final;

    std::unordered_map<const RasterizerDesc*, Microsoft::WRL::ComPtr<ID3D11RasterizerState>> m_rasterizerStates;
    std::unordered_map<const DepthStencilDesc*, Microsoft::WRL::ComPtr<ID3D11DepthStencilState>> m_depthStencilStates;

private:
    std::shared_ptr<PipelineState> CreateGraphicsPipeline(Microsoft::WRL::ComPtr<ID3D11Device>& p_device, const PipelineCreateInfo& p_info, const std::vector<D3D_SHADER_MACRO>& p_defines);
    std::shared_ptr<PipelineState> CreateComputePipeline(Microsoft::WRL::ComPtr<ID3D11Device>& p_device, const PipelineCreateInfo& p_info, const std::vector<D3D_SHADER_MACRO>& p_defines);
};

}  // namespace my
