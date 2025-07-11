#pragma once
#include <d3d11_1.h>
#include <wrl/client.h>

#include "engine/core/base/rid_owner.h"
#include "engine/renderer/graphics_manager.h"

namespace my {

struct D3d11Buffer : GpuBuffer {
    using GpuBuffer::GpuBuffer;

    Microsoft::WRL::ComPtr<ID3D11Buffer> buffer;

    uint64_t GetHandle() const final {
        return (size_t)buffer.Get();
    }
};

struct D3d11MeshBuffers : GpuMesh {
    using GpuMesh::GpuMesh;
};

class D3d11GraphicsManager : public GraphicsManager {
public:
    D3d11GraphicsManager();

    void FinalizeImpl() final;

    void SetStencilRef(uint32_t p_ref) final;
    void SetBlendState(const BlendDesc& p_desc, const float* p_factor, uint32_t p_mask) final;

    void SetRenderTarget(const Framebuffer* p_framebuffer, int p_index, int p_mip_level) final;
    void UnsetRenderTarget() final;

    void Clear(const Framebuffer* p_framebuffer,
               ClearFlags p_flags,
               const float* p_clear_color,
               float p_clear_depth,
               uint8_t p_clear_stencil,
               int p_index) final;

    void SetViewport(const Viewport& p_viewport) final;

    auto CreateBuffer(const GpuBufferDesc& p_desc) -> Result<std::shared_ptr<GpuBuffer>> final;

    auto CreateMeshImpl(const GpuMeshDesc& p_desc,
                        uint32_t p_count,
                        const GpuBufferDesc* p_vb_descs,
                        const GpuBufferDesc* p_ib_desc) -> Result<std::shared_ptr<GpuMesh>> final;

    void SetMesh(const GpuMesh* p_mesh) final;
    void UpdateBuffer(const GpuBufferDesc& p_desc, GpuBuffer* p_buffer) final;

    void DrawElements(uint32_t p_count, uint32_t p_offset) final;
    void DrawElementsInstanced(uint32_t p_instance_count, uint32_t p_count, uint32_t p_offset) final;
    void DrawArrays(uint32_t p_count, uint32_t p_offset) final;
    void DrawArraysInstanced(uint32_t p_instance_count, uint32_t p_count, uint32_t p_offset) final;

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

    void BeginEvent(std::string_view p_event) final;
    void EndEvent() final;

    std::shared_ptr<Framebuffer> CreateFramebuffer(const FramebufferDesc&) final;

    // For fast and dirty access to device and device context, try not to use it
    Microsoft::WRL::ComPtr<ID3D11Device>& GetD3dDevice() { return m_device; }
    Microsoft::WRL::ComPtr<ID3D11DeviceContext>& GetD3dContext() { return m_deviceContext; }

protected:
    virtual auto InitializeInternal() -> Result<void> final;
    virtual std::shared_ptr<GpuTexture> CreateTextureImpl(const GpuTextureDesc& p_texture_desc, const SamplerDesc& p_sampler_desc) final;

    virtual void Render() final;
    virtual void Present() final;

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
    Microsoft::WRL::ComPtr<ID3DUserDefinedAnnotation> m_annotation;
    std::vector<Microsoft::WRL::ComPtr<ID3D11SamplerState>> m_samplers;

    RIDAllocator<D3d11MeshBuffers> m_meshes;

    // @TODO: cache
    struct {
        ID3D11RasterizerState* rasterizer = nullptr;
        ID3D11DepthStencilState* depthStencil = nullptr;
        uint32_t stencilRef = 0xFFFFFFFF;
        ID3D11BlendState* blendState = nullptr;
    } m_stateCache;
};

}  // namespace my
