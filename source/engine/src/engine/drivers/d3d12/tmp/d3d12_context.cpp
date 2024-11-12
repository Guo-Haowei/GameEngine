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
        WIN_CALL(m_device->CreateCommittedResource(
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
        WIN_CALL(m_device->CreateCommittedResource(
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

void D3D12Context::finalize()
{
    CleanupRenderTarget();

    ImGui_ImplDX12_Shutdown();

    m_graphicsContext.Finalize();
    m_copyContext.Finalize();
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
        WIN_CALL(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error));
        WIN_CALL(m_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature)));
    }

    return true;
}

bool D3D12Context::CreateTexture(const Image& image)
{
    D3D12_HEAP_PROPERTIES props;
    memset(&props, 0, sizeof(D3D12_HEAP_PROPERTIES));
    props.Type = D3D12_HEAP_TYPE_DEFAULT;
    props.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    props.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

    D3D12_RESOURCE_DESC desc;
    ZeroMemory(&desc, sizeof(desc));
    desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    desc.Alignment = 0;
    desc.Width = image.width;
    desc.Height = image.height;
    desc.DepthOrArraySize = 1;
    desc.MipLevels = 1;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;
    desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    desc.Flags = D3D12_RESOURCE_FLAG_NONE;

    ID3D12Resource* pTexture = nullptr;
    HRESULT hr = WIN_CALL(m_device->CreateCommittedResource(&props, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_COPY_DEST, NULL, IID_PPV_ARGS(&pTexture)));
    if (FAILED(hr))
    {
        return false;
    }

    // Create a temporary upload resource to move the data in
    UINT uploadPitch = ALIGN((image.width * 4), D3D12_TEXTURE_DATA_PITCH_ALIGNMENT);
    UINT uploadSize = image.height * uploadPitch;
    desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    desc.Alignment = 0;
    desc.Width = uploadSize;
    desc.Height = 1;
    desc.DepthOrArraySize = 1;
    desc.MipLevels = 1;
    desc.Format = DXGI_FORMAT_UNKNOWN;
    desc.SampleDesc = { 1, 0 };
    desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    desc.Flags = D3D12_RESOURCE_FLAG_NONE;

    props.Type = D3D12_HEAP_TYPE_UPLOAD;
    props.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    props.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

    ComPtr<ID3D12Resource> uploadBuffer;
    hr = WIN_CALL(m_device->CreateCommittedResource(&props, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_GENERIC_READ, NULL, IID_PPV_ARGS(&uploadBuffer)));
    if (FAILED(hr))
    {
        return false;
    }

    // Write pixels into the upload resource
    void* mapped = NULL;
    D3D12_RANGE range = { 0, uploadSize };
    hr = uploadBuffer->Map(0, &range, &mapped);
    DEV_ASSERT(SUCCEEDED(hr));
    for (uint32_t y = 0; y < image.height; y++)
    {
        memcpy((void*)((uintptr_t)mapped + y * uploadPitch), image.pixels.data() + y * image.width * 4, image.width * 4);
    }
    uploadBuffer->Unmap(0, &range);

    // Copy the upload resource content into the real resource
    D3D12_TEXTURE_COPY_LOCATION srcLocation = {};
    srcLocation.pResource = uploadBuffer.Get();
    srcLocation.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
    srcLocation.PlacedFootprint.Footprint.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    srcLocation.PlacedFootprint.Footprint.Width = image.width;
    srcLocation.PlacedFootprint.Footprint.Height = image.height;
    srcLocation.PlacedFootprint.Footprint.Depth = 1;
    srcLocation.PlacedFootprint.Footprint.RowPitch = uploadPitch;

    D3D12_TEXTURE_COPY_LOCATION dstLocation = {};
    dstLocation.pResource = pTexture;
    dstLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    dstLocation.SubresourceIndex = 0;

    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.Transition.pResource = pTexture;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;

    // Create a temporary command queue to do the copy with
    ComPtr<ID3D12Fence> fence;
    hr = WIN_CALL(m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)));
    if (FAILED(hr))
    {
        return false;
    }

    HANDLE event = CreateEvent(0, 0, 0, 0);
    if (event == NULL)
    {
        return false;
    }

    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    queueDesc.NodeMask = 1;

    ComPtr<ID3D12CommandQueue> cmdQueue;
    hr = WIN_CALL(m_device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&cmdQueue)));
    if (FAILED(hr))
    {
        return false;
    }

    ComPtr<ID3D12CommandAllocator> cmdAlloc;
    hr = WIN_CALL(m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&cmdAlloc)));
    if (FAILED(hr))
    {
        return false;
    }

    ComPtr<ID3D12GraphicsCommandList> cmdList;
    hr = WIN_CALL(m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, cmdAlloc.Get(), NULL, IID_PPV_ARGS(&cmdList)));
    if (FAILED(hr))
    {
        return false;
    }

    cmdList->CopyTextureRegion(&dstLocation, 0, 0, 0, &srcLocation, NULL);
    cmdList->ResourceBarrier(1, &barrier);

    WIN_CALL(hr = cmdList->Close());

    // Execute the copy
    ID3D12CommandList* commandLists[] = { cmdList.Get() };
    cmdQueue->ExecuteCommandLists(array_length(commandLists), commandLists);
    hr = WIN_CALL(cmdQueue->Signal(fence.Get(), 1));

    // Wait for everything to complete
    fence->SetEventOnCompletion(1, event);
    WaitForSingleObject(event, INFINITE);

    // Tear down our temporary command queue and release the upload resource
    CloseHandle(event);

    image.index = m_textureCounter.fetch_add(1);
    CD3DX12_CPU_DESCRIPTOR_HANDLE cpuHandle(m_srvDescHeap.m_startCPU);
    cpuHandle.Offset(image.index, m_srvDescHeap.m_incrementSize);
    CD3DX12_GPU_DESCRIPTOR_HANDLE gpuHandle(m_srvDescHeap.m_startGPU);
    gpuHandle.Offset(image.index, m_srvDescHeap.m_incrementSize);

    // Create a shader resource view for the texture
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc;
    ZeroMemory(&srvDesc, sizeof(srvDesc));
    srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = desc.MipLevels;
    srvDesc.Texture2D.MostDetailedMip = 0;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    m_device->CreateShaderResourceView(pTexture, &srvDesc, cpuHandle);

    // Return results
    m_textures.emplace_back(ComPtr<ID3D12Resource>(pTexture));
    image.cpuHandle = cpuHandle.ptr;
    image.gpuHandle = gpuHandle.ptr;
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
            WIN_CALL(m_device->CreateCommittedResource(
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
    auto internalState = std::make_shared<PipelineState_DX12>();
    ComPtr<ID3DBlob> vsBlob = CompileShaderFromFile(
        desc.vertexShader.c_str(), SHADER_TYPE_VERTEX, true);
    if (!vsBlob)
    {
        return false;
    }
    ComPtr<ID3DBlob> psBlob = CompileShaderFromFile(
        desc.pixelShader.c_str(), SHADER_TYPE_PIXEL, true);
    if (!psBlob)
    {
        return false;
    }

    std::vector<D3D12_INPUT_ELEMENT_DESC> elements;
    elements.reserve(desc.inputlayoutDesc.elements.size());
    for (const auto& ele : desc.inputlayoutDesc.elements)
    {
        D3D12_INPUT_ELEMENT_DESC ildesc;
        ildesc.SemanticName = ele.semanticName.c_str();
        ildesc.SemanticIndex = ele.semanticIndex;
        ildesc.Format = Convert(ele.format);
        ildesc.InputSlot = ele.inputSlot;
        ildesc.AlignedByteOffset = ele.alignedByteOffset;
        ildesc.InputSlotClass = Convert(ele.inputSlotClass);
        ildesc.InstanceDataStepRate = ele.instanceDataStepRate;
        elements.push_back(ildesc);
    }
    DEV_ASSERT(elements.size());

    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};

    D3D12_PRIMITIVE_TOPOLOGY_TYPE tp = D3D12_PRIMITIVE_TOPOLOGY_TYPE_UNDEFINED;
    switch (desc.primitiveTopologyType)
    {
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

    if (FAILED(WIN_CALL(m_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&internalState->pso)))))
    {
        return false;
    }

    out_pso.internalState = internalState;
    out_pso.desc = desc;

    return true;
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
