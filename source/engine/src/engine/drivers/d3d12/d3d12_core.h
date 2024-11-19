#pragma once
#include <d3d12.h>
#include <d3dx12/d3dx12.h>
#include <dxgi1_6.h>

#include <atomic>
#include <mutex>

#include "core/framework/graphics_manager.h"
#include "drivers/d3d_common/d3d_common.h"

namespace my {

// @TODO: refactor, this is duplicate
struct GPUBufferDesc {
    uint32_t byteWidth = 0;
};

struct GPUBuffer {
    GPUBufferDesc desc;
    Microsoft::WRL::ComPtr<ID3D12Resource> buffer;
};

class D3d12GraphicsManager;

class DescriptorHeap {
public:
    struct Handle {
        int index;
        D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle;
        D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle;
    };

    bool Initialize(int p_start, D3D12_DESCRIPTOR_HEAP_TYPE p_type, uint32_t p_num_descriptors, ID3D12Device* p_device, bool p_shard_visible);

    Handle AllocHandle();

    D3D12_CPU_DESCRIPTOR_HANDLE GetStartCpu() const { return m_startCpu; }
    D3D12_GPU_DESCRIPTOR_HANDLE GetStartGpu() const { return m_startGpu; };
    ID3D12DescriptorHeap* GetHeap() { return m_heap.Get(); }

    uint32_t GetIncrementSize() const { return m_incrementSize; }

protected:
    D3D12_DESCRIPTOR_HEAP_DESC m_desc{};
    D3D12_CPU_DESCRIPTOR_HANDLE m_startCpu{};
    D3D12_GPU_DESCRIPTOR_HANDLE m_startGpu{};
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_heap{};

    uint32_t m_incrementSize{ 0 };
    std::atomic_int m_counter{ 0 };
};

class DescriptorHeapSrv : public DescriptorHeap {
public:
    Handle AllocHandle(Dimension p_dimension);

private:
    Handle AllocHandle(int p_begin, int p_max, std::atomic_int& p_counter);

    const int m_2dArrayStart = 1;
    const int m_2dArrayMax = MAX_TEXTURE_2D_COUNT;

    const int m_cubeArrayStart = MAX_TEXTURE_2D_COUNT;
    const int m_cubeArrayMax = MAX_TEXTURE_CUBE_ARRAY_COUNT;

    std::atomic_int m_2dArrayCounter = m_2dArrayStart;
    std::atomic_int m_cubeArrayCounter = m_cubeArrayStart;
};

class CopyContext {
public:
    struct CopyCommand {
        Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator;
        Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList;
        Microsoft::WRL::ComPtr<ID3D12Fence> fence;
        GPUBuffer uploadBuffer;
        void* data = nullptr;
        // ID3D12Resource* uploadResource = nullptr;
    };

    bool Initialize(D3d12GraphicsManager* p_device);

    void Finalize();

    CopyCommand Allocate(uint32_t p_staging_size);

    void Submit(CopyCommand p_cmd);

private:
    D3d12GraphicsManager* m_device = nullptr;
    Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_queue;
    std::vector<CopyCommand> m_freeList;
    std::mutex m_lock;
};

}  // namespace my