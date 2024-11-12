#include "d3d12_pipeline_state_manager.h"

#include "drivers/d3d12/d3d12_graphics_manager.h"
#include "drivers/d3d_common/d3d_common.h"
#define INCLUDE_AS_D3D12
#include "drivers/d3d_common/d3d_convert.h"

namespace my {

using Microsoft::WRL::ComPtr;

std::shared_ptr<PipelineState> D3d12PipelineStateManager::CreateInternal(const PipelineCreateInfo& p_info) {
    auto pipeline_state = std::make_shared<D3d12PipelineState>(p_info.inputLayoutDesc,
                                                               p_info.rasterizerDesc,
                                                               p_info.depthStencilDesc);
    // ComPtr<ID3DBlob> vsBlob;
    //= CompileShaderFromFile(
    //     desc.vertexShader.c_str(), SHADER_TYPE_VERTEX, true);
    // if (!vsBlob)
    //{
    //     return false;
    // }
    // ComPtr<ID3DBlob> psBlob;
    //= CompileShaderFromFile(
    //     desc.pixelShader.c_str(), SHADER_TYPE_PIXEL, true);
    // if (!psBlob)
    //{
    //     return false;
    // }

    std::vector<D3D12_INPUT_ELEMENT_DESC> elements;
    elements.reserve(p_info.inputLayoutDesc->elements.size());
    for (const auto& ele : p_info.inputLayoutDesc->elements) {
        D3D12_INPUT_ELEMENT_DESC ildesc;
        ildesc.SemanticName = ele.semanticName.c_str();
        ildesc.SemanticIndex = ele.semanticIndex;
        ildesc.Format = d3d::Convert(ele.format);
        ildesc.InputSlot = ele.inputSlot;
        ildesc.AlignedByteOffset = ele.alignedByteOffset;
        ildesc.InputSlotClass = d3d::Convert(ele.inputSlotClass);
        ildesc.InstanceDataStepRate = ele.instanceDataStepRate;
        elements.push_back(ildesc);
    }
    DEV_ASSERT(elements.size());

    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};

    D3D12_PRIMITIVE_TOPOLOGY_TYPE tp = D3D12_PRIMITIVE_TOPOLOGY_TYPE_UNDEFINED;
    switch (p_info.primitiveTopologyType) {
        case PrimitiveTopologyType::LINE:
            tp = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
            break;
        case PrimitiveTopologyType::TRIANGLE:
            tp = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
            break;
        default:
            PANIC("");
            break;
    }
#if 0

    D3D12_RASTERIZER_DESC rsDesc{};
    rsDesc.FillMode = Convert(desc.rasterizer.fillMode);
    rsDesc.CullMode = Convert(desc.rasterizer.cullMode);
    rsDesc.FrontCounterClockwise = desc.rasterizer.frontCounterClockwise;
    rsDesc.DepthBias = desc.rasterizer.depthBias;
    rsDesc.SlopeScaledDepthBias = desc.rasterizer.slopeScaledDepthBias;
    rsDesc.DepthClipEnable = desc.rasterizer.depthClipEnable;
    // rsDesc.ScissorEnable = desc.rasterizer.scissorEnable;
    rsDesc.MultisampleEnable = desc.rasterizer.multisampleEnable;
    rsDesc.AntialiasedLineEnable = desc.rasterizer.antialiasedLineEnable;

    D3D12_DEPTH_STENCIL_DESC dssDesc{};
    dssDesc.DepthEnable = desc.depthStencil.depthEnable;
    dssDesc.DepthWriteMask = Convert(desc.depthStencil.depthWriteMask);
    dssDesc.DepthFunc = Convert(desc.depthStencil.depthFunc);
    dssDesc.StencilEnable = desc.depthStencil.stencilEnable;
    dssDesc.StencilReadMask = desc.depthStencil.stencilReadMask;
    dssDesc.StencilWriteMask = desc.depthStencil.stencilWriteMask;
    // @TODO:
    // dssDesc.FrontFace;
    // dssDesc.BackFace;

    psoDesc.InputLayout = { elements.data(), (uint32_t)elements.size() };
    psoDesc.pRootSignature = m_rootSignature.Get();
    psoDesc.VS = CD3DX12_SHADER_BYTECODE(vsBlob.Get());
    psoDesc.PS = CD3DX12_SHADER_BYTECODE(psBlob.Get());
    psoDesc.RasterizerState = rsDesc;
    psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    psoDesc.DepthStencilState = dssDesc;
    psoDesc.SampleMask = UINT_MAX;
    psoDesc.PrimitiveTopologyType = tp;
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    psoDesc.DSVFormat = DEFAULT_DEPTH_STENCIL_FORMAT;
    psoDesc.SampleDesc.Count = 1;

    out_pso.internalState = internalState;
    out_pso.desc = desc;
#endif

    ID3D12Device4* device = reinterpret_cast<D3d12GraphicsManager*>(GraphicsManager::GetSingletonPtr())->GetDevice();
    D3D_FAIL_V(device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pipeline_state->pso)), nullptr);

    return pipeline_state;
}

}  // namespace my

#undef INCLUDE_AS_D3D12
