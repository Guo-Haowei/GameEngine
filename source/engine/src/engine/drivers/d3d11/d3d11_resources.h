#ifdef D3D11_RESOURCES_INCLUDED
#error DO NOT INCLUDE THIS FILE IN HEADER
#endif
#define D3D11_RESOURCES_INCLUDED

#include <d3d11.h>
#include <wrl/client.h>

#include "rendering/render_graph/draw_pass.h"
#include "rendering/texture.h"

namespace my {

struct D3d11Texture : public Texture {
    using Texture::Texture;

    uint64_t get_resident_handle() const { return 0; }
    uint64_t get_handle() const { return (uint64_t)srv.Get(); }

    Microsoft::WRL::ComPtr<ID3D11Resource> texture;

    // @TODO: refactor render target and texture entirely
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv;
    // @TODO: this shouldn't be here
    Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView> uav;
};

struct D3d11DrawPass : public DrawPass {
    std::vector<Microsoft::WRL::ComPtr<ID3D11RenderTargetView>> rtvs;
    Microsoft::WRL::ComPtr<ID3D11DepthStencilView> dsv;
};

struct D3d11UniformBuffer : public UniformBufferBase {
    using UniformBufferBase::UniformBufferBase;

    Microsoft::WRL::ComPtr<ID3D11Buffer> buffer;
    mutable const char* data;
};

}  // namespace my