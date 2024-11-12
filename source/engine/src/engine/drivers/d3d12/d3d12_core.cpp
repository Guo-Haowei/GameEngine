#include "d3d12_core.h"

#include "drivers/d3d12/d3d12_graphics_manager.h"
#include "drivers/d3d_common/d3d_common.h"

namespace my {

//------------------------------------------------------------------------------
// FrameContext
void FrameContext::Wait(HANDLE p_fence_event, ID3D12Fence1* p_fence) {
    if (p_fence->GetCompletedValue() < m_fenceValue) {
        D3D_CALL(p_fence->SetEventOnCompletion(m_fenceValue, p_fence_event));
        WaitForSingleObject(p_fence_event, INFINITE);
    }
}

//------------------------------------------------------------------------------
// GraphicsContext
GraphicsContext::~GraphicsContext() {
    DEV_ASSERT(m_fenceEvent == NULL);
}

bool GraphicsContext::Initialize(D3d12GraphicsManager* p_device) {
    ID3D12Device4* const device = p_device->GetDevice();
    m_commandQueue = p_device->CreateCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT);
    DEV_ASSERT(m_commandQueue);

    for (int i = 0; i < static_cast<int>(m_frames.size()); ++i) {
        FrameContext& frame = m_frames[i];

        D3D_FAIL_V(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&frame.m_commandAllocator)), false);

        NAME_DX12_OBJECT_INDEXED(frame.m_commandAllocator, L"GraphicsCommandAllocator", i);

        // @TODO: refactor
        // frame.perFrameBuffer = std::make_unique<UploadBuffer<PerFrameConstants>>(dev, 1, true);
        // frame.perBatchBuffer = std::make_unique<UploadBuffer<PerBatchConstants>>(dev, 3000, true);
        // frame.boneConstants = std::make_unique<UploadBuffer<BoneConstants>>(dev, 3000, true);
    }

    D3D_FAIL_V(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_frames[0].m_commandAllocator, nullptr, IID_PPV_ARGS(&m_commandList)), false);

    m_commandList->Close();
    NAME_DX12_OBJECT(m_commandList, L"GraphicsCommandList");

    D3D_FAIL_V(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)), false);

    NAME_DX12_OBJECT(m_fence, L"GraphicsFence");

    m_fenceEvent = CreateEventEx(nullptr, nullptr, 0, EVENT_ALL_ACCESS);

    return true;
}

void GraphicsContext::Flush() {
    for (int i = 0; i < static_cast<int>(m_frames.size()); ++i) {
        m_frames[i].Wait(m_fenceEvent, m_fence.Get());
    }
    m_frameIndex = 0;
}

void GraphicsContext::Finalize() {
    Flush();

    m_fence.Reset();
    m_lastSignaledFenceValue = 0;

    CloseHandle(m_fenceEvent);
    m_fenceEvent = NULL;

    m_commandList.Reset();
    m_commandQueue.Reset();

    for (auto& frame : m_frames) {
        SafeRelease(frame.m_commandAllocator);
    }
}

FrameContext& GraphicsContext::BeginFrame() {
    FrameContext& frame = m_frames[m_frameIndex];
    frame.Wait(m_fenceEvent, m_fence.Get());
    D3D_CALL(frame.m_commandAllocator->Reset());
    D3D_CALL(m_commandList->Reset(frame.m_commandAllocator, nullptr));
    return frame;
}

void GraphicsContext::EndFrame() {
    D3D_CALL(m_commandList->Close());
    ID3D12CommandList* cmdLists[] = { m_commandList.Get() };
    m_commandQueue->ExecuteCommandLists(array_length(cmdLists), cmdLists);
}

void GraphicsContext::MoveToNextFrame() {
    uint64_t fenceValue = m_lastSignaledFenceValue + 1;
    m_commandQueue->Signal(m_fence.Get(), fenceValue);
    m_lastSignaledFenceValue = fenceValue;

    FrameContext& frame = m_frames[m_frameIndex];
    frame.m_fenceValue = fenceValue;

    m_frameIndex = (m_frameIndex + 1) % static_cast<uint32_t>(m_frames.size());
}

