#if 0
#include "d3d12_context.h"

#include <imgui/backends/imgui_impl_dx12.h>

#include "Scene/Scene.h"
#include "core/framework/AssetManager.h"
#include "servers/display_server.h"

namespace vct
{

template<typename T>
inline size_t SizeInByte(const std::vector<T>& vec)
{
    static_assert(std::is_trivially_copyable<T>());
    return vec.size() * sizeof(T);
}

static PipelineState_DX12& Internal(const PipelineState& in)
{
    return *static_cast<PipelineState_DX12*>(in.internalState.get());
}

#define INCLUDE_DX12
#include "DXEnums.inl"

struct MeshGeometry
{
    Microsoft::WRL::ComPtr<ID3D12Resource> VertexBufferGPU = nullptr;
    Microsoft::WRL::ComPtr<ID3D12Resource> IndexBufferGPU = nullptr;
};

static std::vector<std::shared_ptr<MeshGeometry>> s_geometries;

ErrorCode D3D12Context::initialize(const CreateInfo& info)
{
    ErrorCode ret = GraphicsManager::initialize(info);
    ERR_FAIL_COND_V(ret != OK, ret);

    auto [w, h] = DisplayServer::singleton().get_window_size();
    m_hwnd = reinterpret_cast<HWND>(DisplayServer::singleton().get_native_window());

    std::expected<void, Error<HRESULT>> res;

    bool ok = true;

    ok = ok && (res = create_device());

    if (m_enable_debug_layer)
    {
        if (EnableDebugLayer())
        {
            LOG("[GraphicsManager_DX12] Debug layer enabled");
        }
        else
        {
            LOG_ERROR("[GraphicsManager_DX12] Debug layer not enabled");
        }
    }

    ok = ok && (res = create_descriptor_heaps());
    ok = ok && (res = m_graphicsContext.initialize(this));
    ok = ok && (res = m_copyContext.initialize(this));

    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = m_rtvDescHeap.m_startCPU;
    for (int i = 0; i < NUM_BACK_BUFFERS; i++)
    {
        m_renderTargetDescriptor[i] = rtvHandle;
        rtvHandle.ptr += m_rtvDescHeap.m_incrementSize;
    }

    m_depthStencilDescriptor = m_dsvDescHeap.m_startCPU;

    ok = ok && (res = create_swap_chain(w, h));
    ok = ok && (res = create_render_target(w, h));

    if (!ok)
    {
        auto err = res.error();
        report_error_impl(err.function, err.filepath, err.line, err.get_message());
        return ERR_CANT_CREATE;
    }

    LoadAssets();

    // Create debug buffer.
    {
        size_t bufferSize = sizeof(PosColor) * 1000;  // hard code
        auto heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
        auto bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);
        D3D_CALL(m_device->CreateCommittedResource(
            &heapProperties,
            D3D12_HEAP_FLAG_NONE,
            &bufferDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&m_debugVertexData)));
    }
    {
        size_t bufferSize = sizeof(uint32_t) * 3000;  // hard code
        auto heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
        auto bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);
        D3D_CALL(m_device->CreateCommittedResource(
            &heapProperties,
            D3D12_HEAP_FLAG_NONE,
            &bufferDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&m_debugIndexData)));
    }

    ImGui_ImplDX12_Init(m_device.Get(),
                        NUM_FRAMES_IN_FLIGHT,
                        DXGI_FORMAT_R8G8B8A8_UNORM,
                        m_srvDescHeap.m_heap.Get(),
                        m_srvDescHeap.m_startCPU,
                        m_srvDescHeap.m_startGPU);
    ImGui_ImplDX12_CreateDeviceObjects();

    return OK;
}

bool D3D12Context::LoadAssets()
{
    // Create a root signature consisting of a descriptor table with a single CBV.
    {
        CD3DX12_DESCRIPTOR_RANGE texTable;
        texTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
                      1,   // number of descriptors
                      0);  // register t0

        CD3DX12_ROOT_PARAMETER rootParams[4];
        rootParams[0].InitAsDescriptorTable(1, &texTable, D3D12_SHADER_VISIBILITY_PIXEL);
        rootParams[1].InitAsConstantBufferView(0);
        rootParams[2].InitAsConstantBufferView(1);
        rootParams[3].InitAsConstantBufferView(2);

        auto staticSamplers = GetStaticSamplers();

        // A root signature is an array of root parameters.
        CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc(array_length(rootParams),
                                                      rootParams,
                                                      (uint32_t)staticSamplers.size(),
                                                      staticSamplers.data(),
                                                      D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

        ComPtr<ID3DBlob> signature;
        ComPtr<ID3DBlob> error;
        D3D_CALL(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error));
        D3D_CALL(m_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature)));
    }

    return true;
}

