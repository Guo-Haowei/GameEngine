#include "d3d12_core.h"

#include "drivers/d3d12/d3d12_graphics_manager.h"
#include "drivers/d3d_common/d3d_common.h"

namespace my {

static constexpr int DESCRIPTOR_MAX_COUNT = 512;

//------------------------------------------------------------------------------
// DescriptorHeapBase
auto DescriptorHeapBase::Initialize(int p_count, D3D12_DESCRIPTOR_HEAP_TYPE p_type, ID3D12Device* p_device, bool p_shader_visible) -> Result<void> {
    m_desc.Type = p_type;
    m_desc.NumDescriptors = DESCRIPTOR_MAX_COUNT;
    m_desc.Flags = p_shader_visible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    m_desc.NodeMask = 1;
    m_capacity = p_count;

    const char* name = "";
    switch (p_type) {
        case D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV:
            name = "CBV_SRV_UAV";
            break;
        case D3D12_DESCRIPTOR_HEAP_TYPE_RTV:
            name = "RTV Heap";
            break;
        case D3D12_DESCRIPTOR_HEAP_TYPE_DSV:
            name = "DSV Heap";
            break;
        default:
            CRASH_NOW();
            break;
    }

    D3D_FAIL(p_device->CreateDescriptorHeap(&m_desc, IID_PPV_ARGS(&m_heap)), "Failed to create heap {}", name);

    D3D12_SET_DEBUG_NAME(m_heap.Get(), name);

    m_incrementSize = p_device->GetDescriptorHandleIncrementSize(p_type);
    m_startCpu = m_heap->GetCPUDescriptorHandleForHeapStart();
    m_startGpu = m_heap->GetGPUDescriptorHandleForHeapStart();

    return Result<void>();
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
DescriptorHeap::DescriptorHeap() {
    m_space.start = 0;
    m_space.counter = 0;
}

DescriptorHeapHandle DescriptorHeap::AllocHandle() {
    m_space.max = m_capacity;
    return Base::AllocHandle(m_space);
}

//------------------------------------------------------------------------------
// DescriptorSrv
auto DescriptorHeapSrv::Initialize(int p_count,
                                   D3D12_DESCRIPTOR_HEAP_TYPE p_type,
                                   ID3D12Device* p_device,
                                   bool p_shard_visible) -> Result<void> {
    m_capacity = p_count;

    DEV_ASSERT(GetBindlessMax() < m_capacity);

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
    m_backCounter = m_capacity - 1;

    return DescriptorHeapBase::Initialize(p_count, p_type, p_device, p_shard_visible);
}

DescriptorHeapHandle DescriptorHeapSrv::AllocHandle() {
    int index = m_backCounter.fetch_add(-1);
    DEV_ASSERT(index > GetBindlessMax() && index < m_capacity);
    CD3DX12_CPU_DESCRIPTOR_HANDLE cpu_handle(m_startCpu);
    cpu_handle.Offset(index, m_incrementSize);
    CD3DX12_GPU_DESCRIPTOR_HANDLE gpu_handle(m_startGpu);
    gpu_handle.Offset(index, m_incrementSize);

    return {
        index,
        cpu_handle,
        gpu_handle,
    };
}

DescriptorHeapHandle DescriptorHeapSrv::AllocBindlessHandle(DescriptorResourceType p_type) {
    DEV_ASSERT_INDEX(p_type, DescriptorResourceType::COUNT);
    return Base::AllocHandle(m_spaces[std::to_underlying(p_type)]);
}

//------------------------------------------------------------------------------
// CopyContext
auto CopyContext::Initialize(D3d12GraphicsManager* p_device) -> Result<void> {
    DEV_ASSERT(p_device);
    m_device = p_device;
    m_queue = p_device->CreateCommandQueue(D3D12_COMMAND_LIST_TYPE_COPY);

    if (DEV_VERIFY(m_queue)) {
        return Result<void>();
    }
    return HBN_ERROR(ErrorCode::ERR_CANT_CREATE, "failed to create command queue");
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
