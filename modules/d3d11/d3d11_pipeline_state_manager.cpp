#include "d3d11_pipeline_state_manager.h"

#include "../d3d_common/d3d_common.h"
#include "d3d11_graphics_manager.h"
#include "d3d11_helpers.h"
#define INCLUDE_AS_D3D11
#include "../d3d_common/d3d_convert.h"

namespace my {

using Microsoft::WRL::ComPtr;

D3d11PipelineStateManager::D3d11PipelineStateManager() {
    m_defines.push_back({ "HLSL_LANG", "1" });
    m_defines.push_back({ "HLSL_LANG_D3D11", "1" });
    m_defines.push_back({ nullptr, nullptr });
}

auto D3d11PipelineStateManager::CreateGraphicsPipeline(const PipelineStateDesc& p_desc) -> Result<std::shared_ptr<PipelineState>> {
    auto graphics_manager = reinterpret_cast<D3d11GraphicsManager*>(IGraphicsManager::GetSingletonPtr());
    auto& device = graphics_manager->GetD3dDevice();
    DEV_ASSERT(device);
    if (!device) {
        return HBN_ERROR(ErrorCode::ERR_INVALID_DATA);
    }

    auto pipeline_state = std::make_shared<D3d11PipelineState>(p_desc);

    HRESULT hr = S_OK;
    ComPtr<ID3DBlob> vsblob;
    if (!p_desc.vs.empty()) {
        auto res = CompileShader(p_desc.vs, "vs_5_0", m_defines.data());
        if (!res) {
            return HBN_ERROR(res.error());
        }

        ComPtr<ID3DBlob> blob;
        blob = *res;
        hr = device->CreateVertexShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, pipeline_state->vertexShader.GetAddressOf());
        D3D_FAIL_V_MSG(hr, nullptr, "failed to create vertex buffer");
        vsblob = blob;
    }
    if (!p_desc.ps.empty()) {
        auto res = CompileShader(p_desc.ps, "ps_5_0", m_defines.data());
        if (!res) {
            return HBN_ERROR(res.error());
        }

        ComPtr<ID3DBlob> blob;
        blob = *res;
        hr = device->CreatePixelShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, pipeline_state->pixelShader.GetAddressOf());
        D3D_FAIL_V_MSG(hr, nullptr, "failed to create vertex buffer");
    }

    std::vector<D3D11_INPUT_ELEMENT_DESC> elements;
    elements.reserve(p_desc.inputLayoutDesc->elements.size());
    for (const auto& ele : p_desc.inputLayoutDesc->elements) {
        D3D11_INPUT_ELEMENT_DESC desc;
        desc.SemanticName = ele.semanticName.c_str();
        desc.SemanticIndex = ele.semanticIndex;
        desc.Format = d3d::Convert(ele.format);
        desc.InputSlot = ele.inputSlot;
        desc.AlignedByteOffset = ele.alignedByteOffset;
        desc.InputSlotClass = d3d::Convert(ele.inputSlotClass);
        desc.InstanceDataStepRate = ele.instanceDataStepRate;
        elements.emplace_back(desc);
    }
    DEV_ASSERT(elements.size());

    hr = device->CreateInputLayout(elements.data(), (UINT)elements.size(), vsblob->GetBufferPointer(), vsblob->GetBufferSize(), pipeline_state->inputLayout.GetAddressOf());
    D3D_FAIL_V_MSG(hr, nullptr, "failed to create input layout");

    if (DEV_VERIFY(p_desc.rasterizerDesc)) {
        ComPtr<ID3D11RasterizerState> state;

        auto it = m_rasterizerStates.find(p_desc.rasterizerDesc);
        if (it == m_rasterizerStates.end()) {
            D3D11_RASTERIZER_DESC desc{};
            desc.FillMode = d3d::Convert(p_desc.rasterizerDesc->fillMode);
            desc.CullMode = d3d::Convert(p_desc.rasterizerDesc->cullMode);
            desc.FrontCounterClockwise = p_desc.rasterizerDesc->frontCounterClockwise;
            hr = device->CreateRasterizerState(&desc, state.GetAddressOf());
            D3D_FAIL_V_MSG(hr, nullptr, "failed to create rasterizer state");
            m_rasterizerStates[p_desc.rasterizerDesc] = state;
        } else {
            state = it->second;
        }
        DEV_ASSERT(state);
        pipeline_state->rasterizerState = state;
    }
    if (DEV_VERIFY(p_desc.depthStencilDesc)) {
        ComPtr<ID3D11DepthStencilState> state;

        auto it = m_depthStencilStates.find(p_desc.depthStencilDesc);
        if (it == m_depthStencilStates.end()) {
            D3D11_DEPTH_STENCIL_DESC desc = d3d::Convert(p_desc.depthStencilDesc);
            D3D_FAIL_V(device->CreateDepthStencilState(&desc, state.GetAddressOf()), nullptr);
            m_depthStencilStates[p_desc.depthStencilDesc] = state;
        } else {
            state = it->second.Get();
        }
        DEV_ASSERT(state);
        pipeline_state->depthStencilState = state;
    }
    if (DEV_VERIFY(p_desc.blendDesc)) {
        ComPtr<ID3D11BlendState> state;

        auto it = m_blendStates.find(p_desc.blendDesc);
        if (it == m_blendStates.end()) {
            D3D11_BLEND_DESC desc = d3d::Convert(p_desc.blendDesc);
            D3D_FAIL_V(device->CreateBlendState(&desc, state.GetAddressOf()), nullptr);
            m_blendStates[p_desc.blendDesc] = state;
        } else {
            state = it->second.Get();
        }
        DEV_ASSERT(state);
        pipeline_state->blendState = state;
    }

    return pipeline_state;
}

auto D3d11PipelineStateManager::CreateComputePipeline(const PipelineStateDesc& p_desc) -> Result<std::shared_ptr<PipelineState>> {
    auto graphics_manager = reinterpret_cast<D3d11GraphicsManager*>(BaseGraphicsManager::GetSingletonPtr());
    auto& device = graphics_manager->GetD3dDevice();
    DEV_ASSERT(device);

    if (!device) {
        return HBN_ERROR(ErrorCode::ERR_INVALID_DATA);
    }

    auto pipeline_state = std::make_shared<D3d11PipelineState>(p_desc);

    auto res = CompileShader(p_desc.cs, "cs_5_0", m_defines.data());
    if (!res) {
        return HBN_ERROR(res.error());
    }

    ComPtr<ID3DBlob> blob;
    blob = *res;
    HRESULT hr = device->CreateComputeShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, pipeline_state->computeShader.GetAddressOf());

    D3D_FAIL_V_MSG(hr, HBN_ERROR(res.error()), "failed to create vertex buffer");

    return pipeline_state;
}

}  // namespace my

#undef INCLUDE_AS_D3D11
