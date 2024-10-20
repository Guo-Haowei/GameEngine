#include "d3d11_graphics_manager.h"

#include <dxgi.h>
#include <imgui/backends/imgui_impl_dx11.h>

#include "drivers/d3d11/convert.h"
#include "drivers/d3d11/d3d11_helpers.h"
#include "drivers/d3d11/d3d11_pipeline_state_manager.h"
#include "drivers/d3d11/d3d11_resources.h"
#include "drivers/windows/win32_display_manager.h"
#include "rendering/render_graph/render_graph_defines.h"
#include "rendering/rendering_dvars.h"
#include "rendering/texture.h"

namespace my {

using Microsoft::WRL::ComPtr;

// @TODO: fix this
ComPtr<ID3D11SamplerState> m_sampler_state;
ComPtr<ID3D11SamplerState> m_shadow_sampler_state;

D3d11GraphicsManager::D3d11GraphicsManager() : GraphicsManager("D3d11GraphicsManager", Backend::D3D11) {
    m_pipeline_state_manager = std::make_shared<D3d11PipelineStateManager>();
}

bool D3d11GraphicsManager::initializeImpl() {
    bool ok = true;
    ok = ok && createDevice();
    ok = ok && createSwapChain();
    ok = ok && createRenderTarget();
    ok = ok && ImGui_ImplDX11_Init(m_device.Get(), m_ctx.Get());

    ImGui_ImplDX11_NewFrame();

    selectRenderGraph();
    // @TODO: refactor this
    m_ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    {
        // @TODO: refactor sampler
        D3D11_SAMPLER_DESC sampler_desc{};
        sampler_desc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
        sampler_desc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
        sampler_desc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
        sampler_desc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
        sampler_desc.MaxAnisotropy = 1;
        sampler_desc.MinLOD = 0;
        sampler_desc.MaxLOD = D3D11_FLOAT32_MAX;
        sampler_desc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
        auto hr = m_device->CreateSamplerState(&sampler_desc, m_sampler_state.GetAddressOf());
        DEV_ASSERT(SUCCEEDED(hr));

        m_ctx->PSSetSamplers(0, 1, m_sampler_state.GetAddressOf());
    }
    {
        D3D11_SAMPLER_DESC sampler_desc{};
        sampler_desc.Filter = D3D11_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT;
        sampler_desc.AddressU = D3D11_TEXTURE_ADDRESS_BORDER;
        sampler_desc.AddressV = D3D11_TEXTURE_ADDRESS_BORDER;
        sampler_desc.AddressW = D3D11_TEXTURE_ADDRESS_BORDER;
        sampler_desc.MaxAnisotropy = 16;
        sampler_desc.MinLOD = 0;
        sampler_desc.MaxLOD = D3D11_FLOAT32_MAX;
        sampler_desc.ComparisonFunc = D3D11_COMPARISON_LESS_EQUAL;
        sampler_desc.BorderColor[0] = 0.0f;
        sampler_desc.BorderColor[1] = 0.0f;
        sampler_desc.BorderColor[2] = 0.0f;
        sampler_desc.BorderColor[3] = 1.0f;

        auto hr = m_device->CreateSamplerState(&sampler_desc, m_shadow_sampler_state.GetAddressOf());
        DEV_ASSERT(SUCCEEDED(hr));

        m_ctx->PSSetSamplers(1, 1, m_sampler_state.GetAddressOf());
    }

    m_meshes.set_description("GPU-Mesh-Allocator");
    return ok;
}

void D3d11GraphicsManager::finalize() {
    ImGui_ImplDX11_Shutdown();
}

void D3d11GraphicsManager::render() {
    m_render_graph.execute();

    // @TODO: fix the following
    const float clear_color[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    m_ctx->OMSetRenderTargets(1, m_window_rtv.GetAddressOf(), nullptr);
    m_ctx->ClearRenderTargetView(m_window_rtv.Get(), clear_color);
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

    ImGuiIO& io = ImGui::GetIO();
    // Update and Render additional Platform Windows
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
    }

    m_swap_chain->Present(1, 0);  // Present with vsync
}

void D3d11GraphicsManager::setStencilRef(uint32_t p_ref) {
    unused(p_ref);
}

void D3d11GraphicsManager::onWindowResize(int p_width, int p_height) {
    if (m_device) {
        m_window_rtv.Reset();
        m_swap_chain->ResizeBuffers(0, p_width, p_height, DXGI_FORMAT_UNKNOWN, 0);
        createRenderTarget();
    }
}

bool D3d11GraphicsManager::createDevice() {
    D3D_FEATURE_LEVEL feature_level = D3D_FEATURE_LEVEL_11_0;
    UINT create_device_flags = 0;
    if (DVAR_GET_BOOL(r_gpu_validation)) {
        create_device_flags |= D3D11_CREATE_DEVICE_DEBUG;
    }

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
        m_ctx.GetAddressOf());

