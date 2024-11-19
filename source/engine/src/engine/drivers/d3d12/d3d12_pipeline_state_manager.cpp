#include "d3d12_pipeline_state_manager.h"

#include "drivers/d3d12/d3d12_graphics_manager.h"
#include "drivers/d3d_common/d3d_common.h"
#define INCLUDE_AS_D3D12
#include "drivers/d3d_common/d3d_convert.h"

namespace my {

using Microsoft::WRL::ComPtr;

std::shared_ptr<PipelineState> D3d12PipelineStateManager::CreateInternal(const PipelineStateDesc& p_desc) {
    auto graphics_manager = reinterpret_cast<D3d12GraphicsManager*>(GraphicsManager::GetSingletonPtr());

    auto pipeline_state = std::make_shared<D3d12PipelineState>(p_desc);
    ComPtr<ID3DBlob> vs_blob;
    ComPtr<ID3DBlob> ps_blob;

    // @TODO: refactor
    std::vector<D3D_SHADER_MACRO> defines;
    for (const auto& define : p_desc.defines) {
        defines.push_back({ define.name, define.value });
    }
    defines.push_back({ "HLSL_LANG", "1" });
    defines.push_back({ "HLSL_LANG_D3D12", "1" });
    defines.push_back({ nullptr, nullptr });

    if (!p_desc.vs.empty()) {
        auto res = CompileShader(p_desc.vs, "vs_5_1", defines.data());
        if (!res) {
            LOG_FATAL("Failed to compile '{}'\n  detail: {}", p_desc.vs, res.error());
            return nullptr;
        }
        vs_blob = *res;
    }
    if (!p_desc.ps.empty()) {
        auto res = CompileShader(p_desc.ps, "ps_5_1", defines.data());
        if (!res) {
            LOG_FATAL("Failed to compile '{}'\n  detail: {}", p_desc.ps, res.error());
            return nullptr;
        }
        ps_blob = *res;
    }

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

    // @TODO: make it a Convert
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
    // rasterizer_desc.ScissorEnable = p_desc.rasterizerDesc->scissorEnable;
    rasterizer_desc.MultisampleEnable = p_desc.rasterizerDesc->multisampleEnable;
    rasterizer_desc.AntialiasedLineEnable = p_desc.rasterizerDesc->antialiasedLineEnable;

    D3D12_DEPTH_STENCIL_DESC depth_stencil_desc{};
    depth_stencil_desc.DepthEnable = p_desc.depthStencilDesc->depthEnabled;
    depth_stencil_desc.DepthFunc = d3d::Convert(p_desc.depthStencilDesc->depthFunc);
    depth_stencil_desc.StencilEnable = p_desc.depthStencilDesc->stencilEnabled;
    depth_stencil_desc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
    depth_stencil_desc.StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK;
    depth_stencil_desc.StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK;
    const D3D12_DEPTH_STENCILOP_DESC default_stencil_op = {
        D3D12_STENCIL_OP_KEEP,
        D3D12_STENCIL_OP_KEEP,
        D3D12_STENCIL_OP_KEEP,
        D3D12_COMPARISON_FUNC_ALWAYS,
    };
    depth_stencil_desc.FrontFace = default_stencil_op;
    depth_stencil_desc.BackFace = default_stencil_op;

    D3D12_GRAPHICS_PIPELINE_STATE_DESC pso_desc = {};
    pso_desc.pRootSignature = graphics_manager->GetRootSignature();
    pso_desc.VS = CD3DX12_SHADER_BYTECODE(vs_blob.Get());
    pso_desc.PS = CD3DX12_SHADER_BYTECODE(ps_blob.Get());
    pso_desc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    pso_desc.SampleMask = UINT_MAX;
    pso_desc.RasterizerState = rasterizer_desc;
    pso_desc.DepthStencilState = depth_stencil_desc;
    pso_desc.InputLayout = { elements.data(), (uint32_t)elements.size() };
    pso_desc.PrimitiveTopologyType = topology;
    pso_desc.SampleDesc.Count = 1;

    pso_desc.NumRenderTargets = p_desc.numRenderTargets;
    for (uint32_t index = 0; index < p_desc.numRenderTargets; ++index) {
        pso_desc.RTVFormats[index] = d3d::Convert(p_desc.rtvFormats[index]);
    }
    pso_desc.DSVFormat = d3d::Convert(p_desc.dsvFormat);

    ID3D12Device4* device = reinterpret_cast<D3d12GraphicsManager*>(GraphicsManager::GetSingletonPtr())->GetDevice();
    D3D_FAIL_V(device->CreateGraphicsPipelineState(&pso_desc, IID_PPV_ARGS(&pipeline_state->pso)), nullptr);

    return pipeline_state;
}

}  // namespace my

#undef INCLUDE_AS_D3D12
