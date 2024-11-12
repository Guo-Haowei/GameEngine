#include "d3d11_pipeline_state_manager.h"

#include "drivers/d3d11/d3d11_graphics_manager.h"
#include "drivers/d3d11/d3d11_helpers.h"
#include "drivers/d3d_common/d3d_common.h"
#define INCLUDE_AS_D3D11
#include "drivers/d3d_common/d3d_convert.h"

namespace my {

using Microsoft::WRL::ComPtr;

std::shared_ptr<PipelineState> D3d11PipelineStateManager::CreateInternal(const PipelineStateDesc& p_desc) {
    auto graphics_manager = reinterpret_cast<D3d11GraphicsManager*>(GraphicsManager::GetSingletonPtr());
    // @TODO: try not to expose device
    auto& device = graphics_manager->GetD3dDevice();
    DEV_ASSERT(device);

    if (!device) {
        return nullptr;
    }

    std::vector<D3D_SHADER_MACRO> defines;
    for (const auto& define : p_desc.defines) {
        defines.push_back({ define.name, define.value });
    }
    defines.push_back({ "HLSL_LANG", "1" });
    defines.push_back({ nullptr, nullptr });

    if (!p_desc.cs.empty()) {
        return CreateComputePipeline(device, p_desc, defines);
    }

    return CreateGraphicsPipeline(device, p_desc, defines);
}

std::shared_ptr<PipelineState> D3d11PipelineStateManager::CreateGraphicsPipeline(Microsoft::WRL::ComPtr<ID3D11Device>& p_device, const PipelineStateDesc& p_desc, const std::vector<D3D_SHADER_MACRO>& p_defines) {
    auto pipeline_state = std::make_shared<D3d11PipelineState>(p_desc);

    HRESULT hr = S_OK;

    ComPtr<ID3DBlob> vsblob;
    if (!p_desc.vs.empty()) {
        auto res = CompileShader(p_desc.vs, "vs_5_0", p_defines.data());
        if (!res) {
            LOG_FATAL("Failed to compile '{}'\n  detail: {}", p_desc.vs, res.error());
            return nullptr;
        }

        ComPtr<ID3DBlob> blob;
        blob = *res;
        hr = p_device->CreateVertexShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, pipeline_state->vertexShader.GetAddressOf());
        D3D_FAIL_V_MSG(hr, nullptr, "failed to create vertex buffer");
        vsblob = blob;
    }
    if (!p_desc.ps.empty()) {
        auto res = CompileShader(p_desc.ps, "ps_5_0", p_defines.data());
        if (!res) {
            LOG_FATAL("Failed to compile '{}'\n  detail: {}", p_desc.vs, res.error());
            return nullptr;
        }

        ComPtr<ID3DBlob> blob;
        blob = *res;
        hr = p_device->CreatePixelShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, pipeline_state->pixelShader.GetAddressOf());
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

    hr = p_device->CreateInputLayout(elements.data(), (UINT)elements.size(), vsblob->GetBufferPointer(), vsblob->GetBufferSize(), pipeline_state->inputLayout.GetAddressOf());
    D3D_FAIL_V_MSG(hr, nullptr, "failed to create input layout");

    if (p_desc.rasterizerDesc) {
        ComPtr<ID3D11RasterizerState> state;

        auto it = m_rasterizerStates.find(p_desc.rasterizerDesc);
        if (it == m_rasterizerStates.end()) {
            D3D11_RASTERIZER_DESC desc{};
            desc.FillMode = d3d::Convert(p_desc.rasterizerDesc->fillMode);
            desc.CullMode = d3d::Convert(p_desc.rasterizerDesc->cullMode);
            desc.FrontCounterClockwise = p_desc.rasterizerDesc->frontCounterClockwise;
            hr = p_device->CreateRasterizerState(&desc, state.GetAddressOf());
            D3D_FAIL_V_MSG(hr, nullptr, "failed to create rasterizer state");
            m_rasterizerStates[p_desc.rasterizerDesc] = state;
        } else {
            state = it->second;
        }
        DEV_ASSERT(state);
        pipeline_state->rasterizerState = state;
    }
    if (p_desc.depthStencilDesc) {
        ComPtr<ID3D11DepthStencilState> state;

        auto it = m_depthStencilStates.find(p_desc.depthStencilDesc);
        if (it == m_depthStencilStates.end()) {
            D3D11_DEPTH_STENCIL_DESC desc{};
            desc.DepthEnable = p_desc.depthStencilDesc->depthEnabled;
            desc.DepthFunc = d3d::Convert(p_desc.depthStencilDesc->depthFunc);
            desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
            desc.StencilEnable = false;
            // desc.StencilEnable = p_desc.depthStencilDesc->stencilEnabled;
            p_device->CreateDepthStencilState(&desc, state.GetAddressOf());
            D3D_FAIL_V_MSG(hr, nullptr, "failed to create depth stencil state");
            m_depthStencilStates[p_desc.depthStencilDesc] = state;
        } else {
            state = it->second.Get();
        }
        DEV_ASSERT(state);
        pipeline_state->depthStencilState = state;
    }

    return pipeline_state;
}

std::shared_ptr<PipelineState> D3d11PipelineStateManager::CreateComputePipeline(Microsoft::WRL::ComPtr<ID3D11Device>& p_device, const PipelineStateDesc& p_desc, const std::vector<D3D_SHADER_MACRO>& p_defines) {
    auto pipeline_state = std::make_shared<D3d11PipelineState>(p_desc);

    auto res = CompileShader(p_desc.cs, "cs_5_0", p_defines.data());
    if (!res) {
        LOG_ERROR("Failed to compile '{}'\n  detail: {}", p_desc.cs, res.error());
        return nullptr;
    }

    ComPtr<ID3DBlob> blob;
    blob = *res;
    HRESULT hr = p_device->CreateComputeShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, pipeline_state->computeShader.GetAddressOf());
    D3D_FAIL_V_MSG(hr, nullptr, "failed to create vertex buffer");

    return pipeline_state;
}

}  // namespace my

#undef INCLUDE_AS_D3D11
