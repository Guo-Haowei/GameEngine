#include "d3d12_pipeline_state_manager.h"

#include "drivers/d3d12/d3d12_graphics_manager.h"
#include "drivers/d3d_common/d3d_common.h"
#define INCLUDE_AS_D3D12
#include "drivers/d3d_common/d3d_convert.h"

namespace my {

using Microsoft::WRL::ComPtr;

// @TODO: refactor
constexpr DXGI_FORMAT DEFAULT_DEPTH_STENCIL_FORMAT = DXGI_FORMAT_D32_FLOAT;

std::shared_ptr<PipelineState> D3d12PipelineStateManager::CreateInternal(const PipelineStateDesc& p_desc) {
    auto graphics_manager = reinterpret_cast<D3d12GraphicsManager*>(GraphicsManager::GetSingletonPtr());

    auto pipeline_state = std::make_shared<D3d12PipelineState>(p_desc);
     ComPtr<ID3DBlob> vsBlob;
    //= CompileShaderFromFile(
    //     desc.vertexShader.c_str(), SHADER_TYPE_VERTEX, true);
    // if (!vsBlob)
    //{
    //     return false;
    // }
     ComPtr<ID3DBlob> psBlob;
    //= CompileShaderFromFile(
    //     desc.pixelShader.c_str(), SHADER_TYPE_PIXEL, true);
    // if (!psBlob)
    //{
    //     return false;
    // }

    std::vector<D3D12_INPUT_ELEMENT_DESC> elements;
    elements.reserve(p_desc.inputLayoutDesc->elements.size());
    for (const auto& ele : p_desc.inputLayoutDesc->elements) {
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

    D3D12_PRIMITIVE_TOPOLOGY_TYPE topology = D3D12_PRIMITIVE_TOPOLOGY_TYPE_UNDEFINED;
    switch (p_desc.primitiveTopology) {
        case PrimitiveTopology::LINE:
            topology = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
            break;
        case PrimitiveTopology::TRIANGLE:
            topology = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
            break;
        default:
            CRASH_NOW();
            break;
    }

    D3D12_RASTERIZER_DESC rasterizer_desc{};
    rasterizer_desc.FillMode = d3d::Convert(p_desc.rasterizerDesc->fillMode);
    rasterizer_desc.CullMode = d3d::Convert(p_desc.rasterizerDesc->cullMode);
    rasterizer_desc.FrontCounterClockwise = p_desc.rasterizerDesc->frontCounterClockwise;
    rasterizer_desc.DepthBias = p_desc.rasterizerDesc->depthBias;
    rasterizer_desc.SlopeScaledDepthBias = p_desc.rasterizerDesc->slopeScaledDepthBias;
    rasterizer_desc.DepthClipEnable = p_desc.rasterizerDesc->depthClipEnable;
    //rasterizer_desc.ScissorEnable = p_desc.rasterizerDesc->scissorEnable;
    rasterizer_desc.MultisampleEnable = p_desc.rasterizerDesc->multisampleEnable;
    rasterizer_desc.AntialiasedLineEnable = p_desc.rasterizerDesc->antialiasedLineEnable;

    D3D12_DEPTH_STENCIL_DESC depth_stencil_desc{};
    depth_stencil_desc.DepthEnable = p_desc.depthStencilDesc->depthEnabled;
    depth_stencil_desc.DepthFunc = d3d::Convert(p_desc.depthStencilDesc->depthFunc);
    depth_stencil_desc.StencilEnable = p_desc.depthStencilDesc->stencilEnabled;
    //depth_stencil_desc.DepthWriteMask = d3d::Convert(p_desc.depthStencilDesc->depthWriteMask);
    //depth_stencil_desc.StencilReadMask = p_desc.depthStencilDesc->stencilReadMask;
    //depth_stencil_desc.StencilWriteMask = p_desc.depthStencilDesc->stencilWriteMask;
    depth_stencil_desc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
    depth_stencil_desc.StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK;
    depth_stencil_desc.StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK;
    // @TODO:
    // depth_stencil_desc.FrontFace;
    // depth_stencil_desc.BackFace;
#if 0

#endif
    psoDesc.InputLayout = { elements.data(), (uint32_t)elements.size() };
    psoDesc.pRootSignature = graphics_manager->GetRootSignature();
    psoDesc.VS = CD3DX12_SHADER_BYTECODE(vsBlob.Get());
    psoDesc.PS = CD3DX12_SHADER_BYTECODE(psBlob.Get());
    psoDesc.RasterizerState = rasterizer_desc;
    psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    psoDesc.DepthStencilState = depth_stencil_desc;
    psoDesc.SampleMask = UINT_MAX;
    psoDesc.PrimitiveTopologyType = topology;
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    psoDesc.DSVFormat = DEFAULT_DEPTH_STENCIL_FORMAT;
    psoDesc.SampleDesc.Count = 1;

    ID3D12Device4* device = reinterpret_cast<D3d12GraphicsManager*>(GraphicsManager::GetSingletonPtr())->GetDevice();
    D3D_FAIL_V(device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pipeline_state->pso)), nullptr);

    return pipeline_state;
}

}  // namespace my

#undef INCLUDE_AS_D3D12
