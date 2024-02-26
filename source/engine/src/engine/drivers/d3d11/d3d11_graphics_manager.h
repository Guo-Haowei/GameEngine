#pragma once
#include <d3d11.h>
#include <wrl/client.h>

#include "core/framework/graphics_manager.h"

namespace my {

class D3d11GraphicsManager : public GraphicsManager {
public:
    D3d11GraphicsManager() : GraphicsManager("D3d11GraphicsManager", Backend::D3D11) {}

    bool initialize() final;
    void finalize() final;
    void render() final;

    // @TODO: filter
    std::shared_ptr<Texture> create_texture(const TextureDesc& p_texture_desc, const SamplerDesc& p_sampler_desc) final;

    std::shared_ptr<RenderTarget> create_resource(const RenderTargetDesc& p_desc, const SamplerDesc& p_sampler) final;

    std::shared_ptr<Subpass> create_subpass(const SubpassDesc&) final { return nullptr; }

    uint64_t get_final_image() const final;

    // @TODO: refactor this
    void fill_material_constant_buffer(const MaterialComponent*, MaterialConstantBuffer&) final {}

protected:
    void on_scene_change(const Scene&) final {}
    void on_window_resize(int p_width, int p_height) final;
    void set_pipeline_state_impl(PipelineStateName) final {}

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
};

}  // namespace my
