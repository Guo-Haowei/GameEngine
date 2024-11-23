#pragma once
#include <d3d11.h>
#include <wrl/client.h>

#include "core/base/rid_owner.h"
#include "core/framework/graphics_manager.h"

namespace my {

struct D3d11MeshBuffers : public MeshBuffers {
    Microsoft::WRL::ComPtr<ID3D11Buffer> vertex_buffer[6]{};
    Microsoft::WRL::ComPtr<ID3D11Buffer> index_buffer;
};

class D3d11GraphicsManager : public GraphicsManager {
public:
    D3d11GraphicsManager();

    void Finalize() final;

    void SetStencilRef(uint32_t p_ref) final;
    void SetBlendState(const BlendDesc& p_desc, const float* p_factor, uint32_t p_mask) final;

    void SetRenderTarget(const DrawPass* p_draw_pass, int p_index, int p_mip_level) final;
    void UnsetRenderTarget() final;

    void Clear(const DrawPass* p_draw_pass, ClearFlags p_flags, const float* p_clear_color, int p_index) final;
    void SetViewport(const Viewport& p_viewport) final;

    const MeshBuffers* CreateMesh(const MeshComponent& p_mesh) final;
    void SetMesh(const MeshBuffers* p_mesh) final;
    void DrawElements(uint32_t p_count, uint32_t p_offset) final;
    void DrawElementsInstanced(uint32_t p_instance_count, uint32_t p_count, uint32_t p_offset) final;

    void Dispatch(uint32_t p_num_groups_x, uint32_t p_num_groups_y, uint32_t p_num_groups_z) final;
    void BindUnorderedAccessView(uint32_t p_slot, GpuTexture* p_texture) final;
    void UnbindUnorderedAccessView(uint32_t p_slot) final;

    auto CreateConstantBuffer(const GpuBufferDesc& p_desc) -> Result<std::shared_ptr<GpuConstantBuffer>> final;
    auto CreateStructuredBuffer(const GpuBufferDesc& p_desc) -> Result<std::shared_ptr<GpuStructuredBuffer>> final;

    void BindStructuredBuffer(int p_slot, const GpuStructuredBuffer* p_buffer) final;
    void UnbindStructuredBuffer(int p_slot) final;
    void BindStructuredBufferSRV(int p_slot, const GpuStructuredBuffer* p_buffer) final;
    void UnbindStructuredBufferSRV(int p_slot) final;

    void UpdateConstantBuffer(const GpuConstantBuffer* p_buffer, const void* p_data, size_t p_size) final;
    void BindConstantBufferRange(const GpuConstantBuffer* p_buffer, uint32_t p_size, uint32_t p_offset) final;

    void BindTexture(Dimension p_dimension, uint64_t p_handle, int p_slot) final;
    void UnbindTexture(Dimension p_dimension, int p_slot) final;

    void GenerateMipmap(const GpuTexture* p_texture) final;

    std::shared_ptr<DrawPass> CreateDrawPass(const DrawPassDesc&) final;

    // For fast and dirty access to device and device context, try not to use it
    Microsoft::WRL::ComPtr<ID3D11Device>& GetD3dDevice() { return m_device; }
    Microsoft::WRL::ComPtr<ID3D11DeviceContext>& GetD3dContext() { return m_deviceContext; }

protected:
    auto InitializeImpl() -> Result<void> final;
    std::shared_ptr<GpuTexture> CreateTextureImpl(const GpuTextureDesc& p_texture_desc, const SamplerDesc& p_sampler_desc) final;

    void Render() final;
    void Present() final;

    void OnSceneChange(const Scene& p_scene) final;
    void OnWindowResize(int p_width, int p_height) final;
    void SetPipelineStateImpl(PipelineStateName p_name) final;

    auto CreateDevice() -> Result<void>;
    auto CreateSwapChain() -> Result<void>;
    auto CreateRenderTarget() -> Result<void>;
    auto CreateSampler(uint32_t p_slot, D3D11_SAMPLER_DESC p_desc) -> Result<void>;
    auto InitSamplers() -> Result<void>;

    Microsoft::WRL::ComPtr<ID3D11Device> m_device;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> m_deviceContext;
    Microsoft::WRL::ComPtr<IDXGISwapChain> m_swapChain;
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView> m_windowRtv;
    Microsoft::WRL::ComPtr<IDXGIDevice> m_dxgiDevice;
    Microsoft::WRL::ComPtr<IDXGIAdapter> m_dxgiAdapter;
    Microsoft::WRL::ComPtr<IDXGIFactory> m_dxgiFactory;
    std::vector<Microsoft::WRL::ComPtr<ID3D11SamplerState>> m_samplers;

    RIDAllocator<D3d11MeshBuffers> m_meshes;

    // @TODO: cache
    struct {
        ID3D11RasterizerState* rasterizer = nullptr;
        ID3D11DepthStencilState* depthStencil = nullptr;
        ID3D11BlendState* blendState = nullptr;
    } m_stateCache;
};

}  // namespace my
