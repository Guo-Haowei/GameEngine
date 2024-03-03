#pragma once
#include <d3d11.h>
#include <wrl/client.h>

#include "rendering/pipeline_state_manager.h"

namespace my {

struct D3d11PipelineState : public PipelineState {
    using PipelineState::PipelineState;

    Microsoft::WRL::ComPtr<ID3D11VertexShader> vertex_shader;
    Microsoft::WRL::ComPtr<ID3D11GeometryShader> geometry_shader;
    Microsoft::WRL::ComPtr<ID3D11PixelShader> pixel_shader;
    Microsoft::WRL::ComPtr<ID3D11ComputeShader> compute_shader;
    Microsoft::WRL::ComPtr<ID3D11InputLayout> input_layout;

    Microsoft::WRL::ComPtr<ID3D11RasterizerState> rasterizer;
};

class D3d11PipelineStateManager : public PipelineStateManager {
protected:
    std::shared_ptr<PipelineState> create(const PipelineCreateInfo& p_info) final;

    std::unordered_map<void*, Microsoft::WRL::ComPtr<ID3D11RasterizerState>> m_rasterizer_states;
};

}  // namespace my