    D3D_FAIL_V_MSG(hr, false, "Failed to create d3d11 device");

    D3D_FAIL_V_MSG(m_device->QueryInterface(__uuidof(IDXGIDevice), (void**)m_dxgi_device.GetAddressOf()),
                   false,
                   "Failed to query IDXGIDevice");

    D3D_FAIL_V_MSG(m_dxgi_device->GetParent(__uuidof(IDXGIAdapter), (void**)m_dxgi_adapter.GetAddressOf()),
                   false,
                   "Failed to query IDXGIAdapter");

    D3D_FAIL_V_MSG(m_dxgi_adapter->GetParent(__uuidof(IDXGIFactory), (void**)m_dxgi_factory.GetAddressOf()),
                   false,
                   "Failed to query IDXGIFactory");

    return true;
}

bool D3d11GraphicsManager::createSwapChain() {
    auto display_manager = dynamic_cast<Win32DisplayManager*>(DisplayManager::singleton_ptr());
    DEV_ASSERT(display_manager);

    DXGI_MODE_DESC buffer_desc{};
    buffer_desc.Width = 0;
    buffer_desc.Height = 0;
    buffer_desc.RefreshRate = { 60, 1 };
    buffer_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    buffer_desc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;

    DXGI_SWAP_CHAIN_DESC swap_chain_desc{};
    swap_chain_desc.BufferDesc = buffer_desc;
    swap_chain_desc.SampleDesc = { 1, 0 };
    swap_chain_desc.BufferCount = 2;
    swap_chain_desc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    swap_chain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swap_chain_desc.OutputWindow = display_manager->get_hwnd();
    swap_chain_desc.Windowed = true;
    swap_chain_desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

    D3D_FAIL_V_MSG(m_dxgi_factory->CreateSwapChain(m_device.Get(), &swap_chain_desc, m_swap_chain.GetAddressOf()),
                   false,
                   "Failed to create swap chain");

    return true;
}

bool D3d11GraphicsManager::createRenderTarget() {
    ComPtr<ID3D11Texture2D> back_buffer;
    m_swap_chain->GetBuffer(0, IID_PPV_ARGS(back_buffer.GetAddressOf()));
    m_device->CreateRenderTargetView(back_buffer.Get(), nullptr, m_window_rtv.GetAddressOf());
    return true;
}

std::shared_ptr<UniformBufferBase> D3d11GraphicsManager::createUniform(int p_slot, size_t p_capacity) {
    ComPtr<ID3D11Buffer> buffer;
    D3D11_BUFFER_DESC buffer_desc;
    buffer_desc.ByteWidth = (UINT)p_capacity;
    buffer_desc.Usage = D3D11_USAGE_DYNAMIC;
    buffer_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    buffer_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    buffer_desc.MiscFlags = 0;
    buffer_desc.StructureByteStride = 0;

    D3D_FAIL_V_MSG(m_device->CreateBuffer(&buffer_desc, nullptr, buffer.GetAddressOf()),
                   nullptr,
                   "Failed to create constant buffer");

    auto uniform_buffer = std::make_shared<D3d11UniformBuffer>(p_slot, p_capacity);
    uniform_buffer->buffer = buffer;

    m_ctx->VSSetConstantBuffers(p_slot, 1, uniform_buffer->buffer.GetAddressOf());
    m_ctx->PSSetConstantBuffers(p_slot, 1, uniform_buffer->buffer.GetAddressOf());
    return uniform_buffer;
}

