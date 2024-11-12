#pragma once
#include "core/framework/graphics_manager.h"
#include "drivers/d3d12/d3d12_core.h"
// @TODO: remove
#include "drivers/empty/empty_graphics_manager.h"

namespace my {

class D3d12GraphicsManager : public EmptyGraphicsManager {
public:
    D3d12GraphicsManager();

    bool InitializeImpl() final;
    void Finalize() final;
    void Render() final;

    ID3D12CommandQueue* CreateCommandQueue(D3D12_COMMAND_LIST_TYPE p_type);

    ID3D12Device4* const GetDevice() const { return m_device.Get(); }

protected:
    void OnWindowResize(int p_width, int p_height) final;

private:
    bool CreateDevice();
    bool EnableDebugLayer();
    bool CreateDescriptorHeaps();
    bool CreateSwapChain(uint32_t p_width, uint32_t p_height);
    bool CreateRenderTarget(uint32_t p_width, uint32_t p_height);
    void CleanupRenderTarget();
    bool LoadAssets();

    void BeginFrame();
    void EndFrame();

    GraphicsContext m_graphicsContext;
    CopyContext m_copyContext;

    DescriptorHeapGPU m_rtvDescHeap;
    DescriptorHeapGPU m_dsvDescHeap;
    DescriptorHeapGPU m_srvDescHeap;
    FrameContext* m_currentFrameContext = nullptr;

    Microsoft::WRL::ComPtr<ID3D12Device4> m_device;
    Microsoft::WRL::ComPtr<ID3D12Debug> m_debugController;
    Microsoft::WRL::ComPtr<IDXGIFactory4> m_factory;
    Microsoft::WRL::ComPtr<IDXGISwapChain3> m_swap_chain;

    HANDLE m_swapChainWaitObject = INVALID_HANDLE_VALUE;

    // Render Target

    ID3D12Resource* m_renderTargets[NUM_BACK_BUFFERS] = { nullptr };
    D3D12_CPU_DESCRIPTOR_HANDLE m_renderTargetDescriptor[NUM_BACK_BUFFERS] = {};
    ID3D12Resource* m_depthStencilBuffer = nullptr;
    D3D12_CPU_DESCRIPTOR_HANDLE m_depthStencilDescriptor = {};

    uint32_t m_backbufferIndex = 0;

    Microsoft::WRL::ComPtr<ID3D12RootSignature> m_rootSignature;

    Microsoft::WRL::ComPtr<ID3D12Resource> m_debugVertexData;
    Microsoft::WRL::ComPtr<ID3D12Resource> m_debugIndexData;

    std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> m_textures;
    std::atomic_int m_textureCounter = 1;  // slot 0 is for imgui
};

}  // namespace my
