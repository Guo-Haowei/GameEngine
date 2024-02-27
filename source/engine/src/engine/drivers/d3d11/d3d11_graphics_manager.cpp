#include "d3d11_graphics_manager.h"

#include <dxgi.h>
#include <imgui/backends/imgui_impl_dx11.h>

#include "drivers/d3d11/convert.h"
#include "drivers/d3d11/d3d11_helpers.h"
#include "drivers/d3d11/d3d11_pipeline_state_manager.h"
#include "drivers/d3d11/d3d11_resources.h"
#include "drivers/windows/win32_display_manager.h"
#include "rendering/render_graph/render_graphs.h"
#include "rendering/rendering_dvars.h"
#include "rendering/texture.h"
// @TODO: refactor
#include "core/framework/scene_manager.h"
#include "rendering/renderer.h"
extern ID3D11Device* get_d3d11_device();

using Microsoft::WRL::ComPtr;

// rasterizer
ComPtr<ID3D11RasterizerState> m_rasterizer;
// reverse depth
ComPtr<ID3D11DepthStencilState> m_depthStencilState;

namespace my {

// @TODO: refactor
template<class Cache>
class ConstantBuffer {
public:
    inline size_t BufferSize() const { return sizeof(Cache); }

    ConstantBuffer() = default;

    void Create(ComPtr<ID3D11Device>& device) {
        D3D11_BUFFER_DESC bufferDesc;
        bufferDesc.ByteWidth = sizeof(Cache);
        bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
        bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        bufferDesc.MiscFlags = 0;
        bufferDesc.StructureByteStride = 0;

        D3D_FAIL_MSG(device->CreateBuffer(&bufferDesc, nullptr, m_buffer.GetAddressOf()),
                     "Failed to create constant buffer");
    }

