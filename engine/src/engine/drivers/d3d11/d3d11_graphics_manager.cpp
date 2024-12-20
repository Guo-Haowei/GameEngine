#include "d3d11_graphics_manager.h"

#include <imgui/backends/imgui_impl_dx11.h>

#include "engine/core/framework/application.h"
#include "engine/core/framework/imgui_manager.h"
#include "engine/drivers/d3d11/d3d11_helpers.h"
#include "engine/drivers/d3d11/d3d11_pipeline_state_manager.h"
#include "engine/drivers/d3d11/d3d11_resources.h"
#include "engine/drivers/d3d_common/d3d_common.h"
#include "engine/drivers/windows/win32_display_manager.h"
#include "engine/renderer/gpu_resource.h"
#include "engine/renderer/graphics_private.h"
#include "engine/renderer/render_graph/render_graph_defines.h"
#include "engine/scene/scene.h"

#define INCLUDE_AS_D3D11
#include "engine/drivers/d3d_common/d3d_convert.h"

namespace my {

using Microsoft::WRL::ComPtr;

D3d11GraphicsManager::D3d11GraphicsManager() : GraphicsManager("D3d11GraphicsManager", Backend::D3D11, 1) {
    m_pipelineStateManager = std::make_shared<D3d11PipelineStateManager>();
}

auto D3d11GraphicsManager::InitializeInternal() -> Result<void> {
    if (auto res = CreateDevice(); !res) {
        return HBN_ERROR(res.error());
    }
    if (auto res = CreateSwapChain(); !res) {
        return HBN_ERROR(res.error());
    }
    if (auto res = CreateRenderTarget(); !res) {
        return HBN_ERROR(res.error());
    }
    if (auto res = InitSamplers(); !res) {
        return HBN_ERROR(res.error());
    }

    m_meshes.set_description("GPU-Mesh-Allocator");

    auto imgui = m_app->GetImguiManager();
    if (imgui) {
        imgui->SetRenderCallbacks(
            [this]() {
                ImGui_ImplDX11_Init(m_device.Get(), m_deviceContext.Get());
                ImGui_ImplDX11_NewFrame();
            },
            []() {
                ImGui_ImplDX11_Shutdown();
            });
    }

    return Result<void>();
}

void D3d11GraphicsManager::FinalizeImpl() {
}

void D3d11GraphicsManager::Render() {
    m_deviceContext->OMSetRenderTargets(1, m_windowRtv.GetAddressOf(), nullptr);
    // const float clear_color[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    // m_deviceContext->ClearRenderTargetView(m_windowRtv.Get(), clear_color);

    if (m_app->GetSpecification().enableImgui) {
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
    }
}

void D3d11GraphicsManager::Present() {
    if (m_app->GetSpecification().enableImgui) {
        ImGuiIO& io = ImGui::GetIO();
        // Update and Render additional Platform Windows
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
        }
    }

    m_swapChain->Present(1, 0);  // Present with vsync
}

void D3d11GraphicsManager::SetStencilRef(uint32_t p_ref) {
    if (m_stateCache.depthStencil) {
        m_deviceContext->OMSetDepthStencilState(m_stateCache.depthStencil, p_ref);
    }
}

void D3d11GraphicsManager::SetBlendState(const BlendDesc& p_desc, const float* p_factor, uint32_t p_mask) {
    unused(p_desc);
    unused(p_factor);
    unused(p_mask);
}

void D3d11GraphicsManager::Dispatch(uint32_t p_num_groups_x, uint32_t p_num_groups_y, uint32_t p_num_groups_z) {
    m_deviceContext->Dispatch(p_num_groups_x, p_num_groups_y, p_num_groups_z);
}

void D3d11GraphicsManager::BindUnorderedAccessView(uint32_t p_slot, GpuTexture* p_texture) {
    DEV_ASSERT(p_texture);

    ID3D11UnorderedAccessView* ptr = reinterpret_cast<ID3D11UnorderedAccessView*>(p_texture->GetUavHandle());
    m_deviceContext->CSSetUnorderedAccessViews(p_slot, 1, &ptr, nullptr);
}

void D3d11GraphicsManager::UnbindUnorderedAccessView(uint32_t p_slot) {
    ID3D11UnorderedAccessView* uav = nullptr;
    m_deviceContext->CSSetUnorderedAccessViews(p_slot, 1, &uav, nullptr);
}

void D3d11GraphicsManager::OnWindowResize(int p_width, int p_height) {
    if (m_device) {
        m_windowRtv.Reset();
        m_swapChain->ResizeBuffers(0, p_width, p_height, DXGI_FORMAT_UNKNOWN, 0);
        CreateRenderTarget();
    }
}

