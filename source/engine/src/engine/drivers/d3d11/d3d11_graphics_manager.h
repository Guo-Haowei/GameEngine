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

    bool initialize_internal() final;
    void finalize() final;
    void render() final;

    void set_stencil_ref(uint32_t p_ref) final;

    void set_render_target(const Subpass* p_subpass, int p_index, int p_mip_level) final;
    void clear(const Subpass* p_subpass, uint32_t p_flags, float* p_clear_color) final;
    void set_viewport(const Viewport& p_viewport) final;

    void set_mesh(const MeshBuffers* p_mesh) final;
    void draw_elements(uint32_t p_count, uint32_t p_offset) final;

    std::shared_ptr<UniformBufferBase> uniform_create(int p_slot, size_t p_capacity) final;
    void uniform_update(const UniformBufferBase* p_buffer, const void* p_data, size_t p_size) final;
    void uniform_bind_range(const UniformBufferBase* p_buffer, uint32_t p_size, uint32_t p_offset) final;

    std::shared_ptr<Texture> create_texture(const TextureDesc& p_texture_desc, const SamplerDesc& p_sampler_desc) final;
    std::shared_ptr<Subpass> create_subpass(const SubpassDesc&) final;

    Microsoft::WRL::ComPtr<ID3D11Device>& get_d3d_device() { return m_device; }

protected:
    void on_scene_change(const Scene& p_scene) final;
    void on_window_resize(int p_width, int p_height) final;
    void set_pipeline_state_impl(PipelineStateName p_name) final;

    bool create_device();
    bool create_swap_chain();
    bool create_render_target();

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
