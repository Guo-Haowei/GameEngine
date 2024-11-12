#pragma once
#include <d3d12.h>
#include <d3dx12/d3dx12.h>
#include <dxgi1_6.h>

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

struct FrameContext {
    ID3D12CommandAllocator* m_commandAllocator = nullptr;
    uint64_t m_fenceValue = 0;

    // @TODO: fix
    // std::unique_ptr<UploadBuffer<PerFrameConstants>> perFrameBuffer;
    // std::unique_ptr<UploadBuffer<PerBatchConstants>> perBatchBuffer;
    // std::unique_ptr<UploadBuffer<BoneConstants>> boneConstants;

    void Wait(HANDLE p_fence_event, ID3D12Fence1* p_fence);
};

struct DescriptorHeapGPU {
    D3D12_DESCRIPTOR_HEAP_DESC m_desc = {};
    D3D12_CPU_DESCRIPTOR_HANDLE m_startCPU = {};
    D3D12_GPU_DESCRIPTOR_HANDLE m_startGPU = {};
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_heap;

    uint32_t m_incrementSize = 0;

    bool Initialize(D3D12_DESCRIPTOR_HEAP_TYPE p_type, uint32_t p_num_descriptors, ID3D12Device* p_device, bool p_shard_visible = false);
};

struct GraphicsContext {
    GraphicsContext() = default;
    ~GraphicsContext();

    bool Initialize(D3d12GraphicsManager* p_device);
    void Finalize();

    FrameContext& BeginFrame();
    void EndFrame();
    void MoveToNextFrame();
    void Flush();

    Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_commandQueue;
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> m_commandList;
    Microsoft::WRL::ComPtr<ID3D12Fence1> m_fence;

    uint64_t m_lastSignaledFenceValue = 0;
    HANDLE m_fenceEvent = NULL;

    std::array<FrameContext, GraphicsManager::NUM_FRAMES_IN_FLIGHT> m_frames;
    uint32_t m_frameIndex = 0;
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

// @TODO: refactor
static std::array<const CD3DX12_STATIC_SAMPLER_DESC, 6> GetStaticSamplers() {
    // Applications usually only need a handful of samplers.  So just define them all up front
    // and keep them available as part of the root signature.

    const CD3DX12_STATIC_SAMPLER_DESC pointWrap(
        0,                                 // shaderRegister
        D3D12_FILTER_MIN_MAG_MIP_POINT,    // filter
        D3D12_TEXTURE_ADDRESS_MODE_WRAP,   // addressU
        D3D12_TEXTURE_ADDRESS_MODE_WRAP,   // addressV
        D3D12_TEXTURE_ADDRESS_MODE_WRAP);  // addressW

    const CD3DX12_STATIC_SAMPLER_DESC pointClamp(
        1,                                  // shaderRegister
        D3D12_FILTER_MIN_MAG_MIP_POINT,     // filter
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP,   // addressU
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP,   // addressV
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP);  // addressW

    const CD3DX12_STATIC_SAMPLER_DESC linearWrap(
        2,                                 // shaderRegister
        D3D12_FILTER_MIN_MAG_MIP_LINEAR,   // filter
        D3D12_TEXTURE_ADDRESS_MODE_WRAP,   // addressU
        D3D12_TEXTURE_ADDRESS_MODE_WRAP,   // addressV
        D3D12_TEXTURE_ADDRESS_MODE_WRAP);  // addressW

    const CD3DX12_STATIC_SAMPLER_DESC linearClamp(
        3,                                  // shaderRegister
        D3D12_FILTER_MIN_MAG_MIP_LINEAR,    // filter
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP,   // addressU
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP,   // addressV
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP);  // addressW

    const CD3DX12_STATIC_SAMPLER_DESC anisotropicWrap(
        4,                                // shaderRegister
        D3D12_FILTER_ANISOTROPIC,         // filter
        D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
        D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
        D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressW
        0.0f,                             // mipLODBias
        8);                               // maxAnisotropy

    const CD3DX12_STATIC_SAMPLER_DESC anisotropicClamp(
        5,                                 // shaderRegister
        D3D12_FILTER_ANISOTROPIC,          // filter
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressW
        0.0f,                              // mipLODBias
        8);                                // maxAnisotropy

    return {
        pointWrap, pointClamp,
        linearWrap, linearClamp,
        anisotropicWrap, anisotropicClamp
    };
}
}  // namespace my

// @TODO: refactor
#if 1
#define NAME_DX12_OBJECT(OBJ, NAME) \
    do {                            \
        (OBJ)->SetName(NAME);       \
    } while (0)
#define NAME_DX12_OBJECT_INDEXED(OBJ, NAME, IDX)  \
    do {                                          \
        wchar_t buffer[256] = { 0 };              \
        swprintf_s(buffer, L"%s[%d]", NAME, IDX); \
        (OBJ)->SetName(buffer);                   \
    } while (0)
#else
#define NAME_DX12_OBJECT(OBJ, NAME)
#define NAME_DX12_OBJECT_INDEXED(OBJ, NAME, IDX)
#endif