auto D3d11GraphicsManager::CreateDevice() -> Result<void> {
    D3D_FEATURE_LEVEL feature_level = D3D_FEATURE_LEVEL_11_1;
    UINT create_device_flags = m_enableValidationLayer ? D3D11_CREATE_DEVICE_DEBUG : 0;

    HRESULT hr = D3D11CreateDevice(
        nullptr,
        D3D_DRIVER_TYPE_HARDWARE,
        0,
        create_device_flags,
        &feature_level,
        1,
        D3D11_SDK_VERSION,
        m_device.GetAddressOf(),
        nullptr,
        m_deviceContext.GetAddressOf());

    D3D_FAIL(hr, "Failed to create d3d11 device");

    D3D_FAIL(m_device->QueryInterface(__uuidof(IDXGIDevice), (void**)m_dxgiDevice.GetAddressOf()),
             "Failed to query IDXGIDevice");

    D3D_FAIL(m_dxgiDevice->GetParent(__uuidof(IDXGIAdapter), (void**)m_dxgiAdapter.GetAddressOf()),
             "Failed to query IDXGIAdapter");

    D3D_FAIL(m_dxgiAdapter->GetParent(__uuidof(IDXGIFactory), (void**)m_dxgiFactory.GetAddressOf()),
             "Failed to query IDXGIFactory");

    return Result<void>();
}

auto D3d11GraphicsManager::CreateSwapChain() -> Result<void> {
    auto display_manager = dynamic_cast<Win32DisplayManager*>(DisplayManager::GetSingletonPtr());
    DEV_ASSERT(display_manager);

    DXGI_MODE_DESC buffer_desc{};
    buffer_desc.Width = 0;
    buffer_desc.Height = 0;
    buffer_desc.RefreshRate = { 60, 1 };
    buffer_desc.Format = d3d::Convert(DEFAULT_SURFACE_FORMAT);
    buffer_desc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;

    DXGI_SWAP_CHAIN_DESC swap_chain_desc{};
    swap_chain_desc.BufferDesc = buffer_desc;
    swap_chain_desc.SampleDesc = { 1, 0 };
    swap_chain_desc.BufferCount = 2;
    swap_chain_desc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    swap_chain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swap_chain_desc.OutputWindow = display_manager->GetHwnd();
    swap_chain_desc.Windowed = true;
    swap_chain_desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

    D3D_FAIL(m_dxgiFactory->CreateSwapChain(m_device.Get(), &swap_chain_desc, m_swapChain.GetAddressOf()),
             "Failed to create SwapChain");

    return Result<void>();
}

auto D3d11GraphicsManager::CreateRenderTarget() -> Result<void> {
    ComPtr<ID3D11Texture2D> back_buffer;
    D3D_FAIL(m_swapChain->GetBuffer(0, IID_PPV_ARGS(back_buffer.GetAddressOf())),
             "Failed to get SwapChain buffer");
    D3D_FAIL(m_device->CreateRenderTargetView(back_buffer.Get(), nullptr, m_windowRtv.GetAddressOf()),
             "Failed to create RenderTarget View");
    return Result<void>();
}

auto D3d11GraphicsManager::CreateSampler(uint32_t p_slot, D3D11_SAMPLER_DESC p_desc) -> Result<void> {
    ComPtr<ID3D11SamplerState> sampler_state;
    D3D_FAIL(m_device->CreateSamplerState(&p_desc, sampler_state.GetAddressOf()),
             "Failed to create sampler");

    m_deviceContext->CSSetSamplers(p_slot, 1, sampler_state.GetAddressOf());
    m_deviceContext->PSSetSamplers(p_slot, 1, sampler_state.GetAddressOf());
    m_samplers.emplace_back(sampler_state);
    return Result<void>();
}

