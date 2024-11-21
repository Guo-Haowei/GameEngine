#pragma once
#include <d3d12.h>
#include <d3dx12/d3dx12.h>
#include <dxgi1_6.h>

#include <atomic>
#include <mutex>

#include "core/framework/graphics_manager.h"
#include "descriptor_table_defines.hlsl.h"
#include "drivers/d3d_common/d3d_common.h"

namespace my {

class D3d12GraphicsManager;

enum class DescriptorResourceType : uint8_t {
#define DESCRIPTOR_ENUM(ENUM, ...) ENUM,
#define DESCRIPTOR_SRV             DESCRIPTOR_ENUM
#define DESCRIPTOR_UAV             DESCRIPTOR_ENUM
    DESCRIPTOR_TABLE
#undef DESCRIPTOR_SRV
#undef DESCRIPTOR_UAV
#undef DESCRIPTOR_ENUM
        COUNT
};

enum DescriptorHeapConstants {
    _RESOURCE_DUMMY_START = 0,
    _RESOURCE_DUMMY_MAX_COUNT = 0,
#define DESCRIPTOR_CONSTANTS(ENUM, NUM, PREV, ...) \
    ENUM##_MAX_COUNT = NUM,                        \
    ENUM##_START = PREV##_START + PREV##_MAX_COUNT,
#define DESCRIPTOR_SRV DESCRIPTOR_CONSTANTS
#define DESCRIPTOR_UAV DESCRIPTOR_CONSTANTS
    DESCRIPTOR_TABLE
#undef DESCRIPTOR_SRV
#undef DESCRIPTOR_UAV
#undef DESCRIPTOR_CONSTANTS
};

struct DescriptorHeapHandle {
    int index;
    D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle;
    D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle;
};

class DescriptorHeapBase {
public:
    virtual ~DescriptorHeapBase() = default;

    bool Initialize(D3D12_DESCRIPTOR_HEAP_TYPE p_type,
                    ID3D12Device* p_device,
                    bool p_shard_visible);

    D3D12_CPU_DESCRIPTOR_HANDLE GetStartCpu() const { return m_startCpu; }
    D3D12_GPU_DESCRIPTOR_HANDLE GetStartGpu() const { return m_startGpu; };
    ID3D12DescriptorHeap* GetHeap() { return m_heap.Get(); }

    uint32_t GetIncrementSize() const { return m_incrementSize; }

protected:
    struct Space {
        int start;
        int max;
        mutable std::atomic_int counter;
    };

    DescriptorHeapHandle AllocHandle(const Space& p_space);

    virtual uint32_t GetCapacity() const = 0;

    D3D12_DESCRIPTOR_HEAP_DESC m_desc{};
    D3D12_CPU_DESCRIPTOR_HANDLE m_startCpu{};
    D3D12_GPU_DESCRIPTOR_HANDLE m_startGpu{};
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_heap{};

    uint32_t m_incrementSize{ 0 };
};

class DescriptorHeap : public DescriptorHeapBase {
    using Base = DescriptorHeapBase;

public:
    DescriptorHeap(int p_num_descriptors);

    DescriptorHeapHandle AllocHandle();

protected:
    uint32_t GetCapacity() const override {
        return m_space.max;
    }

    Space m_space;
};

class DescriptorHeapSrv : public DescriptorHeapBase {
    using Base = DescriptorHeapBase;

public:
    DescriptorHeapSrv();

    DescriptorHeapHandle AllocHandle(DescriptorResourceType p_type);

protected:
    uint32_t GetCapacity() const override {
        return RWTexture3D_START + RWTexture3D_MAX_COUNT;
    }

    Space m_spaces[std::to_underlying(DescriptorResourceType::COUNT)];
};

// @TODO: refactor, this is duplicate
struct GPUBufferDesc {
    uint32_t byteWidth = 0;
};

struct GPUBuffer {
    GPUBufferDesc desc;
    Microsoft::WRL::ComPtr<ID3D12Resource> buffer;
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