#if 0
#pragma once
#include <wrl/client.h>

#include "DX12Core.h"
#include "core/framework/GraphicsManager.h"

namespace vct
{

using Microsoft::WRL::ComPtr;

struct PipelineState_DX12
{
    ComPtr<ID3D12PipelineState> pso;
};

class D3D12Context final : public GraphicsManager
{
    static constexpr DXGI_FORMAT DEFAULT_DEPTH_STENCIL_FORMAT = DXGI_FORMAT_D32_FLOAT;

public:
    [[nodiscard]] virtual ErrorCode initialize(const CreateInfo& info) override;
    virtual void finalize() override;

    virtual void Render(const DrawData& draw_data) override;

    virtual void Present() override;
    virtual void Resize(uint32_t new_width, uint32_t new_height) override;

    virtual bool CreatePipelineState(const PipelineStateDesc& desc, PipelineState& pso) override;

    virtual void SetPipelineState(PipelineState& pipeline_state, CommandList cmd) override;

    ID3D12CommandQueue* CreateCommandQueue(D3D12_COMMAND_LIST_TYPE type);

    ID3D12Device4* const GetDevice() const { return m_device.Get(); }

    virtual bool CreateTexture(const Image& image) override;

protected:
    virtual void BeginScene(const Scene& scene) override;
    virtual void EndScene() override;

    virtual void BeginFrame() override;
    virtual void EndFrame() override;

private:
    static constexpr DXGI_FORMAT SURFACE_FORMAT = DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM;

    [[nodiscard]] auto create_device() -> std::expected<void, Error<HRESULT>>;
    [[nodiscard]] auto enable_debug_layer() -> std::expected<void, Error<HRESULT>>;
    [[nodiscard]] auto create_descriptor_heaps() -> std::expected<void, Error<HRESULT>>;
    [[nodiscard]] auto create_swap_chain(uint32_t width, uint32_t height) -> std::expected<void, Error<HRESULT>>;
    [[nodiscard]] auto create_render_target(uint32_t width, uint32_t height) -> std::expected<void, Error<HRESULT>>;

    void CleanupRenderTarget();
    bool LoadAssets();

    GraphicsContext m_graphicsContext;
    CopyContext m_copyContext;

    DescriptorHeapGPU m_rtvDescHeap;
    DescriptorHeapGPU m_dsvDescHeap;
    DescriptorHeapGPU m_srvDescHeap;
    FrameContext* m_currentFrameContext = nullptr;

    ComPtr<ID3D12Device4> m_device;
    ComPtr<ID3D12Debug> m_debugController;
    ComPtr<IDXGIFactory4> m_factory;
    ComPtr<IDXGISwapChain3> m_swap_chain;

    HANDLE m_swapChainWaitObject = INVALID_HANDLE_VALUE;

    // Render Target

    ID3D12Resource* m_renderTargets[NUM_BACK_BUFFERS] = { nullptr };
    D3D12_CPU_DESCRIPTOR_HANDLE m_renderTargetDescriptor[NUM_BACK_BUFFERS] = {};
    ID3D12Resource* m_depthStencilBuffer = nullptr;
    D3D12_CPU_DESCRIPTOR_HANDLE m_depthStencilDescriptor = {};

    uint32_t m_backbufferIndex = 0;

    ComPtr<ID3D12RootSignature> m_rootSignature;

    ComPtr<ID3D12Resource> m_debugVertexData;
    ComPtr<ID3D12Resource> m_debugIndexData;

    std::vector<ComPtr<ID3D12Resource>> m_textures;
    std::atomic_int m_textureCounter = 1;  // slot 0 is for imgui
};
}  // namespace vct
#endif
