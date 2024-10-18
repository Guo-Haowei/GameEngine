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

    bool initializeImpl() final;
    void finalize() final;
    void render() final;

    void setStencilRef(uint32_t p_ref) final;

    void setRenderTarget(const Subpass* p_subpass, int p_index, int p_mip_level) final;
    void clear(const Subpass* p_subpass, uint32_t p_flags, float* p_clear_color) final;
    void setViewport(const Viewport& p_viewport) final;

    const MeshBuffers* createMesh(const MeshComponent& p_mesh) final;
    void setMesh(const MeshBuffers* p_mesh) final;
    void drawElements(uint32_t p_count, uint32_t p_offset) final;

    std::shared_ptr<UniformBufferBase> createUniform(int p_slot, size_t p_capacity) final;
    void updateUniform(const UniformBufferBase* p_buffer, const void* p_data, size_t p_size) final;
    void bindUniformRange(const UniformBufferBase* p_buffer, uint32_t p_size, uint32_t p_offset) final;

    void bindTexture(Dimension p_dimension, uint64_t p_handle, int p_slot) final;
    void unbindTexture(Dimension p_dimension, int p_slot) final;

    std::shared_ptr<Texture> createTexture(const TextureDesc& p_texture_desc, const SamplerDesc& p_sampler_desc) final;
    std::shared_ptr<Subpass> createSubpass(const SubpassDesc&) final;

    Microsoft::WRL::ComPtr<ID3D11Device>& getD3dDevice() { return m_device; }

protected:
    void onSceneChange(const Scene& p_scene) final;
    void onWindowResize(int p_width, int p_height) final;
    void setPipelineStateImpl(PipelineStateName p_name) final;

    bool createDevice();
    bool createSwapChain();
    bool createRenderTarget();

    Microsoft::WRL::ComPtr<ID3D11Device> m_device;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> m_ctx;
    Microsoft::WRL::ComPtr<IDXGISwapChain> m_swap_chain;
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView> m_window_rtv;
    Microsoft::WRL::ComPtr<IDXGIDevice> m_dxgi_device;
    Microsoft::WRL::ComPtr<IDXGIAdapter> m_dxgi_adapter;
    Microsoft::WRL::ComPtr<IDXGIFactory> m_dxgi_factory;

    RIDAllocator<D3d11MeshBuffers> m_meshes;

    struct {
        ID3D11RasterizerState* rasterizer = nullptr;
        ID3D11DepthStencilState* depth_stencil = nullptr;
    } m_state_cache;
};

}  // namespace my
