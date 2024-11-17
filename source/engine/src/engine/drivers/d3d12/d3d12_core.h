#pragma once
#include <d3d12.h>
#include <d3dx12/d3dx12.h>
#include <dxgi1_6.h>

#include <atomic>
#include <mutex>

#include "core/framework/graphics_manager.h"
#include "drivers/d3d_common/d3d_common.h"

namespace my {

// @TODO: refactor
struct GPUBufferDesc {
    uint32_t byteWidth = 0;
    // Usage usage = Usage::DEFAULT;
    // uint32_t bindFlags = 0;
    // uint32_t cpuAccessFlag = 0;
    // uint32_t stride = 0;
};

struct GPUBuffer {
    GPUBufferDesc desc;
    Microsoft::WRL::ComPtr<ID3D12Resource> buffer;
};

class D3d12GraphicsManager;

// @TODO: remove
template<typename T>
class UploadBuffer {
    UploadBuffer(const UploadBuffer&) = delete;
    UploadBuffer& operator=(const UploadBuffer&) = delete;

    static constexpr size_t GetSize(size_t p_size, bool p_is_constant_buffer) {
        if (p_is_constant_buffer) {
            return math::Align(p_size, 256);
        }
        return p_size;
    }

public:
    UploadBuffer(ID3D12Device* p_device, uint32_t p_element_count, bool p_is_constant_buffer)
        : m_elementSizeInByte((uint32_t)GetSize(sizeof(T), p_is_constant_buffer)), m_elementCount(p_element_count) {
        auto heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
        auto buffer = CD3DX12_RESOURCE_DESC::Buffer(m_elementSizeInByte * p_element_count);
        D3D_CALL(p_device->CreateCommittedResource(
            &heapProperties,
            D3D12_HEAP_FLAG_NONE,
            &buffer,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr, IID_PPV_ARGS(&m_gpuBuffer)));

        D3D_CALL(m_gpuBuffer->Map(0, nullptr, reinterpret_cast<void**>(&m_mappedData)));
    }

    ~UploadBuffer() {
        if (m_gpuBuffer != nullptr) {
            m_gpuBuffer->Unmap(0, nullptr);
        }
        m_mappedData = nullptr;
    }

    ID3D12Resource* Resource() const { return m_gpuBuffer.Get(); }

    void CopyData(const void* data, size_t size_in_byte) {
        assert(size_in_byte <= m_elementCount * m_elementSizeInByte);
        memcpy(m_mappedData, data, size_in_byte);
    }

private:
    const uint32_t m_elementSizeInByte = 0;
    const uint32_t m_elementCount = 0;
    Microsoft::WRL::ComPtr<ID3D12Resource> m_gpuBuffer;
    uint8_t* m_mappedData = nullptr;
    bool m_isConstantBuffer = false;
};

class DescriptorHeapGPU {
public:
    struct Handle {
        int index;
        D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle;
        D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle;
    };

    bool Initialize(int p_start, D3D12_DESCRIPTOR_HEAP_TYPE p_type, uint32_t p_num_descriptors, ID3D12Device* p_device, bool p_shard_visible = false);

    Handle AllocHandle();

    D3D12_CPU_DESCRIPTOR_HANDLE GetStartCpu() const { return m_startCpu; }
    D3D12_GPU_DESCRIPTOR_HANDLE GetStartGpu() const { return m_startGpu; };
    ID3D12DescriptorHeap* GetHeap() { return m_heap.Get(); }

private:
    D3D12_DESCRIPTOR_HEAP_DESC m_desc{};
    D3D12_CPU_DESCRIPTOR_HANDLE m_startCpu{};
    D3D12_GPU_DESCRIPTOR_HANDLE m_startGpu{};
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_heap{};

    uint32_t m_incrementSize{ 0 };
    std::atomic_int m_counter{ 0 };
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