void D3D12Context::BeginScene(const Scene& scene)
{
    for (const MeshComponent& mesh : scene.get_component_array<MeshComponent>())
    {
        const std::vector<std::uint32_t>& indices = mesh.indices;

        std::vector<char> vertices = mesh.GenerateCombinedBuffer();

        auto geometry = std::make_shared<MeshGeometry>();

        const UINT ibByteSize = (UINT)SizeInByte(indices);
        const UINT vbByteSize = (UINT)SizeInByte(vertices);

        auto UploadBuffer = [&](uint32_t byte_size, const void* init_data, ID3D12GraphicsCommandList* command_list, ID3D12Resource* intermidate) {
            ID3D12Resource* buffer;
            auto heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
            auto bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(byte_size);
            // Create the actual default buffer resource.
            D3D_CALL(m_device->CreateCommittedResource(
                &heapProperties,
                D3D12_HEAP_FLAG_NONE,
                &bufferDesc,
                D3D12_RESOURCE_STATE_COMMON,
                nullptr,
                IID_PPV_ARGS(&buffer)));

            // Describe the data we want to copy into the default buffer.
            D3D12_SUBRESOURCE_DATA subResourceData = {};
            subResourceData.pData = init_data;
            subResourceData.RowPitch = byte_size;
            subResourceData.SlicePitch = subResourceData.RowPitch;

            UpdateSubresources<1>(command_list, buffer, intermidate, 0, 0, 1, &subResourceData);
            return buffer;
        };

        {
            auto cmd = m_copyContext.Allocate(vbByteSize);
            geometry->VertexBufferGPU = UploadBuffer(vbByteSize, vertices.data(), cmd.commandList.Get(), cmd.uploadBuffer.buffer.Get());
            m_copyContext.Submit(cmd);
        }
        {
            auto cmd = m_copyContext.Allocate(ibByteSize);
            geometry->IndexBufferGPU = UploadBuffer(ibByteSize, indices.data(), cmd.commandList.Get(), cmd.uploadBuffer.buffer.Get());
            m_copyContext.Submit(cmd);
        }

        s_geometries.emplace_back(geometry);
        mesh.gpuResource = s_geometries.back().get();
    }
}

void D3D12Context::EndScene()
{
    // s_geometries.clear();
}

void D3D12Context::BeginFrame()
{
    // @TODO: wait for swap chain
    FrameContext& frame = m_graphicsContext.BeginFrame();

    WaitForSingleObject(m_swapChainWaitObject, INFINITE);

    m_currentFrameContext = &frame;
    m_backbufferIndex = m_swap_chain->GetCurrentBackBufferIndex();
}

void D3D12Context::EndFrame()
{
    m_graphicsContext.EndFrame();
}

template<typename T>
static uint32_t SizeOfVector(const std::vector<T>& vec)
{
    return static_cast<uint32_t>(vec.size() * sizeof(vec[0]));
}

bool D3D12Context::CreatePipelineState(const PipelineStateDesc& desc, PipelineState& out_pso)
{
}

void D3D12Context::SetPipelineState(PipelineState& pipeline_state, CommandList cmd)
{
    unused(cmd);

    PipelineState_DX12& pso = Internal(pipeline_state);
    ID3D12GraphicsCommandList* cmdList = m_graphicsContext.m_commandList.Get();
    cmdList->SetPipelineState(pso.pso.Get());
}

void D3D12Context::CleanupRenderTarget()
{
    m_graphicsContext.Flush();

    for (uint32_t i = 0; i < NUM_BACK_BUFFERS; ++i)
    {
        SafeRelease(m_renderTargets[i]);
    }
    SafeRelease(m_depthStencilBuffer);
}

}  // namespace vct
#endif
