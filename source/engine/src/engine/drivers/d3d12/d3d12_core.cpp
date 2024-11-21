#include "d3d12_core.h"

#include "drivers/d3d12/d3d12_graphics_manager.h"
#include "drivers/d3d_common/d3d_common.h"

namespace my {

//------------------------------------------------------------------------------
// DescriptorHeapBase
bool DescriptorHeapBase::Initialize(D3D12_DESCRIPTOR_HEAP_TYPE p_type, ID3D12Device* p_device, bool p_shader_visible) {
    m_desc.Type = p_type;
    m_desc.NumDescriptors = GetCapacity();
    m_desc.Flags = p_shader_visible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    m_desc.NodeMask = 1;

    D3D_FAIL_V(p_device->CreateDescriptorHeap(&m_desc, IID_PPV_ARGS(&m_heap)), false);

#if USING(USE_D3D_DEBUG_NAME)
    switch (p_type) {
        case D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV:
            D3D12_SET_DEBUG_NAME(m_heap.Get(), "SRV Heap");
            break;
        case D3D12_DESCRIPTOR_HEAP_TYPE_RTV:
            D3D12_SET_DEBUG_NAME(m_heap.Get(), "RTV Heap");
            break;
        case D3D12_DESCRIPTOR_HEAP_TYPE_DSV:
            D3D12_SET_DEBUG_NAME(m_heap.Get(), "DSV Heap");
            break;
        default:
            CRASH_NOW();
            break;
    }
#endif

    m_incrementSize = p_device->GetDescriptorHandleIncrementSize(p_type);
    m_startCpu = m_heap->GetCPUDescriptorHandleForHeapStart();
    m_startGpu = m_heap->GetGPUDescriptorHandleForHeapStart();

    return true;
}

DescriptorHeapHandle DescriptorHeapBase::AllocHandle(const Space& p_space) {
    int index = p_space.counter.fetch_add(1);
    DEV_ASSERT(index >= p_space.start && index < p_space.start + p_space.max);
    CD3DX12_CPU_DESCRIPTOR_HANDLE cpu_handle(m_startCpu);
    cpu_handle.Offset(index, m_incrementSize);
    CD3DX12_GPU_DESCRIPTOR_HANDLE gpu_handle(m_startGpu);
    gpu_handle.Offset(index, m_incrementSize);

    return {
        index - p_space.start,
        cpu_handle,
        gpu_handle,
    };
}

//------------------------------------------------------------------------------
// DescriptorHeap
DescriptorHeap::DescriptorHeap(int p_num_descriptors) {
    m_space.max = p_num_descriptors;
    m_space.start = 0;
    m_space.counter = 0;
}

DescriptorHeapHandle DescriptorHeap::AllocHandle() {
    return Base::AllocHandle(m_space);
}

//------------------------------------------------------------------------------
// DescriptorSrv
DescriptorHeapSrv::DescriptorHeapSrv() {

#define DESCRIPTOR_SPACE(ENUM, NUM, PREV, SPACE, ...) \
    do {                                              \
        m_spaces[SPACE].start = ENUM##_START;         \
        m_spaces[SPACE].max = ENUM##_MAX_COUNT;       \
        m_spaces[SPACE].counter = ENUM##_START;       \
    } while (0);
#define DESCRIPTOR_SRV DESCRIPTOR_SPACE
#define DESCRIPTOR_UAV DESCRIPTOR_SPACE
    DESCRIPTOR_TABLE
#undef DESCRIPTOR_SRV
#undef DESCRIPTOR_UAV
#undef DESCRIPTOR_SPACE

    // reserve 1 for imgui
    m_spaces[0].counter = 1;
}

DescriptorHeapHandle DescriptorHeapSrv::AllocHandle(DescriptorResourceType p_type) {
    DEV_ASSERT_INDEX(p_type, DescriptorResourceType::COUNT);
    return Base::AllocHandle(m_spaces[std::to_underlying(p_type)]);
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