    void Update(ComPtr<ID3D11DeviceContext>& deviceContext) {
        D3D11_MAPPED_SUBRESOURCE mapped;
        ZeroMemory(&mapped, sizeof(D3D11_MAPPED_SUBRESOURCE));
        deviceContext->Map(m_buffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
        memcpy(mapped.pData, (void*)&m_cache, sizeof(Cache));
        deviceContext->Unmap(m_buffer.Get(), 0);
    }

    void VSSet(ComPtr<ID3D11DeviceContext>& deviceContext, uint32_t slot) {
        deviceContext->VSSetConstantBuffers(slot, 1, m_buffer.GetAddressOf());
    }

    void PSSet(ComPtr<ID3D11DeviceContext>& deviceContext, uint32_t slot) {
        deviceContext->PSSetConstantBuffers(slot, 1, m_buffer.GetAddressOf());
    }

public:
    Cache m_cache;

private:
    ComPtr<ID3D11Buffer> m_buffer;
};

// @TODO: fix
using float4x4 = mat4;
struct PerFrameConstants {
    float4x4 Proj;
    float4x4 View;
    float4x4 ProjView;
    float4x4 _dummy0;
    // float3 EyePos;
    // int NumOfLights;
    // Light Lights[2];
};

struct PerBatchConstants {
    float4x4 Model;
    float4x4 _dummy1;
    float4x4 _dummy2;
    float4x4 _dummy3;
};

struct BoneConstants {
    float4x4 Bones[128];
};

typedef ConstantBuffer<PerFrameConstants> PerFrameBuffer;
typedef ConstantBuffer<PerBatchConstants> PerBatchBuffer;
PerFrameBuffer m_perFrameBuffer;
PerBatchBuffer m_perDrawBuffer;
ConstantBuffer<BoneConstants> m_bone_buffer;
// typedef ConstantBuffer<LightDataCache> LightBuffer;
// typedef ConstantBuffer<ViewPositionCache> ViewPositionBuffer;
// typedef ConstantBuffer<Vec4Cache> FourFloatsBuffer;

////////////////////

bool D3d11GraphicsManager::initialize_internal() {
    bool ok = true;
    ok = ok && create_device();
    ok = ok && create_swap_chain();
    ok = ok && create_render_target();
    ok = ok && ImGui_ImplDX11_Init(m_device.Get(), m_ctx.Get());

    ImGui_ImplDX11_NewFrame();

    select_render_graph();

    m_perFrameBuffer.Create(m_device);
    m_perDrawBuffer.Create(m_device);
    m_bone_buffer.Create(m_device);
    m_perFrameBuffer.VSSet(m_ctx, 0);
    m_perDrawBuffer.VSSet(m_ctx, 1);
    m_bone_buffer.VSSet(m_ctx, 2);
    {
        // rasterizer
        {
            D3D11_RASTERIZER_DESC desc;
            ZeroMemory(&desc, sizeof(desc));
            desc.FillMode = D3D11_FILL_SOLID;
            desc.CullMode = D3D11_CULL_NONE;
            m_device->CreateRasterizerState(&desc, m_rasterizer.GetAddressOf());
            m_ctx->RSSetState(m_rasterizer.Get());
        }

        // set depth function to less equal
        {
            D3D11_DEPTH_STENCIL_DESC dsDesc{};
            dsDesc.DepthEnable = true;
            dsDesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
            dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
            dsDesc.StencilEnable = false;

            m_device->CreateDepthStencilState(&dsDesc, m_depthStencilState.GetAddressOf());

            m_ctx->OMSetDepthStencilState(m_depthStencilState.Get(), 1);
        }
    }

    m_meshes.set_description("GPU-Mesh-Allocator");
    return ok;
}

void D3d11GraphicsManager::finalize() {
    ImGui_ImplDX11_Shutdown();
}

void D3d11GraphicsManager::render() {

    m_render_graph.execute();
    /////////////////////////////

    Scene& scene = SceneManager::singleton().get_scene();

    const mat4 fixup = mat4({ 1, 0, 0, 0 }, { 0, 1, 0, 0 }, { 0, 0, 0.5, 0 }, { 0, 0, 0, 1 }) * mat4({ 1, 0, 0, 0 }, { 0, 1, 0, 0 }, { 0, 0, 1, 0 }, { 0, 0, 1, 1 });

    m_perFrameBuffer.m_cache.View = scene.m_camera->get_view_matrix();
    m_perFrameBuffer.m_cache.Proj = fixup * scene.m_camera->get_projection_matrix();

    m_perFrameBuffer.m_cache.ProjView =
        m_perFrameBuffer.m_cache.Proj *
        m_perFrameBuffer.m_cache.View;

    m_perFrameBuffer.Update(m_ctx);

    for (uint32_t idx = 0; idx < scene.get_count<ObjectComponent>(); ++idx) {
        ecs::Entity entity = scene.get_entity<ObjectComponent>(idx);
        if (!scene.contains<TransformComponent>(entity)) {
            continue;
        }

        const ObjectComponent& obj = scene.get_component_array<ObjectComponent>()[idx];

        const TransformComponent& transform = *scene.get_component<TransformComponent>(entity);
        DEV_ASSERT(scene.contains<MeshComponent>(obj.mesh_id));
        const MeshComponent& mesh = *scene.get_component<MeshComponent>(obj.mesh_id);

        const mat4& world_matrix = transform.get_world_matrix();
        unused(world_matrix);

        // set vertex/index buffer
        auto mesh_buffers = reinterpret_cast<D3d11MeshBuffers*>(mesh.gpu_resource);

        bool has_bone = false;
        if (mesh.armature_id.is_valid()) {
            has_bone = true;

            auto& armature = *scene.get_component<ArmatureComponent>(mesh.armature_id);
            DEV_ASSERT(armature.bone_transforms.size() <= MAX_BONE_COUNT);

            memcpy(m_bone_buffer.m_cache.Bones, armature.bone_transforms.data(), sizeof(mat4) * armature.bone_transforms.size());
            m_bone_buffer.Update(m_ctx);
        }
        set_pipeline_state(has_bone ? PROGRAM_GBUFFER_ANIMATED : PROGRAM_GBUFFER_STATIC);

        m_perDrawBuffer.m_cache.Model = world_matrix;
        m_perDrawBuffer.Update(m_ctx);

        m_ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        set_mesh((MeshBuffers*)mesh.gpu_resource);
        draw_elements(mesh_buffers->index_count, 0);
    }

    /////////////////////////////

    // @TODO: for now, draw stuff here

    // @draw here

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

void D3d11GraphicsManager::on_window_resize(int p_width, int p_height) {
    if (m_device) {
        m_window_rtv.Reset();
        m_swap_chain->ResizeBuffers(0, p_width, p_height, DXGI_FORMAT_UNKNOWN, 0);
        create_render_target();
    }
}

bool D3d11GraphicsManager::create_device() {
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

bool D3d11GraphicsManager::create_swap_chain() {
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

bool D3d11GraphicsManager::create_render_target() {
    ComPtr<ID3D11Texture2D> back_buffer;
    m_swap_chain->GetBuffer(0, IID_PPV_ARGS(back_buffer.GetAddressOf()));
    m_device->CreateRenderTargetView(back_buffer.Get(), nullptr, m_window_rtv.GetAddressOf());
    return true;
}

std::shared_ptr<Texture> D3d11GraphicsManager::create_texture(const TextureDesc& p_texture_desc, const SamplerDesc& p_sampler_desc) {
    unused(p_sampler_desc);

    ComPtr<ID3D11ShaderResourceView> srv;

    PixelFormat format = p_texture_desc.format;
    DXGI_FORMAT texture_format = convert(format);
    DXGI_FORMAT srv_format = convert(format);
    if (format == PixelFormat::D32_FLOAT) {
        texture_format = DXGI_FORMAT_R32_TYPELESS;
        srv_format = DXGI_FORMAT_R32_FLOAT;
    }

    D3D11_TEXTURE2D_DESC texture_desc{};
    texture_desc.Width = p_texture_desc.width;
    texture_desc.Height = p_texture_desc.height;
    texture_desc.MipLevels = p_texture_desc.mip_levels;
    texture_desc.ArraySize = p_texture_desc.array_size;
    texture_desc.Format = texture_format;
    texture_desc.SampleDesc = { 1, 0 };
    texture_desc.Usage = D3D11_USAGE_DEFAULT;
    texture_desc.BindFlags = d3d11::convert_bind_flags(p_texture_desc.bind_flags);
    texture_desc.CPUAccessFlags = 0;
    texture_desc.MiscFlags = d3d11::convert_misc_flags(p_texture_desc.misc_flags);

    D3D11_SUBRESOURCE_DATA texture_data{};
    ComPtr<ID3D11Texture2D> texture;
    texture_data.pSysMem = p_texture_desc.initial_data;
    texture_data.SysMemPitch = p_texture_desc.width * channel_count(format) * channel_size(format);
    texture_data.SysMemSlicePitch = p_texture_desc.height * texture_data.SysMemPitch;

    D3D11_SUBRESOURCE_DATA* texture_data_ptr = p_texture_desc.initial_data ? &texture_data : nullptr;
    D3D_FAIL_V_MSG(m_device->CreateTexture2D(&texture_desc, texture_data_ptr, texture.GetAddressOf()),
                   nullptr,
                   "Failed to create texture");

    D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc{};

    srv_desc.Format = srv_format;
    srv_desc.ViewDimension = d3d11::convert_dimension(p_texture_desc.dimension);
    srv_desc.Texture2D.MostDetailedMip = 0;
    srv_desc.Texture2D.MipLevels = texture_desc.MipLevels;

    D3D_FAIL_V_MSG(m_device->CreateShaderResourceView(texture.Get(), &srv_desc, srv.GetAddressOf()),
                   nullptr,
                   "Failed to create shader resource view");

    auto gpu_texture = std::make_shared<D3d11Texture>(p_texture_desc);
    gpu_texture->srv = srv;
    gpu_texture->texture = texture;
    return gpu_texture;
}

std::shared_ptr<Subpass> D3d11GraphicsManager::create_subpass(const SubpassDesc& p_subpass_desc) {
    auto subpass = std::make_shared<D3d11Subpass>();
    subpass->func = p_subpass_desc.func;
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
            default:
                CRASH_NOW();
                break;
        }
    }

    return subpass;
}

void D3d11GraphicsManager::set_render_target(const Subpass* p_subpass, int p_index, int p_mip_level) {
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

void D3d11GraphicsManager::clear(const Subpass* p_subpass, uint32_t p_flags, float* p_clear_color) {
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

void D3d11GraphicsManager::set_viewport(const Viewport& p_viewport) {
    D3D11_VIEWPORT vp{};
    vp.Width = static_cast<float>(p_viewport.width);
    vp.Height = static_cast<float>(p_viewport.height);
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;

    m_ctx->RSSetViewports(1, &vp);
}

void D3d11GraphicsManager::set_mesh(const MeshBuffers* p_mesh) {
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

void D3d11GraphicsManager::draw_elements(uint32_t p_count, uint32_t p_offset) {
    m_ctx->DrawIndexed(p_count, p_offset, 0);
}

// @TODO: refator
static void create_mesh_data(const MeshComponent& mesh, D3d11MeshBuffers& out_mesh) {
    ID3D11Device* device = get_d3d11_device();

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
        D3D_FAIL_V_MSG(device->CreateBuffer(&bufferDesc, &data, &buffer),
                       nullptr,
                       "Failed to create vertex buffer");
        return buffer;
    };

    out_mesh.vertex_buffer[0] = create_vertex_buffer(sizeof(vec3) * mesh.positions.size(), mesh.positions.data());
    out_mesh.vertex_buffer[1] = create_vertex_buffer(sizeof(vec3) * mesh.normals.size(), mesh.normals.data());

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
        D3D_FAIL_MSG(device->CreateBuffer(&bufferDesc, &data, out_mesh.index_buffer.GetAddressOf()),
                     "Failed to create index buffer");
    }
}

void D3d11GraphicsManager::on_scene_change(const Scene& p_scene) {
    for (size_t idx = 0; idx < p_scene.get_count<MeshComponent>(); ++idx) {
        const MeshComponent& mesh = p_scene.get_component_array<MeshComponent>()[idx];
        if (mesh.gpu_resource != nullptr) {
            ecs::Entity entity = p_scene.get_entity<MeshComponent>(idx);
            const NameComponent& name = *p_scene.get_component<NameComponent>(entity);
            LOG_WARN("[begin_scene] mesh '{}' (idx: {}) already has gpu resource", name.get_name(), idx);
            continue;
        }
        RID rid = m_meshes.make_rid();
        D3d11MeshBuffers* mesh_buffers = m_meshes.get_or_null(rid);
        mesh.gpu_resource = mesh_buffers;
        create_mesh_data(mesh, *mesh_buffers);
    }
}

void D3d11GraphicsManager::set_pipeline_state_impl(PipelineStateName p_name) {
    auto pipeline = reinterpret_cast<D3d11PipelineState*>(m_pipeline_state_manager->find(p_name));
    if (pipeline->vertex_shader) {
        m_ctx->VSSetShader(pipeline->vertex_shader.Get(), 0, 0);
        m_ctx->IASetInputLayout(pipeline->input_layout.Get());
    }
    if (pipeline->pixel_shader) {
        m_ctx->PSSetShader(pipeline->pixel_shader.Get(), 0, 0);
    }
}

}  // namespace my