auto D3d11GraphicsManager::InitSamplers() -> Result<void> {
    auto FillSamplerDesc = [](const SamplerDesc& p_desc) {
        D3D11_SAMPLER_DESC sampler_desc;

        sampler_desc.Filter = d3d::Convert(p_desc.minFilter, p_desc.magFilter);
        sampler_desc.AddressU = d3d::Convert(p_desc.addressU);
        sampler_desc.AddressV = d3d::Convert(p_desc.addressV);
        sampler_desc.AddressW = d3d::Convert(p_desc.addressW);
        sampler_desc.MipLODBias = p_desc.mipLodBias;
        sampler_desc.MaxAnisotropy = p_desc.maxAnisotropy;
        sampler_desc.ComparisonFunc = d3d::Convert(p_desc.comparisonFunc);
        sampler_desc.BorderColor[0] = p_desc.border[0];
        sampler_desc.BorderColor[1] = p_desc.border[1];
        sampler_desc.BorderColor[2] = p_desc.border[2];
        sampler_desc.BorderColor[3] = p_desc.border[3];
        sampler_desc.MinLOD = p_desc.minLod;
        sampler_desc.MaxLOD = p_desc.maxLod;
        return sampler_desc;
    };

#define SAMPLER_STATE(REG, NAME, DESC) \
    if (!CreateSampler(REG, FillSamplerDesc(DESC))) { return HBN_ERROR(ErrorCode::ERR_CANT_CREATE, "Failed to create Sampler {}", #NAME); }
#include "sampler.hlsl.h"
#undef SAMPLER_STATE
    return Result<void>();
}

auto D3d11GraphicsManager::CreateConstantBuffer(const GpuBufferDesc& p_desc) -> Result<std::shared_ptr<GpuConstantBuffer>> {
    D3D11_BUFFER_DESC buffer_desc{};
    buffer_desc.ByteWidth = p_desc.elementCount * p_desc.elementSize;
    buffer_desc.Usage = D3D11_USAGE_DYNAMIC;
    buffer_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    buffer_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    buffer_desc.MiscFlags = 0;
    buffer_desc.StructureByteStride = 0;

    ComPtr<ID3D11Buffer> d3d_buffer;
    D3D_FAIL(m_device->CreateBuffer(&buffer_desc, nullptr, d3d_buffer.GetAddressOf()),
             "failed to create ConstantBuffer");

    auto uniform_buffer = std::make_shared<D3d11UniformBuffer>(p_desc);
    uniform_buffer->internalBuffer = d3d_buffer;

    m_deviceContext->VSSetConstantBuffers(p_desc.slot, 1, uniform_buffer->internalBuffer.GetAddressOf());
    m_deviceContext->PSSetConstantBuffers(p_desc.slot, 1, uniform_buffer->internalBuffer.GetAddressOf());
    m_deviceContext->CSSetConstantBuffers(p_desc.slot, 1, uniform_buffer->internalBuffer.GetAddressOf());
    return uniform_buffer;
}

auto D3d11GraphicsManager::CreateStructuredBuffer(const GpuBufferDesc& p_desc) -> Result<std::shared_ptr<GpuStructuredBuffer>> {
    ComPtr<ID3D11Buffer> buffer;
    ComPtr<ID3D11UnorderedAccessView> uav;
    ComPtr<ID3D11ShaderResourceView> srv;

    D3D11_BUFFER_DESC buffer_desc = {};
    buffer_desc.Usage = D3D11_USAGE_DEFAULT;
    buffer_desc.ByteWidth = p_desc.elementCount * p_desc.elementSize;
    buffer_desc.BindFlags = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;
    buffer_desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
    buffer_desc.CPUAccessFlags = 0;
    buffer_desc.StructureByteStride = p_desc.elementSize;

    D3D11_SUBRESOURCE_DATA* init_data_ref = nullptr;
    D3D11_SUBRESOURCE_DATA init_data{};
    if (p_desc.initialData) {
        init_data.pSysMem = p_desc.initialData;
        init_data.SysMemPitch = 0;
        init_data.SysMemSlicePitch = 0;
        init_data_ref = &init_data;
    }

    D3D_FAIL(m_device->CreateBuffer(&buffer_desc, init_data_ref, buffer.GetAddressOf()),
             "Failed to create buffer (StructuredBuffer)");

    D3D11_UNORDERED_ACCESS_VIEW_DESC uav_desc = {};
    uav_desc.Format = DXGI_FORMAT_UNKNOWN;
    uav_desc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
    uav_desc.Buffer.FirstElement = 0;
    uav_desc.Buffer.NumElements = p_desc.elementCount;
    D3D_FAIL(m_device->CreateUnorderedAccessView(buffer.Get(), &uav_desc, uav.GetAddressOf()),
             "Failed to create UAV (StructuredBuffer)");

    D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
    srv_desc.Format = DXGI_FORMAT_UNKNOWN;
    srv_desc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
    srv_desc.Buffer.FirstElement = 0;
    srv_desc.Buffer.NumElements = p_desc.elementCount;
    D3D_FAIL(m_device->CreateShaderResourceView(buffer.Get(), &srv_desc, srv.GetAddressOf()),
             "Failed to create SRV (StructuredBuffer)");

    auto structured_buffer = std::make_shared<D3d11StructuredBuffer>(p_desc);
    structured_buffer->buffer = buffer;
    structured_buffer->uav = uav;
    structured_buffer->srv = srv;

    return structured_buffer;
}

void D3d11GraphicsManager::UpdateConstantBuffer(const GpuConstantBuffer* p_buffer, const void* p_data, size_t p_size) {
    auto buffer = reinterpret_cast<const D3d11UniformBuffer*>(p_buffer);
    DEV_ASSERT(p_size <= buffer->capacity);
    buffer->data = (const char*)p_data;
}

void D3d11GraphicsManager::BindConstantBufferRange(const GpuConstantBuffer* p_buffer, uint32_t p_size, uint32_t p_offset) {
    auto buffer = reinterpret_cast<const D3d11UniformBuffer*>(p_buffer);
    DEV_ASSERT(p_size + p_offset <= buffer->capacity);
    D3D11_MAPPED_SUBRESOURCE mapped;
    ZeroMemory(&mapped, sizeof(D3D11_MAPPED_SUBRESOURCE));
    m_deviceContext->Map(buffer->internalBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
    memcpy(mapped.pData, buffer->data + p_offset, p_size);
    m_deviceContext->Unmap(buffer->internalBuffer.Get(), 0);
}

void D3d11GraphicsManager::BindTexture(Dimension p_dimension, uint64_t p_handle, int p_slot) {
    unused(p_dimension);

    if (p_handle) {
        ID3D11ShaderResourceView* srv = (ID3D11ShaderResourceView*)(p_handle);
        m_deviceContext->PSSetShaderResources(p_slot, 1, &srv);
        m_deviceContext->CSSetShaderResources(p_slot, 1, &srv);
    }
}

void D3d11GraphicsManager::UnbindTexture(Dimension p_dimension, int p_slot) {
    unused(p_dimension);

    ID3D11ShaderResourceView* srv = nullptr;
    m_deviceContext->PSSetShaderResources(p_slot, 1, &srv);
    m_deviceContext->CSSetShaderResources(p_slot, 1, &srv);
}

void D3d11GraphicsManager::GenerateMipmap(const GpuTexture* p_texture) {
    auto texture = reinterpret_cast<const D3d11GpuTexture*>(p_texture);
    m_deviceContext->GenerateMips(texture->srv.Get());
}

void D3d11GraphicsManager::BindStructuredBuffer(int p_slot, const GpuStructuredBuffer* p_buffer) {
    auto structured_buffer = reinterpret_cast<const D3d11StructuredBuffer*>(p_buffer);
    m_deviceContext->CSSetUnorderedAccessViews(p_slot, 1, structured_buffer->uav.GetAddressOf(), nullptr);
}

void D3d11GraphicsManager::UnbindStructuredBuffer(int p_slot) {
    ID3D11UnorderedAccessView* uav = nullptr;
    m_deviceContext->CSSetUnorderedAccessViews(p_slot, 1, &uav, nullptr);
}

void D3d11GraphicsManager::BindStructuredBufferSRV(int p_slot, const GpuStructuredBuffer* p_buffer) {
    auto structured_buffer = reinterpret_cast<const D3d11StructuredBuffer*>(p_buffer);

    if (structured_buffer->srv != nullptr) {
        m_deviceContext->VSSetShaderResources(p_slot, 1, structured_buffer->srv.GetAddressOf());
    }
}

void D3d11GraphicsManager::UnbindStructuredBufferSRV(int p_slot) {
    ID3D11ShaderResourceView* srv = nullptr;
    m_deviceContext->VSSetShaderResources(p_slot, 1, &srv);
}

std::shared_ptr<GpuTexture> D3d11GraphicsManager::CreateTextureImpl(const GpuTextureDesc& p_texture_desc, const SamplerDesc& p_sampler_desc) {
    unused(p_sampler_desc);

    ComPtr<ID3D11ShaderResourceView> srv;
    ComPtr<ID3D11UnorderedAccessView> uav;

    PixelFormat format = p_texture_desc.format;
    DXGI_FORMAT texture_format = d3d::Convert(format);
    DXGI_FORMAT srv_format = d3d::Convert(format);
    // @TODO: refactor this
    bool gen_mip_map = p_texture_desc.bindFlags & BIND_SHADER_RESOURCE;
    if (p_texture_desc.dimension == Dimension::TEXTURE_CUBE) {
        gen_mip_map = false;
    }
    if (p_texture_desc.dimension == Dimension::TEXTURE_CUBE_ARRAY) {
        gen_mip_map = false;
    }

    // @TODO: refactor this part
    if (format == PixelFormat::D32_FLOAT) {
        texture_format = DXGI_FORMAT_R32_TYPELESS;
        srv_format = DXGI_FORMAT_R32_FLOAT;
        gen_mip_map = false;
    }
    if (format == PixelFormat::D24_UNORM_S8_UINT) {
        texture_format = DXGI_FORMAT_R24G8_TYPELESS;
    }
    if (format == PixelFormat::R24G8_TYPELESS) {
        srv_format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
        gen_mip_map = false;
    }

#if USING(DEBUG_BUILD)
    const char* debug_name = RenderTargetResourceNameToString(p_texture_desc.name);
#endif
    if (p_texture_desc.dimension == Dimension::TEXTURE_3D) {
        D3D11_TEXTURE3D_DESC texture_desc{};
        texture_desc.Width = p_texture_desc.width;
        texture_desc.Height = p_texture_desc.height;
        texture_desc.Depth = p_texture_desc.depth;
        texture_desc.MipLevels = gen_mip_map ? 0 : p_texture_desc.mipLevels;
        // texture_desc.MipLevels = 0;
        texture_desc.Format = texture_format;
        texture_desc.Usage = D3D11_USAGE_DEFAULT;
        texture_desc.BindFlags = d3d::ConvertBindFlags(p_texture_desc.bindFlags);
        texture_desc.CPUAccessFlags = 0;
        texture_desc.MiscFlags = d3d::ConvertResourceMiscFlags(p_texture_desc.miscFlags);
        ComPtr<ID3D11Texture3D> texture;
        D3D_FAIL_V(m_device->CreateTexture3D(&texture_desc, nullptr, texture.GetAddressOf()), nullptr);
        SetDebugName(texture.Get(), debug_name);

        auto gpu_texture = std::make_shared<D3d11GpuTexture>(p_texture_desc);
        if (p_texture_desc.bindFlags & BIND_SHADER_RESOURCE) {
            D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc{};
            srv_desc.Format = srv_format;
            srv_desc.ViewDimension = ConvertDimension(p_texture_desc.dimension);
            srv_desc.Texture3D.MipLevels = p_texture_desc.mipLevels;
            srv_desc.Texture3D.MostDetailedMip = 0;

            if (gen_mip_map) {
                srv_desc.Texture3D.MipLevels = (UINT)-1;
            }

            D3D_FAIL_V_MSG(m_device->CreateShaderResourceView(texture.Get(), &srv_desc, srv.GetAddressOf()),
                           nullptr,
                           "Failed to create shader resource view");

            if (p_texture_desc.miscFlags & RESOURCE_MISC_GENERATE_MIPS) {
                m_deviceContext->GenerateMips(srv.Get());
            }

            gpu_texture->srv = srv;
        }

        if (p_texture_desc.bindFlags & BIND_UNORDERED_ACCESS) {
            // Create UAV
            D3D11_UNORDERED_ACCESS_VIEW_DESC uav_desc = {};
            uav_desc.Format = texture_format;
            uav_desc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE3D;
            uav_desc.Texture3D.MipSlice = 0;
            uav_desc.Texture3D.WSize = p_texture_desc.depth;
            uav_desc.Texture3D.FirstWSlice = 0;
            // uav_desc.Texture3D.FirstWSlice = (UINT)-1;

            D3D_FAIL_V(m_device->CreateUnorderedAccessView(texture.Get(), &uav_desc, uav.GetAddressOf()),
                       nullptr);

            gpu_texture->uav = uav;
        }

        gpu_texture->texture = texture;
        return gpu_texture;
    }

    D3D11_TEXTURE2D_DESC texture_desc{};
    texture_desc.Width = p_texture_desc.width;
    texture_desc.Height = p_texture_desc.height;
    texture_desc.MipLevels = gen_mip_map ? 0 : p_texture_desc.mipLevels;
    // texture_desc.MipLevels = 0;
    texture_desc.ArraySize = p_texture_desc.arraySize;
    texture_desc.Format = texture_format;
    texture_desc.SampleDesc = { 1, 0 };
    texture_desc.Usage = D3D11_USAGE_DEFAULT;
    texture_desc.BindFlags = d3d::ConvertBindFlags(p_texture_desc.bindFlags);
    texture_desc.CPUAccessFlags = 0;
    texture_desc.MiscFlags = d3d::ConvertResourceMiscFlags(p_texture_desc.miscFlags);
    ComPtr<ID3D11Texture2D> texture;
    if (FAILED(m_device->CreateTexture2D(&texture_desc, nullptr, texture.GetAddressOf()))) {
        return nullptr;
    }
    SetDebugName(texture.Get(), debug_name);

    if (p_texture_desc.initialData) {
        uint32_t row_pitch = p_texture_desc.width * channel_count(format) * channel_size(format);
        m_deviceContext->UpdateSubresource(texture.Get(), 0, nullptr, p_texture_desc.initialData, row_pitch, 0);
    }

    auto gpu_texture = std::make_shared<D3d11GpuTexture>(p_texture_desc);
    if (p_texture_desc.bindFlags & BIND_SHADER_RESOURCE) {
        D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc{};
        srv_desc.Format = srv_format;
        srv_desc.ViewDimension = ConvertDimension(p_texture_desc.dimension);
        srv_desc.Texture2D.MipLevels = p_texture_desc.mipLevels;
        srv_desc.Texture2D.MostDetailedMip = 0;

        switch (srv_desc.ViewDimension) {
            case D3D_SRV_DIMENSION_TEXTURE2D:
                break;
            case D3D_SRV_DIMENSION_TEXTURECUBE:
                break;
            case D3D_SRV_DIMENSION_TEXTURECUBEARRAY:
                srv_desc.TextureCubeArray.First2DArrayFace = 0;
                srv_desc.TextureCubeArray.NumCubes = p_texture_desc.arraySize / 6;
                break;
            case D3D_SRV_DIMENSION_TEXTURE3D:
                break;
            default:
                CRASH_NOW();
                break;
        }

        if (gen_mip_map) {
            srv_desc.Texture2D.MipLevels = (UINT)-1;
        }

        D3D_FAIL_V_MSG(m_device->CreateShaderResourceView(texture.Get(), &srv_desc, srv.GetAddressOf()),
                       nullptr,
                       "Failed to create shader resource view");

        if (p_texture_desc.miscFlags & RESOURCE_MISC_GENERATE_MIPS) {
            m_deviceContext->GenerateMips(srv.Get());
        }

        gpu_texture->srv = srv;
    }

    if (p_texture_desc.bindFlags & BIND_UNORDERED_ACCESS) {
        // Create UAV
        D3D11_UNORDERED_ACCESS_VIEW_DESC uav_desc = {};
        uav_desc.Format = texture_format;
        uav_desc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
        uav_desc.Texture2D.MipSlice = 0;

        D3D_FAIL_V_MSG(m_device->CreateUnorderedAccessView(texture.Get(), &uav_desc, uav.GetAddressOf()),
                       nullptr,
                       "Failed to create unordered access view");

        gpu_texture->uav = uav;
    }

    SetDebugName(texture.Get(), RenderTargetResourceNameToString(p_texture_desc.name));
    gpu_texture->texture = texture;
    return gpu_texture;
}

std::shared_ptr<Framebuffer> D3d11GraphicsManager::CreateDrawPass(const FramebufferDesc& p_subpass_desc) {
    auto framebuffer = std::make_shared<D3d11Framebuffer>(p_subpass_desc);

    for (const auto& color_attachment : p_subpass_desc.colorAttachments) {
        auto texture = reinterpret_cast<const D3d11GpuTexture*>(color_attachment.get());
        switch (color_attachment->desc.type) {
            case AttachmentType::COLOR_2D: {
                ComPtr<ID3D11RenderTargetView> rtv;
                D3D_FAIL_V(m_device->CreateRenderTargetView(texture->texture.Get(), nullptr, rtv.GetAddressOf()), nullptr);
                framebuffer->rtvs.emplace_back(rtv);
            } break;
            case AttachmentType::COLOR_CUBE: {
                int mips = color_attachment->desc.mipLevels;
                for (int mip_idx = 0; mip_idx < mips; ++mip_idx) {
                    for (uint32_t face = 0; face < color_attachment->desc.arraySize; ++face) {
                        ComPtr<ID3D11RenderTargetView> rtv;
                        D3D11_RENDER_TARGET_VIEW_DESC desc;
                        desc.Format = d3d::Convert(color_attachment->desc.format);
                        desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
                        desc.Texture2DArray.MipSlice = mip_idx;
                        desc.Texture2DArray.ArraySize = 1;
                        desc.Texture2DArray.FirstArraySlice = face;

                        D3D_FAIL_V(m_device->CreateRenderTargetView(texture->texture.Get(), &desc, rtv.GetAddressOf()), nullptr);
                        framebuffer->rtvs.push_back(rtv);
                    }
                }
            } break;
            default:
                CRASH_NOW();
                break;
        }
    }

    if (auto& depth_attachment = framebuffer->desc.depthAttachment; depth_attachment) {
        auto texture = reinterpret_cast<const D3d11GpuTexture*>(depth_attachment.get());
        switch (depth_attachment->desc.type) {
            case AttachmentType::DEPTH_2D: {
                ComPtr<ID3D11DepthStencilView> dsv;
                D3D11_DEPTH_STENCIL_VIEW_DESC dsv_desc{};
                dsv_desc.Format = DXGI_FORMAT_D32_FLOAT;
                dsv_desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
                dsv_desc.Texture2D.MipSlice = 0;

                D3D_FAIL_V(m_device->CreateDepthStencilView(texture->texture.Get(), &dsv_desc, dsv.GetAddressOf()), nullptr);
                framebuffer->dsvs.push_back(dsv);
            } break;
            case AttachmentType::DEPTH_STENCIL_2D: {
                ComPtr<ID3D11DepthStencilView> dsv;
                D3D11_DEPTH_STENCIL_VIEW_DESC dsv_desc{};
                dsv_desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
                dsv_desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
                dsv_desc.Texture2D.MipSlice = 0;

                D3D_FAIL_V(m_device->CreateDepthStencilView(texture->texture.Get(), &dsv_desc, dsv.GetAddressOf()), nullptr);
                framebuffer->dsvs.push_back(dsv);
            } break;
            case AttachmentType::SHADOW_2D: {
                ComPtr<ID3D11DepthStencilView> dsv;
                D3D11_DEPTH_STENCIL_VIEW_DESC dsv_desc{};
                dsv_desc.Format = DXGI_FORMAT_D32_FLOAT;
                dsv_desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
                dsv_desc.Texture2D.MipSlice = 0;

                D3D_FAIL_V(m_device->CreateDepthStencilView(texture->texture.Get(), &dsv_desc, dsv.GetAddressOf()), nullptr);
                framebuffer->dsvs.push_back(dsv);
            } break;
            case AttachmentType::SHADOW_CUBE_ARRAY: {
                for (uint32_t face = 0; face < depth_attachment->desc.arraySize; ++face) {
                    ComPtr<ID3D11DepthStencilView> dsv;
                    D3D11_DEPTH_STENCIL_VIEW_DESC dsv_desc{};
                    dsv_desc.Format = DXGI_FORMAT_D32_FLOAT;
                    dsv_desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
                    dsv_desc.Texture2DArray.MipSlice = 0;
                    dsv_desc.Texture2DArray.ArraySize = 1;
                    dsv_desc.Texture2DArray.FirstArraySlice = face;

                    D3D_FAIL_V(m_device->CreateDepthStencilView(texture->texture.Get(), &dsv_desc, dsv.GetAddressOf()), nullptr);
                    framebuffer->dsvs.push_back(dsv);
                }
            } break;
            default:
                CRASH_NOW();
                break;
        }
    }

    return framebuffer;
}

void D3d11GraphicsManager::SetRenderTarget(const Framebuffer* p_framebuffer, int p_index, int p_mip_level) {
    unused(p_mip_level);
    DEV_ASSERT(p_framebuffer);

    if (p_framebuffer->desc.type == FramebufferDesc::SCREEN) {
        m_deviceContext->OMSetRenderTargets(1, m_windowRtv.GetAddressOf(), nullptr);
        return;
    }

    auto framebuffer = reinterpret_cast<const D3d11Framebuffer*>(p_framebuffer);
    if (const auto depth_attachment = framebuffer->desc.depthAttachment; depth_attachment) {
        if (depth_attachment->desc.type == AttachmentType::SHADOW_CUBE_ARRAY) {
            ID3D11RenderTargetView* rtv = nullptr;
            m_deviceContext->OMSetRenderTargets(1, &rtv, framebuffer->dsvs[p_index].Get());
            return;
        }
    }

    // @TODO: fixed_vector
    std::vector<ID3D11RenderTargetView*> rtvs;
    for (auto& rtv : framebuffer->rtvs) {
        rtvs.emplace_back(rtv.Get());
    }

    ID3D11DepthStencilView* dsv = framebuffer->dsvs.size() ? framebuffer->dsvs[p_index].Get() : nullptr;

    if (rtvs.size()) {
        if (p_framebuffer->desc.colorAttachments[0]->desc.type == AttachmentType::COLOR_CUBE) {
            int offset = p_index + 6 * p_mip_level;
            m_deviceContext->OMSetRenderTargets(1, rtvs.data() + offset, dsv);
            return;
        }
    }

    m_deviceContext->OMSetRenderTargets((UINT)rtvs.size(), rtvs.data(), dsv);
}

void D3d11GraphicsManager::UnsetRenderTarget() {
    ID3D11RenderTargetView* rtvs[] = { nullptr, nullptr, nullptr, nullptr };
    m_deviceContext->OMSetRenderTargets(array_length(rtvs), rtvs, nullptr);
}

void D3d11GraphicsManager::Clear(const Framebuffer* p_framebuffer, ClearFlags p_flags, const float* p_clear_color, int p_index) {
    // @TODO: refactor
    const bool clear_color = p_flags & CLEAR_COLOR_BIT;
    const bool clear_depth = p_flags & CLEAR_DEPTH_BIT;
    const bool clear_stencil = p_flags & CLEAR_STENCIL_BIT;
    if (p_framebuffer->desc.type == FramebufferDesc::SCREEN) {
        if (clear_color) {
            m_deviceContext->ClearRenderTargetView(m_windowRtv.Get(), p_clear_color);
        }
        return;
    }

    auto framebuffer = reinterpret_cast<const D3d11Framebuffer*>(p_framebuffer);

    if (clear_color) {
        for (auto& rtv : framebuffer->rtvs) {
            m_deviceContext->ClearRenderTargetView(rtv.Get(), p_clear_color);
        }
    }

    uint32_t clear_flags = 0;
    if (clear_depth) {
        clear_flags |= D3D11_CLEAR_DEPTH;
    }
    if (clear_stencil) {
        clear_flags |= D3D11_CLEAR_STENCIL;
    }
    if (clear_flags) {
        // @TODO: better way?
        DEV_ASSERT_INDEX(p_index, framebuffer->dsvs.size());
        m_deviceContext->ClearDepthStencilView(framebuffer->dsvs[p_index].Get(), clear_flags, 1.0f, 0);
    }
}

void D3d11GraphicsManager::SetViewport(const Viewport& p_viewport) {
    D3D11_VIEWPORT vp{};
    // @TODO: gl and d3d use different viewport
    vp.TopLeftX = static_cast<float>(p_viewport.topLeftX);
    vp.TopLeftY = static_cast<float>(p_viewport.topLeftY);
    vp.Width = static_cast<float>(p_viewport.width);
    vp.Height = static_cast<float>(p_viewport.height);
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;

    m_deviceContext->RSSetViewports(1, &vp);
}

auto D3d11GraphicsManager::CreateBuffer(const GpuBufferDesc& p_desc) -> Result<std::shared_ptr<GpuBuffer>> {
    const bool is_dynamic = p_desc.dynamic;
    ComPtr<ID3D11Buffer> buffer;

    uint32_t flags = 0;
    switch (p_desc.type) {
        case GpuBufferType::VERTEX:
            flags |= D3D11_BIND_VERTEX_BUFFER;
            break;
        case GpuBufferType::INDEX:
            flags |= D3D11_BIND_INDEX_BUFFER;
            break;
        default:
            CRASH_NOW();
            break;
    }

    D3D11_BUFFER_DESC bufferDesc{};
    bufferDesc.Usage = is_dynamic ? D3D11_USAGE_DYNAMIC : D3D11_USAGE_IMMUTABLE;
    bufferDesc.ByteWidth = p_desc.elementCount * p_desc.elementSize;
    bufferDesc.BindFlags = flags;
    bufferDesc.CPUAccessFlags = is_dynamic ? D3D11_CPU_ACCESS_WRITE : 0;
    bufferDesc.MiscFlags = 0;

    D3D11_SUBRESOURCE_DATA data{};
    data.pSysMem = p_desc.initialData;
    D3D_FAIL_V_MSG(m_device->CreateBuffer(&bufferDesc, &data, buffer.GetAddressOf()),
                   HBN_ERROR(ErrorCode::ERR_CANT_CREATE, "failed to create buffer"),
                   "Failed to Create vertex buffer");

    auto ret = std::make_shared<D3d11Buffer>(p_desc);
    ret->buffer = buffer;
    return ret;
}

auto D3d11GraphicsManager::CreateMeshImpl(const GpuMeshDesc& p_desc,
                                          uint32_t p_count,
                                          const GpuBufferDesc* p_vb_descs,
                                          const GpuBufferDesc* p_ib_desc) -> Result<std::shared_ptr<GpuMesh>> {
    auto ret = std::make_shared<D3d11MeshBuffers>(p_desc);

    for (uint32_t index = 0; index < p_count; ++index) {
        if (!p_vb_descs[index].elementCount) {
            continue;
        }
        auto res = CreateBuffer(p_vb_descs[index]);
        if (!res) {
            return HBN_ERROR(res.error());
        }
        ret->vertexBuffers[index] = *res;
    }

    if (p_ib_desc) {
        auto res = CreateBuffer(*p_ib_desc);
        if (!res) {
            return HBN_ERROR(res.error());
        }

        ret->indexBuffer = *res;
    }
    return ret;
}

void D3d11GraphicsManager::SetMesh(const GpuMesh* p_mesh) {
    auto mesh = reinterpret_cast<const D3d11MeshBuffers*>(p_mesh);

    std::array<ID3D11Buffer*, MESH_MAX_VERTEX_BUFFER_COUNT> buffers{ nullptr };
    std::array<uint32_t, MESH_MAX_VERTEX_BUFFER_COUNT> strides{ 0 };
    std::array<uint32_t, MESH_MAX_VERTEX_BUFFER_COUNT> offsets{ 0 };

    for (int index = 0; index < mesh->vertexBuffers.size(); ++index) {
        const auto& vertex = mesh->vertexBuffers[index];
        if (vertex == nullptr) {
            continue;
        }
        buffers[index] = (ID3D11Buffer*)vertex->GetHandle();
        strides[index] = p_mesh->desc.vertexLayout[index].strideInByte;
        offsets[index] = p_mesh->desc.vertexLayout[index].offsetInByte;
    }

    m_deviceContext->IASetVertexBuffers(0, MESH_MAX_VERTEX_BUFFER_COUNT, buffers.data(), strides.data(), offsets.data());

    if (mesh->indexBuffer) {
        ID3D11Buffer* index_buffer = (ID3D11Buffer*)mesh->indexBuffer->GetHandle();
        m_deviceContext->IASetIndexBuffer(index_buffer, DXGI_FORMAT_R32_UINT, 0);
    }
}

void D3d11GraphicsManager::UpdateBuffer(const GpuBufferDesc& p_desc, GpuBuffer* p_buffer) {
    auto buffer = reinterpret_cast<D3d11Buffer*>(p_buffer);
    D3D11_MAPPED_SUBRESOURCE mapped;
    HRESULT hr = m_deviceContext->Map(buffer->buffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
    DEV_ASSERT(SUCCEEDED(hr));

    memcpy(mapped.pData, p_desc.initialData, p_desc.elementSize * p_desc.elementCount);
    m_deviceContext->Unmap(buffer->buffer.Get(), 0);
}

void D3d11GraphicsManager::DrawElements(uint32_t p_count, uint32_t p_offset) {
    m_deviceContext->DrawIndexed(p_count, p_offset, 0);
}

void D3d11GraphicsManager::DrawElementsInstanced(uint32_t p_instance_count, uint32_t p_count, uint32_t p_offset) {
    m_deviceContext->DrawIndexedInstanced(p_count, p_instance_count, p_offset, 0, 0);
}

void D3d11GraphicsManager::DrawArrays(uint32_t p_count, uint32_t p_offset) {
    m_deviceContext->Draw(p_count, p_offset);
}

void D3d11GraphicsManager::DrawArraysInstanced(uint32_t p_instance_count, uint32_t p_count, uint32_t p_offset) {
    m_deviceContext->DrawInstanced(p_count, p_instance_count, p_offset, 0);
}

void D3d11GraphicsManager::SetPipelineStateImpl(PipelineStateName p_name) {
    auto pipeline = reinterpret_cast<D3d11PipelineState*>(m_pipelineStateManager->Find(p_name));
    DEV_ASSERT(pipeline);
    if (pipeline->computeShader) {
        m_deviceContext->CSSetShader(pipeline->computeShader.Get(), nullptr, 0);
        return;
    }

    if (pipeline->vertexShader) {
        m_deviceContext->VSSetShader(pipeline->vertexShader.Get(), 0, 0);
        m_deviceContext->IASetInputLayout(pipeline->inputLayout.Get());
    }
    if (pipeline->pixelShader) {
        m_deviceContext->PSSetShader(pipeline->pixelShader.Get(), 0, 0);
    }
    if (pipeline->rasterizerState.Get() != m_stateCache.rasterizer) {
        m_deviceContext->RSSetState(pipeline->rasterizerState.Get());
        m_stateCache.rasterizer = pipeline->rasterizerState.Get();
    }
    if (pipeline->depthStencilState.Get() != m_stateCache.depthStencil) {
        m_deviceContext->OMSetDepthStencilState(pipeline->depthStencilState.Get(), 0);
        m_stateCache.depthStencil = pipeline->depthStencilState.Get();
    }
    if (pipeline->blendState.Get() != m_stateCache.blendState) {
        // @TODO: remove hard code mask
        m_deviceContext->OMSetBlendState(pipeline->blendState.Get(), nullptr, 0xFFFFFFFF);
        m_stateCache.blendState = pipeline->blendState.Get();
    }

    auto topology = d3d::Convert(pipeline->desc.primitiveTopology);
    m_deviceContext->IASetPrimitiveTopology(topology);
}

}  // namespace my

#undef INCLUDE_AS_D3D11