void D3d11GraphicsManager::updateUniform(const UniformBufferBase* p_buffer, const void* p_data, size_t p_size) {
    auto buffer = reinterpret_cast<const D3d11UniformBuffer*>(p_buffer);
    DEV_ASSERT(p_size <= buffer->get_capacity());
    buffer->data = (const char*)p_data;
}

void D3d11GraphicsManager::bindUniformRange(const UniformBufferBase* p_buffer, uint32_t p_size, uint32_t p_offset) {
    auto buffer = reinterpret_cast<const D3d11UniformBuffer*>(p_buffer);
    DEV_ASSERT(p_size + p_offset <= buffer->get_capacity());
    D3D11_MAPPED_SUBRESOURCE mapped;
    ZeroMemory(&mapped, sizeof(D3D11_MAPPED_SUBRESOURCE));
    m_ctx->Map(buffer->buffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
    memcpy(mapped.pData, buffer->data + p_offset, p_size);
    m_ctx->Unmap(buffer->buffer.Get(), 0);
}

void D3d11GraphicsManager::bindTexture(Dimension p_dimension, uint64_t p_handle, int p_slot) {
    unused(p_dimension);

    if (p_handle) {
        ID3D11ShaderResourceView* srv = (ID3D11ShaderResourceView*)(p_handle);
        m_ctx->PSSetShaderResources(p_slot, 1, &srv);
    }
}

void D3d11GraphicsManager::unbindTexture(Dimension p_dimension, int p_slot) {
    unused(p_dimension);

    ID3D11ShaderResourceView* srv = nullptr;
    m_ctx->PSSetShaderResources(p_slot, 1, &srv);
}

std::shared_ptr<Texture> D3d11GraphicsManager::createTexture(const TextureDesc& p_texture_desc, const SamplerDesc& p_sampler_desc) {
    unused(p_sampler_desc);

    ComPtr<ID3D11ShaderResourceView> srv;

    PixelFormat format = p_texture_desc.format;
    DXGI_FORMAT texture_format = convert(format);
    DXGI_FORMAT srv_format = convert(format);
    if (format == PixelFormat::D32_FLOAT) {
        texture_format = DXGI_FORMAT_R32_TYPELESS;
        srv_format = DXGI_FORMAT_R32_FLOAT;
    }
    if (format == PixelFormat::D24_UNORM_S8_UINT) {
        texture_format = DXGI_FORMAT_R24G8_TYPELESS;
    }

    const bool gen_mip_map = p_texture_desc.bind_flags & BIND_SHADER_RESOURCE;

    D3D11_TEXTURE2D_DESC texture_desc{};
    texture_desc.Width = p_texture_desc.width;
    texture_desc.Height = p_texture_desc.height;
    // texture_desc.MipLevels = gen_mip_map ? 0 : p_texture_desc.mip_levels;
    texture_desc.MipLevels = 0;
    texture_desc.ArraySize = p_texture_desc.array_size;
    texture_desc.Format = texture_format;
    texture_desc.SampleDesc = { 1, 0 };
    texture_desc.Usage = D3D11_USAGE_DEFAULT;
    texture_desc.BindFlags = d3d11::convert_bind_flags(p_texture_desc.bind_flags);
    texture_desc.CPUAccessFlags = 0;
    texture_desc.MiscFlags = d3d11::convert_misc_flags(p_texture_desc.misc_flags);
    ComPtr<ID3D11Texture2D> texture;
    D3D_FAIL_V_MSG(m_device->CreateTexture2D(&texture_desc, nullptr, texture.GetAddressOf()),
                   nullptr,
                   "Failed to create texture");

    if (p_texture_desc.initial_data) {
        uint32_t row_pitch = p_texture_desc.width * channel_count(format) * channel_size(format);
        m_ctx->UpdateSubresource(texture.Get(), 0, nullptr, p_texture_desc.initial_data, row_pitch, 0);
    }

    auto gpu_texture = std::make_shared<D3d11Texture>(p_texture_desc);
    if (p_texture_desc.bind_flags & BIND_SHADER_RESOURCE) {
        D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc{};
        srv_desc.Format = srv_format;
        srv_desc.ViewDimension = d3d11::convert_dimension(p_texture_desc.dimension);
        srv_desc.Texture2D.MipLevels = p_texture_desc.mip_levels;
        srv_desc.Texture2D.MostDetailedMip = 0;

        if (gen_mip_map) {
            srv_desc.Texture2D.MipLevels = (UINT)-1;
        }

        D3D_FAIL_V_MSG(m_device->CreateShaderResourceView(texture.Get(), &srv_desc, srv.GetAddressOf()),
                       nullptr,
                       "Failed to create shader resource view");

        if (p_texture_desc.misc_flags & RESOURCE_MISC_GENERATE_MIPS) {
            m_ctx->GenerateMips(srv.Get());
        }

        gpu_texture->srv = srv;
    }

    gpu_texture->texture = texture;

    return gpu_texture;
}

std::shared_ptr<DrawPass> D3d11GraphicsManager::createDrawPass(const DrawPassDesc& p_subpass_desc) {
    auto subpass = std::make_shared<D3d11Subpass>();
    subpass->exec_func = p_subpass_desc.exec_func;
    subpass->color_attachments = p_subpass_desc.color_attachments;
    subpass->depth_attachment = p_subpass_desc.depth_attachment;

    for (const auto& color_attachment : p_subpass_desc.color_attachments) {
        auto texture = reinterpret_cast<const D3d11Texture*>(color_attachment->texture.get());
        switch (color_attachment->desc.type) {
            case AttachmentType::COLOR_2D: {
                ComPtr<ID3D11RenderTargetView> rtv;
                D3D_FAIL_V_MSG(m_device->CreateRenderTargetView(texture->texture.Get(), nullptr, rtv.GetAddressOf()),
                               nullptr,
                               "Failed to create render target view");
                subpass->rtvs.emplace_back(rtv);
            } break;
            default:
                CRASH_NOW();
                break;
        }
    }

    if (auto& depth_attachment = subpass->depth_attachment; depth_attachment) {
        auto texture = reinterpret_cast<const D3d11Texture*>(depth_attachment->texture.get());
        switch (depth_attachment->desc.type) {
            case AttachmentType::DEPTH_2D: {
                ComPtr<ID3D11DepthStencilView> dsv;
                D3D11_DEPTH_STENCIL_VIEW_DESC desc{};
                desc.Format = DXGI_FORMAT_D32_FLOAT;
                desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
                desc.Texture2D.MipSlice = 0;

                D3D_FAIL_V_MSG(m_device->CreateDepthStencilView(texture->texture.Get(), &desc, dsv.GetAddressOf()),
                               nullptr,
                               "Failed to create depth stencil view");
                subpass->dsv = dsv;
            } break;
            case AttachmentType::DEPTH_STENCIL_2D: {
                ComPtr<ID3D11DepthStencilView> dsv;
                D3D11_DEPTH_STENCIL_VIEW_DESC desc{};
                desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
                desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
                desc.Texture2D.MipSlice = 0;

                D3D_FAIL_V_MSG(m_device->CreateDepthStencilView(texture->texture.Get(), &desc, dsv.GetAddressOf()),
                               nullptr,
                               "Failed to create depth stencil view");
                subpass->dsv = dsv;
            } break;
            case AttachmentType::SHADOW_2D: {
                ComPtr<ID3D11DepthStencilView> dsv;
                D3D11_DEPTH_STENCIL_VIEW_DESC desc{};
                desc.Format = DXGI_FORMAT_D32_FLOAT;
                desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
                desc.Texture2D.MipSlice = 0;

                D3D_FAIL_V_MSG(m_device->CreateDepthStencilView(texture->texture.Get(), &desc, dsv.GetAddressOf()),
                               nullptr,
                               "Failed to create depth stencil view");
                subpass->dsv = dsv;
            } break;
            default:
                CRASH_NOW();
                break;
        }
    }

    return subpass;
}

void D3d11GraphicsManager::setRenderTarget(const DrawPass* p_subpass, int p_index, int p_mip_level) {
    unused(p_index);
    unused(p_mip_level);

    auto subpass = reinterpret_cast<const D3d11Subpass*>(p_subpass);

    // @TODO: fixed_vector
    std::vector<ID3D11RenderTargetView*> rtvs;
    for (auto& rtv : subpass->rtvs) {
        rtvs.push_back(rtv.Get());
    }

    ID3D11DepthStencilView* dsv = subpass->dsv.Get();
    m_ctx->OMSetRenderTargets((UINT)rtvs.size(), rtvs.data(), dsv);
}

void D3d11GraphicsManager::unsetRenderTarget() {
    ID3D11RenderTargetView* rtv = nullptr;
    m_ctx->OMSetRenderTargets(1, &rtv, nullptr);
}

void D3d11GraphicsManager::clear(const DrawPass* p_subpass, uint32_t p_flags, float* p_clear_color) {
    auto subpass = reinterpret_cast<const D3d11Subpass*>(p_subpass);

    if (p_flags & CLEAR_COLOR_BIT) {
        float clear_color[] = { 0.0f, 0.0f, 0.0f, 1.0f };
        if (p_clear_color) {
            clear_color[0] = p_clear_color[0];
            clear_color[1] = p_clear_color[1];
            clear_color[2] = p_clear_color[2];
            clear_color[3] = p_clear_color[3];
        }

        for (auto& rtv : subpass->rtvs) {
            m_ctx->ClearRenderTargetView(rtv.Get(), clear_color);
        }
    }

    uint32_t clear_flags = 0;
    if (p_flags & CLEAR_DEPTH_BIT) {
        clear_flags |= D3D11_CLEAR_DEPTH;
    }
    if (p_flags & D3D11_CLEAR_STENCIL) {
        clear_flags |= D3D11_CLEAR_STENCIL;
    }
    if (clear_flags) {
        // @TODO: configure clear depth
        m_ctx->ClearDepthStencilView(subpass->dsv.Get(), clear_flags, 1.0f, 0);
    }
}

void D3d11GraphicsManager::setViewport(const Viewport& p_viewport) {
    D3D11_VIEWPORT vp{};
    // @TODO: gl and d3d use different viewport
    vp.TopLeftX = static_cast<float>(p_viewport.top_left_x);
    vp.TopLeftY = static_cast<float>(p_viewport.top_left_y);
    vp.Width = static_cast<float>(p_viewport.width);
    vp.Height = static_cast<float>(p_viewport.height);
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;

    m_ctx->RSSetViewports(1, &vp);
}

const MeshBuffers* D3d11GraphicsManager::createMesh(const MeshComponent& p_mesh) {
    auto createMesh_data = [](ID3D11Device* p_device, const MeshComponent& mesh, D3d11MeshBuffers& out_mesh) {
        auto create_vertex_buffer = [&](size_t p_size_in_byte, const void* p_data) -> ID3D11Buffer* {
            ID3D11Buffer* buffer = nullptr;
            // vertex buffer
            D3D11_BUFFER_DESC bufferDesc{};
            bufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
            bufferDesc.ByteWidth = (UINT)p_size_in_byte;
            bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
            bufferDesc.CPUAccessFlags = 0;
            bufferDesc.MiscFlags = 0;

            D3D11_SUBRESOURCE_DATA data{};
            data.pSysMem = p_data;
            D3D_FAIL_V_MSG(p_device->CreateBuffer(&bufferDesc, &data, &buffer),
                           nullptr,
                           "Failed to create vertex buffer");
            return buffer;
        };

        out_mesh.vertex_buffer[0] = create_vertex_buffer(sizeof(vec3) * mesh.positions.size(), mesh.positions.data());

        if (!mesh.normals.empty()) {
            out_mesh.vertex_buffer[1] = create_vertex_buffer(sizeof(vec3) * mesh.normals.size(), mesh.normals.data());
        }

        if (!mesh.texcoords_0.empty()) {
            out_mesh.vertex_buffer[2] = create_vertex_buffer(sizeof(vec2) * mesh.texcoords_0.size(), mesh.texcoords_0.data());
        }

        if (!mesh.joints_0.empty()) {
            out_mesh.vertex_buffer[4] = create_vertex_buffer(sizeof(ivec4) * mesh.joints_0.size(), mesh.joints_0.data());
        }
        if (!mesh.weights_0.empty()) {
            out_mesh.vertex_buffer[5] = create_vertex_buffer(sizeof(vec4) * mesh.weights_0.size(), mesh.weights_0.data());
        }
        {
            // index buffer
            out_mesh.index_count = static_cast<uint32_t>(mesh.indices.size());
            D3D11_BUFFER_DESC bufferDesc{};
            bufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
            bufferDesc.ByteWidth = static_cast<uint32_t>(sizeof(uint32_t) * out_mesh.index_count);
            bufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
            bufferDesc.CPUAccessFlags = 0;
            bufferDesc.MiscFlags = 0;

            D3D11_SUBRESOURCE_DATA data{};
            data.pSysMem = mesh.indices.data();
            D3D_FAIL_MSG(p_device->CreateBuffer(&bufferDesc, &data, out_mesh.index_buffer.GetAddressOf()),
                         "Failed to create index buffer");
        }
    };

    RID rid = m_meshes.make_rid();
    D3d11MeshBuffers* mesh_buffers = m_meshes.get_or_null(rid);
    p_mesh.gpu_resource = mesh_buffers;
    createMesh_data(m_device.Get(), p_mesh, *mesh_buffers);
    return mesh_buffers;
}

void D3d11GraphicsManager::setMesh(const MeshBuffers* p_mesh) {
    auto mesh = reinterpret_cast<const D3d11MeshBuffers*>(p_mesh);

    ID3D11Buffer* buffers[6] = {
        mesh->vertex_buffer[0].Get(),
        mesh->vertex_buffer[1].Get(),
        mesh->vertex_buffer[2].Get(),
        mesh->vertex_buffer[3].Get(),
        mesh->vertex_buffer[4].Get(),
        mesh->vertex_buffer[5].Get(),
    };

    UINT stride[6] = {
        sizeof(vec3),
        sizeof(vec3),
        sizeof(vec2),
        sizeof(vec3),
        sizeof(ivec4),
        sizeof(vec4),
    };

    UINT offset[6] = { 0, 0, 0, 0, 0, 0 };

    // @TODO: fix
    m_ctx->IASetVertexBuffers(0, 6, buffers, stride, offset);
    m_ctx->IASetIndexBuffer(mesh->index_buffer.Get(), DXGI_FORMAT_R32_UINT, 0);
}

void D3d11GraphicsManager::drawElements(uint32_t p_count, uint32_t p_offset) {
    m_ctx->DrawIndexed(p_count, p_offset, 0);
}

void D3d11GraphicsManager::onSceneChange(const Scene& p_scene) {
    for (auto [entity, mesh] : p_scene.m_MeshComponents) {
        if (mesh.gpu_resource != nullptr) {
            const NameComponent& name = *p_scene.getComponent<NameComponent>(entity);
            LOG_WARN("[begin_scene] mesh '{}' () already has gpu resource", name.getName());
            continue;
        }

        createMesh(mesh);
    }
}

void D3d11GraphicsManager::setPipelineStateImpl(PipelineStateName p_name) {
    auto pipeline = reinterpret_cast<D3d11PipelineState*>(m_pipeline_state_manager->find(p_name));
    DEV_ASSERT(pipeline);
    if (pipeline->vertex_shader) {
        m_ctx->VSSetShader(pipeline->vertex_shader.Get(), 0, 0);
        m_ctx->IASetInputLayout(pipeline->input_layout.Get());
    }
    if (pipeline->pixel_shader) {
        m_ctx->PSSetShader(pipeline->pixel_shader.Get(), 0, 0);
    }

    if (pipeline->rasterizer.Get() != m_state_cache.rasterizer) {
        m_ctx->RSSetState(pipeline->rasterizer.Get());
        m_state_cache.rasterizer = pipeline->rasterizer.Get();
    }

    if (pipeline->depth_stencil.Get() != m_state_cache.depth_stencil) {
        m_ctx->OMSetDepthStencilState(pipeline->depth_stencil.Get(), 0);
        m_state_cache.depth_stencil = pipeline->depth_stencil.Get();
    }
}

}  // namespace my
