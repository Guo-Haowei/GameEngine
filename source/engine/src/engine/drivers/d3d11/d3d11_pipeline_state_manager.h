#pragma once
#include <d3d11.h>
#include <wrl/client.h>

#include "core/framework/pipeline_state_manager.h"

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
    Microsoft::WRL::ComPtr<ID3D11BlendState> blendState;
};

class D3d11PipelineStateManager : public PipelineStateManager {
public:
    D3d11PipelineStateManager();

protected:
    auto CreateGraphicsPipeline(const PipelineStateDesc& p_desc) -> std::expected<std::shared_ptr<PipelineState>, ErrorRef> final;
    auto CreateComputePipeline(const PipelineStateDesc& p_desc) -> std::expected<std::shared_ptr<PipelineState>, ErrorRef> final;

    std::unordered_map<const RasterizerDesc*, Microsoft::WRL::ComPtr<ID3D11RasterizerState>> m_rasterizerStates;
    std::unordered_map<const DepthStencilDesc*, Microsoft::WRL::ComPtr<ID3D11DepthStencilState>> m_depthStencilStates;
    std::unordered_map<const BlendDesc*, Microsoft::WRL::ComPtr<ID3D11BlendState>> m_blendStates;

private:
    std::vector<D3D_SHADER_MACRO> m_defines;
};

}  // namespace my
