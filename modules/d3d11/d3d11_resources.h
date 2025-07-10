#ifdef D3D11_RESOURCES_INCLUDED
#error DO NOT INCLUDE THIS FILE IN HEADER
#endif
#define D3D11_RESOURCES_INCLUDED

#include <d3d11.h>
#include <wrl/client.h>

#include "engine/render_graph/framebuffer.h"
#include "engine/renderer/gpu_resource.h"

namespace my {

struct D3d11GpuTexture : public GpuTexture {
    using GpuTexture::GpuTexture;

    uint64_t GetResidentHandle() const final { return 0; }
    uint64_t GetHandle() const final { return (uint64_t)srv.Get(); }
    uint64_t GetUavHandle() const final { return (uint64_t)uav.Get(); }

    Microsoft::WRL::ComPtr<ID3D11Resource> texture;

    // @TODO: refactor render target and texture entirely
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv;
    // @TODO: this shouldn't be here
    Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView> uav;
};

struct D3d11Framebuffer : public Framebuffer {
    std::vector<Microsoft::WRL::ComPtr<ID3D11RenderTargetView>> rtvs;
    std::vector<Microsoft::WRL::ComPtr<ID3D11DepthStencilView>> dsvs;
};

struct D3d11UniformBuffer : public GpuConstantBuffer {
    using GpuConstantBuffer::GpuConstantBuffer;

    Microsoft::WRL::ComPtr<ID3D11Buffer> internalBuffer;
    mutable const char* data;
};

struct D3d11StructuredBuffer : GpuStructuredBuffer {
    using GpuStructuredBuffer::GpuStructuredBuffer;

    Microsoft::WRL::ComPtr<ID3D11Buffer> buffer;
    Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView> uav;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv;
};

}  // namespace my