//------------------------------------------------------------------------------
// DescriptorHeapGPU
bool DescriptorHeapGPU::Initialize(D3D12_DESCRIPTOR_HEAP_TYPE p_type, uint32_t p_num_descriptors, ID3D12Device* p_device, bool p_shader_visible) {
    m_desc.Type = p_type;
    m_desc.NumDescriptors = p_num_descriptors;
    m_desc.Flags = p_shader_visible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    m_desc.NodeMask = 1;

    D3D_FAIL_V(p_device->CreateDescriptorHeap(&m_desc, IID_PPV_ARGS(&m_heap)), false);

    const wchar_t* debugName = nullptr;
    switch (p_type) {
        case D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV:
            debugName = L"SRV Heap";
            break;
        case D3D12_DESCRIPTOR_HEAP_TYPE_RTV:
            debugName = L"RTV Heap";
            break;
        case D3D12_DESCRIPTOR_HEAP_TYPE_DSV:
            debugName = L"DSV Heap";
            break;
        default:
            CRASH_NOW();
            break;
    }

    NAME_DX12_OBJECT(m_heap, debugName);
    m_incrementSize = p_device->GetDescriptorHandleIncrementSize(p_type);
    m_startCPU = m_heap->GetCPUDescriptorHandleForHeapStart();
    m_startGPU = m_heap->GetGPUDescriptorHandleForHeapStart();

    return true;
}

//------------------------------------------------------------------------------
// CopyContext
bool CopyContext::Initialize(D3d12GraphicsManager* p_device) {
    DEV_ASSERT(p_device);
    m_device = p_device;
    m_queue = p_device->CreateCommandQueue(D3D12_COMMAND_LIST_TYPE_COPY);
    DEV_ASSERT(m_queue);
    return true;
}

void CopyContext::Finalize() {
    m_queue.Reset();
    m_freeList.clear();
}

CopyContext::CopyCommand CopyContext::Allocate(uint32_t p_staging_size) {
    ID3D12Device4* const device = m_device->GetDevice();
    m_lock.lock();
    // create a new command list if there are no free ones
    if (m_freeList.empty()) {
        CopyCommand cmd;
        D3D_CALL(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_COPY,
                                                IID_PPV_ARGS(cmd.commandAllocator.GetAddressOf())));
        D3D_CALL(device->CreateCommandList1(0,
                                            D3D12_COMMAND_LIST_TYPE_COPY,
                                            D3D12_COMMAND_LIST_FLAG_NONE,
                                            IID_PPV_ARGS(cmd.commandList.GetAddressOf())));
        D3D_CALL(device->CreateFence(0,
                                     D3D12_FENCE_FLAG_NONE,
                                     IID_PPV_ARGS(cmd.fence.GetAddressOf())));
        m_freeList.emplace_back(cmd);
    }

    // Search for a staging buffer that can fit the request
    CopyCommand cmd = m_freeList.back();
    if (cmd.uploadBuffer.desc.byteWidth < p_staging_size) {
        for (size_t i = 0; i < m_freeList.size(); ++i) {
            if (m_freeList[i].uploadBuffer.desc.byteWidth >= p_staging_size) {
                cmd = m_freeList[i];
                std::swap(m_freeList[i], m_freeList.back());
                break;
            }
        }
    }
    m_freeList.pop_back();
    m_lock.unlock();

    // if no buffer was found, create one
    if (cmd.uploadBuffer.desc.byteWidth < p_staging_size) {
        if (cmd.uploadBuffer.buffer) {
            cmd.uploadBuffer.buffer.Reset();
        }

        cmd.uploadBuffer.desc.byteWidth = math::NextPowerOfTwo(p_staging_size);
        auto heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
        auto buffer = CD3DX12_RESOURCE_DESC::Buffer(cmd.uploadBuffer.desc.byteWidth);
        D3D_CALL(device->CreateCommittedResource(
            &heapProperties,
            D3D12_HEAP_FLAG_NONE,
            &buffer,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(cmd.uploadBuffer.buffer.GetAddressOf())));
    }
    // else
    //{
    //     LOG("Copy buffer reused of size ", staging_size);
    // }

    // begine command list in valid state
    D3D_CALL(cmd.commandAllocator->Reset());
    D3D_CALL(cmd.commandList->Reset(cmd.commandAllocator.Get(), nullptr));
    return cmd;
}

void CopyContext::Submit(CopyCommand p_cmd) {
    D3D_CALL(p_cmd.commandList->Close());
    ID3D12CommandList* commandLists[] = { p_cmd.commandList.Get() };
    m_queue->ExecuteCommandLists(array_length(commandLists), commandLists);
    D3D_CALL(m_queue->Signal(p_cmd.fence.Get(), 1));
    D3D_CALL(p_cmd.fence->SetEventOnCompletion(1, NULL));
    D3D_CALL(p_cmd.fence->Signal(0));

    m_lock.lock();
    m_freeList.push_back(p_cmd);
    m_lock.unlock();
}

}  // namespace my
